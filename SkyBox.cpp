#include "SkyBox.hpp"
#include "gl_compile_program.hpp"
#include "load_save_png.hpp"
#include "data_path.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>

SkyBox::SkyBox() {
	glGenTextures(1, &cube_texture_);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube_texture_);

	std::vector<std::string> faces = {
		"skybox/right.png",
		"skybox/left.png",
		"skybox/top.png",
		"skybox/bottom.png",
		"skybox/front.png",
		"skybox/back.png"
	};
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		glm::uvec2 size;
		std::vector<glm::u8vec4> data;
		load_png(data_path(faces.at(i)), &size, &data, UpperLeftOrigin);
		// TODO(xiaoqiao): sRGB gamma correction for png?
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
		             0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data()
		);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// source: https://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox_data
	float skyboxVertices[] = {
		// positions
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vbo_);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid *) 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	program_ = gl_compile_program(
		R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 WORLD_TO_CLIP;

void main()
{
    TexCoords = aPos;
    gl_Position = WORLD_TO_CLIP * vec4(aPos, 1.0);
}
)glsl",
		R"glsl(
#version 330 core
layout (location = 0) out vec4 fragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    fragColor = texture(skybox, TexCoords);
}
)glsl");
}

SkyBox::~SkyBox() {
	glDeleteProgram(program_);
	glDeleteVertexArrays(1, &vao_);
	glDeleteBuffers(1, &vbo_);
	glDeleteTextures(1, &cube_texture_);
}

void SkyBox::draw(const Scene::Camera &camera) {
	assert(camera.transform);
	glDepthMask(GL_FALSE);


	glUseProgram(program_);

	GLuint skybox_texture_loc = glGetUniformLocation(program_, "skybox");
	glUniform1i(skybox_texture_loc, 0);

	glm::mat4 world_to_clip = camera.make_projection() * glm::mat4(glm::mat3(camera.transform->make_world_to_local()));
	GLuint WORLD_TO_CLIP_mat4 = glGetUniformLocation(program_, "WORLD_TO_CLIP");
	glUniformMatrix4fv(WORLD_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));

	glBindVertexArray(vao_);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube_texture_);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);

}
