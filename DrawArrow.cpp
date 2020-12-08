#include "DrawArrow.hpp"
#include "ColorTextureProgram.hpp"

DrawArrow::DrawArrow(){
    glGenVertexArrays(1, &arrow_vertexID);
	glBindVertexArray(arrow_vertexID);
	glGenBuffers(1, &arrow_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, arrow_vertex_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, 30 * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
	GL_ERRORS();
	glBindVertexArray(0);

	//make a 1-pixel white texture to bind by default:
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

DrawArrow::~DrawArrow(){
    glDeleteBuffers(1, &arrow_vertex_buffer);
	glDeleteTextures(1, &textureID);
}

void DrawArrow::draw(std::vector<glm::vec2> arrow_pos){
	glm::mat4 identity_matrix(1.0f);

    std::vector<float> arrow_vertices;
	for (auto a : arrow_pos){
        glm::vec2 left(-0.03f, -0.03f);
        glm::vec2 right(0.03f, -0.03f);
        glm::vec2 top(0.0f, 0.03f);
        glm::vec2 normal = glm::normalize(a);
        float angle = glm::acos(glm::dot(normal, glm::vec2(0.0f, 1.0f)));
        // std::cout<<"angle "<<angle<<std::endl;
        glm::mat2 rotation_matrix(
            glm::cos(angle), -glm::sin(angle),
            glm::sin(angle), glm::cos(angle)
        );
        left = rotation_matrix * left;
        right = rotation_matrix * right;
        top = rotation_matrix * top;
        if (a.x<0){
            left.x = -left.x;
            right.x = -right.x;
            top.x = -top.x;
        }
        left += a;
        right += a;
        top += a;
		float a_arr [] = {
			left.x, left.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
			right.x, right.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
			top.x, top.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		};
		arrow_vertices.insert(arrow_vertices.end(), a_arr, a_arr+30);
	}
	glBindVertexArray(arrow_vertexID);
	glBindBuffer(GL_ARRAY_BUFFER, arrow_vertex_buffer);
	GL_ERRORS();
	glBufferData(GL_ARRAY_BUFFER, arrow_vertices.size() * sizeof(float), arrow_vertices.data(), GL_STREAM_DRAW);
	GL_ERRORS();
	glEnableVertexAttribArray(color_texture_program->Position_vec4);
	glVertexAttribPointer(color_texture_program->Position_vec4, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(color_texture_program->Color_vec4);
	glVertexAttribPointer(color_texture_program->Color_vec4, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (4 * sizeof(float)));
    glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);
	glVertexAttribPointer(color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (8 * sizeof(float)));
	GL_ERRORS();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(color_texture_program->program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(identity_matrix));

	int tri_nums = static_cast<int>(arrow_vertices.size()/10);
	glDrawArrays(GL_TRIANGLES, 0, tri_nums);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisableVertexAttribArray(0);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	GL_ERRORS();
}