#pragma once

#include <string>
#include <vector>
#include "GL.hpp"
#include "TexFramebuffer.hpp"

namespace game_graphics::post_processor {
class BaseProcessor {
public:
	explicit BaseProcessor(const std::string &fragment_shader_source);
	virtual ~BaseProcessor();
	BaseProcessor() = delete;
	BaseProcessor(const BaseProcessor &other) = delete;

protected:
	void draw_quad(std::vector<TexFramebufferPtr> inputs, TexFramebufferPtr outptut);

protected:
	// code from https://learnopengl.com/Advanced-OpenGL/Framebuffers
	GLuint program_;
	GLuint quad_vao_;
	GLuint quad_vbo_;
};

/**
 * threshold post processor:
 * blackout all pixels whose color is darker than threshold.
 */
class Threshold : public BaseProcessor {
public:
	explicit Threshold(float threshold);
	Threshold() = delete;
	Threshold(const Threshold &other) = delete;
	void draw(TexFramebufferPtr input, TexFramebufferPtr output);
private:
	const float threshold;
};

class GaussianBlur : public BaseProcessor {
public:
	explicit GaussianBlur(int rounds);
	GaussianBlur(const GaussianBlur &other) = delete;
	void draw(TexFramebufferPtr input_output, TexFramebufferPtr tmp);
private:
	const int rounds;
	int horizontal_uniform_loc;
};

class ToneMapping : public BaseProcessor {
public:
	ToneMapping();
	ToneMapping(const ToneMapping &other) = delete;
	void draw(TexFramebufferPtr input, TexFramebufferPtr output);
};

class AddTwo : public BaseProcessor {
public:
	AddTwo();
	void draw(TexFramebufferPtr input0, TexFramebufferPtr input1, TexFramebufferPtr output);
};

class Identity : public BaseProcessor {
public:
	Identity();
	Identity(const Identity &other) = delete;
	void draw(TexFramebufferPtr input, TexFramebufferPtr output);
private:
};
}