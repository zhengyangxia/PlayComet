#include "ParticleGenerator.hpp"
#include "shader.h"
#include "texture.h"
#include "gl_errors.hpp"
#include <cstdlib>
#include <algorithm>

ParticleGenerator::ParticleGenerator()
{
    this->shader.reset(new Shader());
    this->texture.reset(new Texture2D());
    this->texture->Generate(1, 1);
    
    this->init();
    glUseProgram(0);
}

ParticleGenerator::~ParticleGenerator(){
    // Cleanup VBO and shader
	glDeleteBuffers(1, &particles_color_buffer);
	glDeleteBuffers(1, &particles_position_buffer);
	glDeleteBuffers(1, &billboard_vertex_buffer);
	glDeleteProgram(shader->ID);
	glDeleteTextures(1, &texture->ID);
	glDeleteVertexArrays(1, &VertexArrayID);
    delete(g_particule_color_data);
    delete(g_particule_position_size_data);
    GL_ERRORS();
}


void ParticleGenerator::init(){
    // set up mesh and attribute properties
    for (size_t i = 0; i < this->particles.size(); i++)
    {
        this->particles[i].pos = glm::vec3(0.f);
    }

    g_particule_position_size_data = new GLfloat[MaxParticles * 4];
    g_particule_color_data         = new GLubyte[MaxParticles * 4];
    
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
    GL_ERRORS();

    static const GLfloat g_vertex_buffer_data[] = {
       	-0.5f, -0.5f, 0.0f, 0.0f,
		0.5f, -0.5f, 1.0f, 0.0f,
		-0.5f,  0.5f, 0.0f, 1.0f,
		0.5f,  0.5f, 1.0f, 1.0f
    }; 

	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    GL_ERRORS();

	// The VBO containing the positions and sizes of the particles
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    GL_ERRORS();

	// The VBO containing the colors of the particles
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    GL_ERRORS();

    glBindVertexArray(0);

    // create this->amount default particle instances
    for (unsigned int i = 0; i < MaxParticles; ++i)
        this->particles.push_back(Particle());

    assert(this->particles.size()==MaxParticles);

    for(int i=0; i<MaxParticles; i++){
		particles[i].life = -1.0f;
        particles[i].cameradistance = -1.0f;
	}

}


// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int ParticleGenerator::find_unused_particle(){
	for(int i=LastUsedParticle; i<MaxParticles; i++){
		if (particles[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	for(int i=0; i<LastUsedParticle; i++){
		if (particles[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

void ParticleGenerator::sort_particles(){
	std::sort(&particles[0], &particles[MaxParticles]);
}

void ParticleGenerator::Update(float elapsed, glm::vec3 next_pos, glm::vec3 camera_pos, glm::mat4 view_matrix, glm::mat4 projection_matrix){
    ParticlesCount = 0;
    
    int newparticles = (int)(elapsed*1000.0);
    if (newparticles > (int)(0.016f*1000.0))
        newparticles = (int)(0.016f*1000.0);

    this->shader->Use();

    GLuint CameraRight_worldspace_ID  = glGetUniformLocation(shader->ID, "CameraRight_worldspace");
	GLuint CameraUp_worldspace_ID  = glGetUniformLocation(shader->ID, "CameraUp_worldspace");
	GLuint ViewProjMatrixID = glGetUniformLocation(shader->ID, "VP");

	glm::mat4 ViewProjectionMatrix = projection_matrix * view_matrix;    
    
    glUniform3f(CameraRight_worldspace_ID, view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
	glUniform3f(CameraUp_worldspace_ID, view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
	glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);

    glUseProgram(0);

    glm::vec3 right_speed = glm::vec3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
    glm::vec3 up_vector = glm::vec3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
    glm::vec3 cross_vec = glm::cross(up_vector, right_speed);

    for (int i = 0; i < newparticles; i++)
    {
        int particleIndex = find_unused_particle();
        particles[particleIndex].life = LifeSpan; // This particle will live LifeSpan seconds.
        glm::vec3 offset = ((rand() % 120 - 60) / 200.0f) * right_speed;
        // offset = rand() % 2 == 1 ? offset:-offset;
        offset += ((rand()%120 - 80) / 200.f) * up_vector;
        // offset = rand() % 2 == 1 ? offset:-offset;
        offset -= cross_vec;

        particles[particleIndex].pos = next_pos + 2.f * offset;

        float degree = rand() % 60 + 60.f;
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::rotate(trans, glm::radians(degree), cross_vec);
        // trans = glm::rotate(trans, glm::radians(degree), up_vector);
        glm::vec4 vec(right_speed.x, right_speed.y, right_speed.z, 1.0f);
        vec = trans * vec;

        particles[particleIndex].speed = 2.f * vec;  
        // particles[particleIndex].size = (rand()%2500)/20000.0f + 0.075f;
        particles[particleIndex].size = 0.05f;
        // Very bad way to generate a random color
		particles[particleIndex].color = glm::vec4(247, 247, 224, 1.0);
    }
    
    for (size_t i = 0; i < MaxParticles; i++)
    {
        Particle& p = particles[i];
        if (p.life > 0.f)
        {
            p.life -= elapsed;
            p.pos += p.speed * elapsed;
            p.size += elapsed * 1.5f / LifeSpan;

            // p.color.x -= elapsed * 0;
            p.color.y -= elapsed * 80;
            p.color.z -= elapsed * 150;

            p.color.w = (float)(glm::pow((p.life / LifeSpan), 1) * 1.0 * 255.f);
            if (p.life > 0.f)
            {
                // Fill the GPU buffer
                g_particule_position_size_data[4*ParticlesCount+0] = p.pos.x;
                g_particule_position_size_data[4*ParticlesCount+1] = p.pos.y;
                g_particule_position_size_data[4*ParticlesCount+2] = p.pos.z;
              
                g_particule_position_size_data[4*ParticlesCount+3] = p.size;
                
                // std::cout << p.pos.x << " " << p.pos.y << " " << p.pos.z <<"\n";

                p.cameradistance = glm::length( p.pos - camera_pos );

                g_particule_color_data[4*ParticlesCount+0] = (GLubyte)p.color.x;
                g_particule_color_data[4*ParticlesCount+1] = (GLubyte)p.color.y;
                g_particule_color_data[4*ParticlesCount+2] = (GLubyte)p.color.z;
                g_particule_color_data[4*ParticlesCount+3] = (GLubyte)p.color.w;
    
            }else{
				p.cameradistance = -1.0f;
            }
            ParticlesCount += 1;
        }

    }

    sort_particles();
}
// TODO -> update->position relative to camera

void ParticleGenerator::Draw(){
    glBindVertexArray(VertexArrayID);

    // use additive blending to give it a 'glow' effect
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);
    GL_ERRORS();

    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);
    GL_ERRORS();

	glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    this->shader->Use();
    GL_ERRORS();

    this->texture->Bind();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->ID);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(
        0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
        4,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    GL_ERRORS();

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : x + y + z + size => 4
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );
    GL_ERRORS();

    // 3rd attribute buffer : particles' colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glVertexAttribPointer(
        2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : r + g + b + a => 4
        GL_UNSIGNED_BYTE,                 // type
        GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
        0,                                // stride
        (void*)0                          // array buffer offset
    );
    GL_ERRORS();

    // These functions are specific to glDrawArrays*Instanced*.
    // The first parameter is the attribute buffer we're talking about.
    // The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
    // http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
    glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1
    GL_ERRORS();

    // Draw the particules !
    // This draws many times a small triangle_strip (which looks like a quad).
    // This is equivalent to :
    // for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
    // but faster.
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    GL_ERRORS();
    // don't forget to reset to default blending mode
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
    GL_ERRORS();
}