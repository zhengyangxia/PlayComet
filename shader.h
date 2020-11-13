/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef SHADER_H
#define SHADER_H

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL.hpp"
#include "gl_compile_program.hpp"
// General purpsoe shader object. Compiles from file, generates
// compile/link-time error messages and hosts several utility 
// functions for easy management.
class Shader
{
public:
    // state
    unsigned int ID; 
    // constructor
    Shader() { 
        ID = gl_compile_program(R"glsl(
#version 330 core
// layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>
// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec4 squareVertices;
layout(location = 1) in vec4 xyzs; // Position of the center of the particule and size of the square
layout(location = 2) in vec4 color; // Position of the center of the particule and size of the square

out vec2 TexCoords;
out vec4 ParticleColor;

// Values that stay constant for the whole mesh.
uniform vec3 CameraRight_worldspace;
uniform vec3 CameraUp_worldspace;
uniform mat4 VP; // Model-View-Projection matrix, but without the Model (the position is in BillboardPos; the orientation depends on the camera)

void main()
{
    float particleSize = xyzs.w;
    vec3 particleCenter_wordspace = xyzs.xyz;

    vec3 vertexPosition_worldspace = 
		particleCenter_wordspace
		+ CameraRight_worldspace * squareVertices.x * particleSize
		+ CameraUp_worldspace * squareVertices.y * particleSize;

    // Output position of the vertex
	gl_Position = VP * vec4(vertexPosition_worldspace, 1.0f);

    // float scale = 10.0f;
    TexCoords = squareVertices.zw;
    ParticleColor = color;
    // gl_Position = vec4(xyzs.x - squareVertices.x * particleSize, xyzs.y - squareVertices.y * particleSize, xyzs.z, 1.0f) /*projection * vec4((vertex.xy * scale) + offset, 0.0, 1.0)*/;
}
)glsl", R"glsl(
#version 330 core
in vec2 TexCoords;
in vec4 ParticleColor;
out vec4 color;

uniform sampler2D sprite;

void main()
{
    // color = (texture(sprite, TexCoords) * ParticleColor);
    color = ParticleColor;
}
)glsl");
    }
    // sets the current shader as active
    Shader  &Use();
    // compiles the shader from given source code
    void    Compile(const char *vertexSource, const char *fragmentSource, const char *geometrySource = nullptr); // note: geometry source code is optional 
    // utility functions
    void    SetFloat    (const char *name, float value, bool useShader = false);
    void    SetInteger  (const char *name, int value, bool useShader = false);
    void    SetVector2f (const char *name, float x, float y, bool useShader = false);
    void    SetVector2f (const char *name, const glm::vec2 &value, bool useShader = false);
    void    SetVector3f (const char *name, float x, float y, float z, bool useShader = false);
    void    SetVector3f (const char *name, const glm::vec3 &value, bool useShader = false);
    void    SetVector4f (const char *name, float x, float y, float z, float w, bool useShader = false);
    void    SetVector4f (const char *name, const glm::vec4 &value, bool useShader = false);
    void    SetMatrix4  (const char *name, const glm::mat4 &matrix, bool useShader = false);
private:
    // checks if compilation or linking failed and if so, print the error logs
    void    checkCompileErrors(unsigned int object, std::string type); 
};

#endif