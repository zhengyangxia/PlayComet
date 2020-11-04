#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint comet_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > comet_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("comet.pnct"));
	comet_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > comet_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("comet.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = comet_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = comet_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*comet_scene) {
	for (auto &transform : scene.transforms)
	{
		if (std::strlen(transform.name.c_str()) >= 6 && std::strncmp(transform.name.c_str(), "Player", 6) == 0)
		{
			comet.transform = &transform;
		}else if (std::strlen(transform.name.c_str()) >= 7 && std::strncmp(transform.name.c_str(), "Planet1", 7) == 0)
		{
			planet = &transform;
		}else if (std::strlen(transform.name.c_str()) >= 3 && std::strncmp(transform.name.c_str(), "Sun", 3) == 0)
		{
			sun = &transform;
		}
	}
	
	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());

	comet.camera = &scene.cameras.back();
	comet.camera->fovy = glm::radians(60.0f);
	comet.camera->near = 0.01f;
	comet.camera->transform->parent = comet.transform;
	
	comet.camera->transform->position = glm::vec3(0.0f, -10.0f, 0.0f);
	comet.camera->transform->rotation = initial_camera_rotation;

	//rotate camera facing direction (-z) to player facing direction (+y):
	// comet.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	revolve.set_center(sun->position);
	revolve.revolve(planet, 0.f);

	// adding planet1 and sun to gravityUtil
	gravityUtil.register_planet(planet, 100.0f);
	gravityUtil.register_planet(sun, 200.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE)
		{
			// space.pressed = !space.pressed;
			if (SDL_GetRelativeMouseMode() == SDL_FALSE)
			{
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}else{
				comet.camera->transform->rotation = initial_camera_rotation;
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			return true;
		} else if (SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} 
	/*
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} */
	else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			comet.camera->transform->rotation = glm::normalize(
				comet.camera->transform->rotation
				* glm::angleAxis(-motion.x * comet.camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * comet.camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}
	
	return false;
}

void PlayMode::update(float elapsed) {
	detect_collision_and_update_state();
	if (state != GameState::InGame) {
		return;
	}

	revolve.revolve(planet, elapsed);

	//player walking:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 0.1f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		// std::cout << "dot product " << glm::dot(comet_velocity, dirx) << std::endl;
		glm::vec3 new_comet_velocity = comet_velocity + (move.x * dirx + move.y * diry) * PlayerSpeed * elapsed;
		glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(new_comet_velocity));

		// add gravity to new_comet_velocity
		new_comet_velocity += gravityUtil.get_acceleration(comet.transform->position) * elapsed;

		comet_velocity = new_comet_velocity;
		dirx = rotation * dirx;
		diry = rotation * diry;
		comet.transform->rotation = rotation * comet.transform->rotation;
		comet.transform->position += comet_velocity * elapsed;
		
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	comet.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*comet.camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		// Draw win/lose text
		if (state == GameState::EndWin || state == GameState::EndLose) {
			std::string prompt = state == GameState::EndWin ? "You win." : "You lose.";
			constexpr float H = 0.20f;
			lines.draw_text(prompt.c_str(),
			                glm::vec3(-0.2f + 0.1f * H, -0.0f + 0.1f * H, 0.0),
			                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			                glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(prompt.c_str(),
			                glm::vec3(-0.2f + 0.1f * H + ofs, -0.0f + + 0.1f * H + ofs, 0.0),
			                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			                glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
	GL_ERRORS();
}

void PlayMode::detect_collision_and_update_state() {
	if (state != GameState::InGame) { return; }
	glm::vec3 comet_pos = comet.transform->position;
	glm::vec3 sun_pos = sun->position;
	glm::vec3 planet_pos = planet->position;

	float comet_sun_dist = glm::distance(comet_pos, sun_pos);
	if (comet_sun_dist <= COMET_RADIUS + SUN_RADIUS) {
		state = GameState::EndLose;
	}
	float comet_planet_dist = glm::distance(comet_pos, planet_pos);
	if (comet_planet_dist <= COMET_RADIUS + PLANET_RADIUS) {
		state = GameState::EndWin;
	}
}
