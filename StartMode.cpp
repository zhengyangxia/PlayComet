#include "StartMode.hpp"
#include "PlayMode.hpp"

void StartMode::draw(const glm::uvec2 &drawable_size) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (show_instruction) {
		instruction_png_view.draw();
	} else {
		title_png_view.draw();
	}
}
bool StartMode::handle_event(const SDL_Event &event, const glm::uvec2 &window_size) {
	if (show_instruction) {
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_h) {
				show_instruction = false;
				return true;
			}
		}
	} else {
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_RETURN2) {
				Mode::set_current(std::make_shared<PlayMode>());
				return true;
			} else if (event.key.keysym.sym == SDLK_h) {
				show_instruction = true;
				return true;
			}
		}
	}

	return false;
}
