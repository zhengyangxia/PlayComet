#include "ParticleGenerator.hpp"
#include "shader.h"
#include "texture.h"
#include "gl_errors.hpp"

ParticleGenerator::ParticleGenerator(){
        this->shader.reset(new Shader());
        this->texture.reset(new Texture2D());
        this->texture->Generate(1, 1);
        this->init();
}

void ParticleGenerator::init(){
    // set up mesh and attribute properties
    for (size_t i = 0; i < this->particles.size(); i++)
    {
        this->particles[i].pos = glm::vec3(0.f);
    }


    unsigned int VBO;
    float particle_quad[] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    }; 
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(this->VAO);
    GL_ERRORS();

    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(this->VAO);
    // fill mesh buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particle_quad), particle_quad, GL_STATIC_DRAW);
    // set mesh attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // create this->amount default particle instances
    for (unsigned int i = 0; i < MaxParticles; ++i)
        this->particles.push_back(Particle());

    std::cout << this->particles.size() << "\n";
    assert(this->particles.size()==500);

    // fill mesh buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particle_quad), particle_quad, GL_STATIC_DRAW);
    GL_ERRORS();

    // set mesh attributes
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // glBindVertexArray(0);

    // set mesh attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    GL_ERRORS();

    // create this->amount default particle instances
    for (unsigned int i = 0; i < MaxParticles; ++i)
        this->particles.push_back(Particle());
}

// TODO -> update->position relative to camera

void ParticleGenerator::Draw(){
    // use additive blending to give it a 'glow' effect
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    this->shader->Use();
    GL_ERRORS();

    // glUseProgram();
    for (Particle particle : this->particles)
    {
        // if (particle.Life > 0.0f)
        {
            this->shader->SetVector2f("offset", glm::vec2(particle.pos.x, particle.pos.y));
            GL_ERRORS();
            this->shader->SetVector4f("color", particle.color);
            GL_ERRORS();
            this->texture->Bind();
            glBindVertexArray(this->VAO);
            GL_ERRORS();
            glDrawArrays(GL_TRIANGLES, 0, 6);
            GL_ERRORS();
            glBindVertexArray(0);
        }
    }
    GL_ERRORS();
    // don't forget to reset to default blending mode
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}