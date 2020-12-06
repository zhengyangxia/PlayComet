#include<glm/glm.hpp>
#include <vector>
#include "GL.hpp"
#include "gl_errors.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

class DrawArrow{
    public:
    DrawArrow();
    ~DrawArrow();
    void draw(std::vector<glm::vec2> arrow_pos);
    private:
	GLuint arrow_vertexID;
	GLuint arrow_vertex_buffer;
    GLuint textureID;
};