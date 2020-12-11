#pragma once

#include "Mode.hpp"
#include "PngView.hpp"
#include "data_path.hpp"

struct StartMode : Mode {
	~StartMode() override = default;
	bool handle_event(const SDL_Event &event, const glm::uvec2 &window_size) override;
	void draw(const glm::uvec2 &drawable_size) override;

private:
	PngView title_png_view{glm::vec2(-1.0f, 1.0f), glm::vec2(2.0f, 2.0f), data_path("title.png")};
	PngView instruction_png_view{glm::vec2(-1.0f, 1.0f), glm::vec2(2.0f, 2.0f), data_path("instruction.png")};
	bool show_instruction = false;
};