#include "PngView.hpp"
#include "ColorTextureProgram.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include <glm/glm.hpp>
#include "gl_errors.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

PngView::PngView(glm::vec2 position, glm::vec2 size, std::string png_path) {
	// vertex attributes for the "quad"
	float quad_vertices[] = {
		// positions vec4         // colors vec4          // texCoords vec2
		position.x, position.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		position.x, position.y - size.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		position.x + size.x, position.y - size.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

        position.x, position.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        position.x + size.x, position.y - size.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		position.x + size.x, position.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
	};
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);

	// GL_STATIC_DRAW: The data store contents will be modified once and used many times as the source for GL drawing commands.
	// GL_DYNAMIC_DRAW: The data store contents will be modified repeatedly and used many times as the source for GL drawing commands.
	// https://www.khronos.org/registry/OpenGL-Refpages/es1.1/xhtml/glBufferData.xml
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	glEnableVertexAttribArray(color_texture_program->Position_vec4);
	glVertexAttribPointer(color_texture_program->Position_vec4, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(color_texture_program->Color_vec4);
	glVertexAttribPointer(color_texture_program->Color_vec4,
	                      4,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      10 * sizeof(float),
	                      (void *) (4 * sizeof(float)));
	glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);
	glVertexAttribPointer(color_texture_program->TexCoord_vec2,
	                      2,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      10 * sizeof(float),
	                      (void *) (8 * sizeof(float)));

	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);

	glm::uvec2 png_size;
	std::vector<glm::u8vec4> data;
	load_png(png_path, &png_size, &data, LowerLeftOrigin);
	glTexImage2D(GL_TEXTURE_2D,
	             0, GL_RGBA, png_size.x, png_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data()
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	GL_ERRORS();
}

PngView::~PngView() {
	glDeleteBuffers(1, &vbo_);
	glDeleteVertexArrays(1, &vao_);
	glDeleteTextures(1, &texture_);
	GL_ERRORS();
}

void PngView::draw() {
	glm::mat4 identity_matrix(1.0f);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(color_texture_program->program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glBindVertexArray(vao_);

	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(identity_matrix));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	GL_ERRORS();
}