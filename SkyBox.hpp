#pragma once

#include "GL.hpp"
#include "Scene.hpp"

class SkyBox {
public:
	SkyBox();
	SkyBox(const SkyBox &other) = delete;
	SkyBox(SkyBox &&other) = default;
	~SkyBox();
	void draw(const Scene::Camera &camera);
private:
	GLuint cube_texture_ = 0;
	GLuint vao_ = 0;
	GLuint vbo_ = 0;
	GLuint program_ = 0;
};