#include "PostProcessor.hpp"

#include <utility>
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

namespace game_graphics::post_processor {

// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
// src:https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/5.1.framebuffers/framebuffers.cpp
float quad_vertices[] = {
	// positions   // texCoords
	-1.0f, 1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 1.0f, 0.0f,

	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};
BaseProcessor::BaseProcessor(const std::string &fragment_shader_source) {
	glGenVertexArrays(1, &quad_vao_);
	glBindVertexArray(quad_vao_);

	glGenBuffers(1, &quad_vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));

	program_ = gl_compile_program(
		R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords;
}
)glsl",
		fragment_shader_source
	);
}

BaseProcessor::~BaseProcessor() {
	glDeleteVertexArrays(1, &quad_vao_);
	glDeleteBuffers(1, &quad_vbo_);
	glDeleteProgram(program_);
}

void BaseProcessor::draw_quad(std::vector<TexFramebufferPtr> inputs, TexFramebufferPtr output) {
	GL_ERRORS();
	if (output) {
		// setup render destination
		glBindFramebuffer(GL_FRAMEBUFFER, output->get_framebuffer_id());
		GL_ERRORS();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		GL_ERRORS();
	}

	// set input texture
	for (size_t i = 0; i < inputs.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, inputs.at(i)->get_texture_id());
		GL_ERRORS();
	}

//	// copied from learnopengl.com
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GL_ERRORS();
	// set render program
	glUseProgram(program_);
	glBindVertexArray(quad_vao_);
	GL_ERRORS();

	// clear canvas
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
	glDisable(GL_DEPTH_TEST);
	GL_ERRORS();

	// specify the destination and draw
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUseProgram(0);
	GL_ERRORS();
}

Threshold::Threshold(float threshold) : BaseProcessor(
	R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float threshold;

void main()
{
	vec4 inputColor = texture(screenTexture, TexCoords);
	float luminance = dot(inputColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	FragColor = luminance > threshold ? inputColor : vec4(0.0f);
}
)glsl"
), threshold{threshold} {
	GL_ERRORS();
//	GLuint screenTexture_uniform_loc = glGetUniformLocation(program_, "screenTexture");
//	glUniform1i(screenTexture_uniform_loc, 0);
	glUseProgram(program_);
	GLuint threshold_uniform_loc = glGetUniformLocation(program_, "threshold");
	glUniform1f(threshold_uniform_loc, this->threshold);
	GL_ERRORS();
}

void Threshold::draw(TexFramebufferPtr input, TexFramebufferPtr output) {
	draw_quad({std::move(input)}, std::move(output));
}

GaussianBlur::GaussianBlur(int rounds) : BaseProcessor(
	R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform bool horizontal;
uniform float weight[14] = float[] (0.132369, 0.125279, 0.10621, 0.080656, 0.054866, 0.033431, 0.018246, 0.008921, 0.003906, 0.001532, 0.000538, 0.000169, 0.000048, 0.000012);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(screenTexture, 0); // gets size of single texel
    vec3 result = texture(screenTexture, TexCoords).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 14; ++i)
        {
            result += texture(screenTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 14; ++i)
        {
            result += texture(screenTexture, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)glsl"
), rounds{rounds} {
	glUseProgram(program_);
	horizontal_uniform_loc = glGetUniformLocation(program_, "horizontal");
	GLuint screenTexture_uniform_loc = glGetUniformLocation(program_, "screenTexture");
	glUniform1i(screenTexture_uniform_loc, 0);
	glUseProgram(0);
	GL_ERRORS();
}

void GaussianBlur::draw(TexFramebufferPtr input_output, TexFramebufferPtr tmp) {
	GL_ERRORS();
	glUseProgram(program_);
	for (int i = 0; i < rounds; i++) {
		glUseProgram(program_);
		glUniform1i(horizontal_uniform_loc, 1);
		draw_quad({input_output}, tmp);
		glUseProgram(program_);
		glUniform1i(horizontal_uniform_loc, 0);
		draw_quad({tmp}, input_output);
	}
	glUseProgram(0);
	GL_ERRORS();
}
Identity::Identity() : BaseProcessor(
R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
	FragColor = texture(screenTexture, TexCoords);
}
)glsl"
	) {
	glUseProgram(program_);
	GLuint screenTexture_uniform_loc = glGetUniformLocation(program_, "screenTexture");
	glUniform1i(screenTexture_uniform_loc, 0);
	glUseProgram(0);
	GL_ERRORS();
}
void Identity::draw(TexFramebufferPtr input, TexFramebufferPtr output) {
	draw_quad({std::move(input)}, std::move(output));
	GL_ERRORS();
}
ToneMapping::ToneMapping() : BaseProcessor(
R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

//uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;

vec3 aces_approx(vec3 v)
{
	v *= 0.6f;
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}
void main()
{
	vec4 hdrColor = texture(bloomBlur, TexCoords);
	FragColor = vec4(aces_approx(hdrColor.rgb), 1.0f);
}
)glsl"
) {
	glUseProgram(program_);
	GLuint bloomBlur_uniform_loc = glGetUniformLocation(program_, "bloomBlur");
	glUniform1i(bloomBlur_uniform_loc, 0);
	GLuint exposure_loc = glGetUniformLocation(program_, "exposure");
	glUniform1f(exposure_loc, 5.0f);
	glUseProgram(0);
	GL_ERRORS();
}
void ToneMapping::draw(TexFramebufferPtr input, TexFramebufferPtr output) {
	draw_quad({input}, output);
	GL_ERRORS();
}
AddTwo::AddTwo() : BaseProcessor(
	R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D input0;
uniform sampler2D input1;

void main()
{
	FragColor = texture(input0, TexCoords) + texture(input1, TexCoords);
}
)glsl"
) {
	glUseProgram(program_);
	GLuint input0_uniform_loc = glGetUniformLocation(program_, "input0");
	glUniform1i(input0_uniform_loc, 0);
	GLuint input1_uniform_loc = glGetUniformLocation(program_, "input1");
	glUniform1i(input1_uniform_loc, 1);
	glUseProgram(0);
	GL_ERRORS();
}
void AddTwo::draw(TexFramebufferPtr input0, TexFramebufferPtr input1, TexFramebufferPtr output) {
	draw_quad({std::move(input0), std::move(input1)}, std::move(output));
	GL_ERRORS();
}
}