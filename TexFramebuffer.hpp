#pragma once
#include <memory>
#include "GL.hpp"
namespace game_graphics {

class TexFramebuffer;
using TexFramebufferPtr = std::shared_ptr<TexFramebuffer>;

// TODO: what's the difference for the shader when it's rendering to texture
class RenderCaptor {
public:
	/** set the render destination to dest, or when its nullptr, render to screen */
	static void set_render_destination(TexFramebufferPtr dest);
};

class TexFramebuffer : public std::enable_shared_from_this<TexFramebuffer> {
public:
	TexFramebuffer(GLsizei drawable_width, GLsizei drawable_height);
	TexFramebuffer() = delete;
	TexFramebuffer(const TexFramebuffer &other) = delete;
	~TexFramebuffer();
	void realloc(GLsizei drawable_width, GLsizei drawable_height);

	GLuint get_framebuffer_id() { return framebuffer_; }
	GLuint get_texture_id() { return texture_; }
	GLuint get_depth_render_buffer_id() { return depth_render_buffer_; }
private:
	/** framebuffer object */
	GLuint framebuffer_ = 0;
	/** renderedTexture */
	GLuint texture_ = 0;
	/** the attached depth buffer */
	GLuint depth_render_buffer_ = 0;
};

}