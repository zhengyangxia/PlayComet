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
    Particle() : pos(0.f), speed(0.f), color(1.f, 1.f, 1.f, 1.f) {}; 
};

const int MaxParticles = 500;

class ParticleGenerator
{
    public:
        ParticleGenerator();
        // update all particles
        // void Update(float dt, GameObject &object, unsigned int newParticles, glm::vec2 offset = glm::vec2(0.0f, 0.0f));
        // render all particles
        void Draw();
    private:
        std::vector<Particle> particles;
        std::shared_ptr<Shader> shader;
        std::shared_ptr<Texture2D> texture;
        unsigned int VAO;
        void init();
};