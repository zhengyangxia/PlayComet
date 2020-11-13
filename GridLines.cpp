#include "GridLines.hpp"
#include "gl_compile_program.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <array>

GridLines::GridLines() {
	vao_ = vbo_ = 0;
	glGenBuffers(1, &vbo_);

	std::array<float, 4 * 3> vertex_data = {
		// vec3: position, vec2: texCoord
		0, 400, 0,
		0, -400, 0,
		800, 400, 0,
		800, -400, 0
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid *)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	program_ = gl_compile_program(
		R"glsl(
#version 330 core
uniform mat4 OBJECT_TO_CLIP;
layout (location = 0) in vec3 aPos;

out vec3 position;

void main()
{
	gl_Position = OBJECT_TO_CLIP * vec4(aPos, 1.0f);
	position = aPos;
}
)glsl",
		R"glsl(
#version 330
in vec3 position;
in vec2 texCoords;
layout (location = 0) out vec4 fragColor;

void main() {
	vec2 scaledCoord = position.xy / 5;
	vec2 ingrid = scaledCoord - floor(scaledCoord);
	bool hasGrid = any(lessThan(ingrid, vec2(0.2, 0.2)));
	fragColor = hasGrid ? vec4(1.0f, 1.0f, 1.0f, 1.0f) : vec4(0.0f);
}
)glsl"
	);
}
GridLines::~GridLines() {
	glDeleteBuffers(1, &vbo_);
	glDeleteVertexArrays(1, &vao_);
	glDeleteProgram(program_);
}
void GridLines::draw(const Scene::Camera &camera) {
	assert(camera.transform);
	glUseProgram(program_);
	glm::mat4 world_to_clip = camera.make_projection() * glm::mat4(camera.transform->make_world_to_local());
	GLuint OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program_, "OBJECT_TO_CLIP");
	glUniformMatrix4fv(OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));
	glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glUseProgram(0);
	glBindVertexArray(0);
}
