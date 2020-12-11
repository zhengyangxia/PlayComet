#include "TexFramebuffer.hpp"
#include <cassert>
#include "gl_errors.hpp"
namespace game_graphics {
void RenderCaptor::set_render_destination(TexFramebufferPtr dest) {
	if (dest) {
		glBindFramebuffer(GL_FRAMEBUFFER, dest->get_framebuffer_id());
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, drawBufs);
		GL_ERRORS();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		GL_ERRORS();
	}
}
TexFramebuffer::TexFramebuffer(GLsizei drawable_width, GLsizei drawable_height) {
	GL_ERRORS();
	current_drawable_height = drawable_height;
	current_drawable_width = drawable_width;
	glGenFramebuffers(1, &framebuffer_);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	GL_ERRORS();

	glGenTextures(1, &texture_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, drawable_width, drawable_height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_, 0);


	glGenRenderbuffers(1, &depth_render_buffer_);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, drawable_width, drawable_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer_);

//	GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
//	glDrawBuffers(1, drawBufs);
//	GL_ERRORS();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	GL_ERRORS();
}

TexFramebuffer::~TexFramebuffer() {
	glDeleteFramebuffers(1, &framebuffer_);
	glDeleteTextures(1, &texture_);
	glDeleteRenderbuffers(1, &depth_render_buffer_);
	GL_ERRORS();
}

void TexFramebuffer::realloc(GLsizei drawable_width, GLsizei drawable_height) {
	if (this->current_drawable_width == drawable_width && this->current_drawable_height == drawable_height) {
		return;
	}
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, drawable_width, drawable_height, 0, GL_RGBA, GL_FLOAT, nullptr);

	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, drawable_width, drawable_height);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	GL_ERRORS();
}

}
