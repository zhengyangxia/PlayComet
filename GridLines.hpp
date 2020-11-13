#pragma once

#include "GL.hpp"
#include "Scene.hpp"

class GridLines {
public:
	GridLines();
	GridLines(const GridLines &other) = delete;
	GridLines(GridLines &&other) = default;
	~GridLines();
	void draw(const Scene::Camera &camera);
private:
	GLuint vao_;
	GLuint vbo_;
	GLuint program_;
};