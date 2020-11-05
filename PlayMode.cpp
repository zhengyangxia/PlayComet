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
	std::string planetPrefix = "Planet";
	for (auto &transform : scene.transforms)
	{
		if (std::strlen(transform.name.c_str()) >= 6 && std::strncmp(transform.name.c_str(), "Player", 6) == 0)
		{
			comet.transform = &transform;
			comet_parent = comet.transform->parent;
		}else if (transform.name.find(planetPrefix) == 0)
		{
			planets.transforms.push_back(&transform);
			planets.planet_num ++;
		}else if (std::strlen(transform.name.c_str()) >= 3 && std::strncmp(transform.name.c_str(), "Sun", 3) == 0)
		{
			sun = &transform;
		}
	}
	planets.hit_bitmap.resize(planets.planet_num, false);
	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());

	comet.camera = &scene.cameras.back();
	comet.camera->fovy = glm::radians(60.0f);
	comet.camera->near = 0.01f;
	comet.camera->transform->parent = comet.transform;
	
	comet.camera->transform->position = glm::vec3(0.0f, -50.0f, 10.0f);
	comet.camera->transform->rotation = initial_camera_rotation;

	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	universal_camera = &scene.cameras.back();
	universal_camera->fovy = glm::radians(60.0f);
	universal_camera->near = 0.01f;
	universal_camera->transform->position = sun->position + glm::vec3(0, 0, 250.0f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	// comet.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	revolve.set_center(sun->position);
	//loop through planets
	for (auto p : planets.transforms)
	{
		revolve.revolve(p, 0.f);
	}

	// adding planet1 and sun to gravityUtil
	gravityUtil.register_planet(sun, 200.0f);
	for (auto &p : planets.transforms)
	{
		gravityUtil.register_planet(p, 100.f);
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (state == GameState::Launched)
	{
		// not accept any input
		return true;
	}

	if (evt.type == SDL_KEYDOWN) {
		if (state == GameState::Grounded)
		{
			speed_is_reset = true;
			state = GameState::Launched;
			return true;
		}
		
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
		} else if (evt.key.keysym.sym == SDLK_q) {
			aclock.downs += 1;
			aclock.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			clock.downs += 1;
			clock.pressed = true;
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
		} else if (evt.key.keysym.sym == SDLK_q) {
			aclock.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			clock.pressed = false;
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

void PlayMode::reset_speed(){
	assert(comet.transform->parent);

	glm::vec3 center = comet.transform->parent->position;

	comet.transform->position += comet.transform->parent->position;
	comet.transform->parent = comet_parent;

	glm::vec3 speed_vector = glm::normalize(comet.transform->position - center);
	glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(speed_vector));
	comet.transform->rotation = rotation * comet.transform->rotation;
	dirx = rotation * dirx;

	comet_velocity = speed_vector;
	return;
}

void PlayMode::update(float elapsed) {
	//loop planets
	for (auto &p : planets.transforms)
	{
		revolve.revolve(p, elapsed);
	}
	
	if (speed_is_reset)
	{
		speed_is_reset = false;
		reset_speed();
		return;
	}

	if (state == GameState::Launched)
	{
		launch_duration += elapsed;
		if (launch_duration < launch_limit)
		{
			return;
		}
		launch_duration = 0.f;
		state = GameState::Flying;
		return;
	}

	detect_collision_and_update_state();
	if (state == GameState::EndLose || state == GameState::EndWin) {
		return;
	}

	//player walking:
	if (state != GameState::Grounded) {
		//combine inputs into a move:
		constexpr float PlayerSpeed = 1.f;
		glm::vec3 move = glm::vec3(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		if (clock.pressed && !aclock.pressed) move.z = 1.0f;
		if (!clock.pressed && aclock.pressed) move.z = -1.0f;
		// std::cout << "dot product " << glm::dot(comet_velocity, dirx) << std::endl;
		glm::vec3 new_comet_velocity = comet_velocity + move.x * dirx * PlayerSpeed * elapsed;
		// add gravity to new_comet_velocity
		new_comet_velocity += gravityUtil.get_acceleration(comet.transform->position) * elapsed;
		glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(new_comet_velocity));
		glm::vec3 deltav = glm::normalize(new_comet_velocity)*move.y;
		if (glm::length2(new_comet_velocity) > 5.0f || move.y >= 0.0f)
			comet_velocity = (deltav + new_comet_velocity) * 0.99f;
		else
			comet_velocity = new_comet_velocity * 0.99f;

		dirx = rotation * dirx;
		// diry = rotation * diry;

		comet.transform->rotation = rotation * comet.transform->rotation;
		comet.transform->position += comet_velocity * elapsed;
		
		rotation = glm::angleAxis(glm::radians(move.z*elapsed*45), glm::normalize(comet_velocity));
		comet.transform->rotation = rotation*comet.transform->rotation;
		dirx = rotation * dirx;
		
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	clock.downs = 0;
	aclock.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	universal_camera->aspect = comet.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

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

	if (state == GameState::Flying || state == GameState::EndLose || state == GameState::EndWin) {
		scene.draw(*comet.camera);
	} else {
		scene.draw(*universal_camera);
	}

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
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		std::string score_str = "Score: "+std::to_string(score);
		lines.draw_text(score_str.c_str(),
						glm::vec3(-0.2f + 0.1f * H, 0.7f + 0.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(score_str.c_str(),
						glm::vec3(-0.2f + 0.1f * H + ofs, 0.7f + 0.1f * H + ofs, 0.0),
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
			                glm::vec3(-0.2f + 0.1f * H + ofs, -0.0f + 0.1f * H + ofs, 0.0),
			                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			                glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
	GL_ERRORS();
}

void PlayMode::detect_collision_and_update_state() {
	if (state == GameState::EndLose) { return; }
	glm::vec3 comet_pos = comet.transform->position;
	glm::vec3 sun_pos = sun->position;

	float comet_sun_dist = glm::distance(comet_pos, sun_pos);
	if (comet_sun_dist <= COMET_RADIUS + SUN_RADIUS) {
		state = GameState::EndLose;
	}

	for (size_t i = 0; i< planets.planet_num; i++){
		glm::vec3 planet_pos = planets.transforms[i]->position;
		float comet_planet_dist = glm::distance(comet_pos, planet_pos);
		if (comet_planet_dist <= COMET_RADIUS + planets.radius[i] && planets.hit_bitmap[i] == false) {
			score++;
			planets.hit_bitmap[i] = true;
			state = GameState::Grounded;
			// comet_velocity = glm::vec3(0.0f);

			comet.transform->parent = planets.transforms.at(i);
			glm::vec3 comet_world_position = comet.transform->position;
			glm::vec3 planet_world_position = planets.transforms.at(i)->position;
			comet.transform->position = comet_world_position - planet_world_position;
		}
	}
	if (score == planets.planet_num){
		state = GameState::EndWin;
	}

}
