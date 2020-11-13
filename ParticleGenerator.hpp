#include<glm/glm.hpp>
#include <vector>
#include <memory>
#include "GL.hpp"
#include "shader.h"
#include "texture.h"

struct Particle
{
    glm::vec3 pos, speed;
	glm::vec4 color; // Color
	float size, angle, weight;
	float life; // Remaining life of the particle. if < 0 : dead and unused.
    float cameradistance; // *Squared* distance to the camera. if dead : -1.0f
    Particle() : pos(0.f), speed(0.f), color(1.f, 1.f, 1.f, 1.f) {}; 

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};

const int MaxParticles = 10000;
const float LifeSpan = 2.f;

class ParticleGenerator
{
    public:
        ParticleGenerator();
        ~ParticleGenerator();
        // update all particles
        // void Update(float dt, GameObject &object, unsigned int newParticles, glm::vec2 offset = glm::vec2(0.0f, 0.0f));
        // render all particles
        void Update(float elapsed, glm::vec3 next_pos, glm::vec3 camera_pos, glm::mat4 view_matrix, glm::mat4 projection_matrix);
        void Draw();
    private:
        void init();
        int find_unused_particle();
        void sort_particles();

        std::vector<Particle> particles;
        std::shared_ptr<Shader> shader;
        std::shared_ptr<Texture2D> texture;
        GLuint VertexArrayID;
        GLfloat *g_particule_position_size_data;
    	GLubyte *g_particule_color_data;
        GLuint billboard_vertex_buffer;
	    GLuint particles_position_buffer;
	    GLuint particles_color_buffer;

        int ParticlesCount = 0;
        int LastUsedParticle = 0;
};