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

		// Sphere.001 is the mesh for the sun.
		// TODO(xiaoqiao, zizhuol): change blender so the mesh for sun has a better name
		bool is_emissive = mesh_name == "Sphere.001";
		GLuint program_id = lit_color_texture_program_pipeline.program;
		drawable.pipeline.set_uniforms = [=]() {
			glUseProgram(program_id);
			GLuint is_emissive_uniform_loc = glGetUniformLocation(program_id, "is_emissive");
			glUniform1i(is_emissive_uniform_loc, is_emissive);
		};

		drawable.pipeline.vao = comet_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

void initialize_asteroids(Scene::Transform& t, PlayMode::Asteroids* asteroids)
{
	asteroids->asteroids_num += 1;
	asteroids->transforms.push_back(&t);
}

void scale_asteroids(PlayMode::Asteroids* asteroids, float scale)
{
	assert(scale >= 0.f);
	for (auto& t: asteroids->transforms)
	{
		t->scale.x *= scale;
		t->scale.y *= scale;
		t->scale.z *= scale;
	}
	asteroids->radius *= scale;
}

PlayMode::PlayMode() : scene(*comet_scene) {
	std::string planetPrefix = "Planet";
	std::string asteroidPrefix = "Asteroid";
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
		}else if (transform.name.find(asteroidPrefix) == 0){
			initialize_asteroids(transform, &asteroids);
		}
	}

	scale_asteroids(&asteroids, 0.5f);
	
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
	universal_camera->transform->position = sun->position + glm::vec3(0, 0, 500.0f);

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

	particle_comet_tail = new ParticleGenerator();

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
			// if (SDL_GetRelativeMouseMode() == SDL_FALSE)
			// {
			// 	SDL_SetRelativeMouseMode(SDL_TRUE);
			// }else{
			// 	comet.camera->transform->rotation = initial_camera_rotation;
			// 	SDL_SetRelativeMouseMode(SDL_FALSE);
			// }
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
		mouse_motion = glm::vec2(
			evt.motion.x / float(window_size.x)-0.5,
			evt.motion.y / float(window_size.y)-0.5
		);
		// std::cout << motion.x << " " << motion.y << std::endl;
		// comet.camera->transform->rotation = glm::normalize(
		// 	comet.camera->transform->rotation
		// 	* glm::angleAxis(-motion.x * comet.camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
		// 	* glm::angleAxis(motion.y * comet.camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
		// );
		

		return true;
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
	dirz = rotation * dirz;
	constexpr float LaunchSpeed = 100.f;
	comet_velocity = speed_vector*LaunchSpeed;
	return;
}

void PlayMode::update(float elapsed) {
	glm::vec3 next_pos = comet.transform->position;
	particle_comet_tail->Update(elapsed, next_pos, comet.camera->transform->position, glm::mat4(comet.camera->transform->make_world_to_local()), comet.camera->make_projection());
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
		// constexpr float PlayerSpeed = 1.f;
		glm::vec3 move = glm::vec3(0.0f);
		// if (left.pressed && !right.pressed) move.x =-1.0f;
		// if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		if (clock.pressed && !aclock.pressed) move.z = 1.0f;
		if (!clock.pressed && aclock.pressed) move.z = -1.0f;
		// std::cout << "dot product " << glm::dot(comet_velocity, dirx) << std::endl;
		glm::vec3 new_comet_velocity = comet_velocity;
		// add gravity to new_comet_velocity
		new_comet_velocity += gravityUtil.get_acceleration(comet.transform->position) * elapsed;
		glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(new_comet_velocity));
		glm::vec3 deltav = glm::normalize(new_comet_velocity)*move.y;
		if (glm::length2(new_comet_velocity) > 5.0f || move.y >= 0.0f)
			comet_velocity = (deltav + new_comet_velocity) * 0.99f;
		else
			comet_velocity = new_comet_velocity * 0.99f;

		dirx = rotation * dirx;
		dirz = rotation * dirz;

		comet.transform->rotation = rotation * comet.transform->rotation;
		comet.transform->position += comet_velocity * elapsed;
		
		rotation = glm::angleAxis(glm::radians(move.z*elapsed*45), glm::normalize(comet_velocity));
		comet.transform->rotation = rotation*comet.transform->rotation;
		dirx = rotation * dirx;
		dirz = rotation * dirz;

		glm::quat rotation_x = glm::angleAxis(-mouse_motion.x * comet.camera->fovy * elapsed, dirz);
		glm::quat rotation_y = glm::angleAxis(-mouse_motion.y * comet.camera->fovy * elapsed, dirx);

		comet_velocity =  rotation_x * rotation_y * comet_velocity;
		comet.transform->rotation = rotation_x * rotation_y * comet.transform->rotation;
		dirx = rotation_x * dirx;
		dirz = rotation_y * dirz;
		
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
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
	auto light_loc = glm::vec3(413.0f, 0.0f, 0.0f);
	auto light_energy = glm::vec3(1e5f);
	glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(light_loc));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light_energy));
	glUseProgram(0);

	RenderCaptor::set_render_destination(render_ofb);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Scene::Camera *current_camera =
		state == GameState::Flying || state == GameState::EndLose || state == GameState::EndWin ? comet.camera
		                                                                                        : universal_camera;
	skybox.draw(*current_camera);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	if (state == GameState::Flying || state == GameState::EndLose || state == GameState::EndWin) {
		scene.draw(*comet.camera);
		particle_comet_tail->Draw();
	} else {
		scene.draw(*universal_camera);
	}

	GL_ERRORS();
	RenderCaptor::set_render_destination(nullptr);
	glViewport(0, 0, GAUSSIAN_BLUR_OUTPUT_WIDTH, GAUSSIAN_BLUR_OUTPUT_HEIGHT);
	threshold_processor.draw(render_ofb, threshold_ofb);
	GL_ERRORS();
	gaussian_processor.draw(threshold_ofb, tmp_ofb);
	GL_ERRORS();
	glViewport(0, 0, drawable_size.x, drawable_size.y);
	add_processor.draw(render_ofb, threshold_ofb, add_ofb);
	tone_mapping_processor.draw(add_ofb, nullptr);

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
		float ofs = 2.0f / drawable_size.y;
		if (state == GameState::Grounded){
			lines.draw_text("Press any key to launch",
			                glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			                glm::u8vec4(0x00, 0x00, 0x00, 0x00));

			lines.draw_text("Press any key to launch",
			                glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			                glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}

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
}

void PlayMode::detect_collision_and_update_state() {
	if (state == GameState::EndLose) { return; }
	glm::vec3 comet_pos = comet.transform->position;
	glm::vec3 sun_pos = sun->position;

	float comet_sun_dist = glm::distance(comet_pos, sun_pos);
	if (comet_sun_dist <= COMET_RADIUS + SUN_RADIUS) {
		state = GameState::EndLose;
	}

	for(auto &t : asteroids.transforms){
		if (glm::distance(comet_pos, t->position) <= COMET_RADIUS + asteroids.radius)
		{
			state = GameState::EndLose;
			break;
		}
	}

	for (size_t i = 0; i< planets.planet_num; i++){
		glm::vec3 planet_pos = planets.transforms[i]->position;
		float comet_planet_dist = glm::distance(comet_pos, planet_pos);
		if (comet_planet_dist <= COMET_RADIUS + planets.radius[i]) {
			if (planets.hit_bitmap[i] == false){
				score++;
				planets.hit_bitmap[i] = true;
			}
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
void PlayMode::on_resize(const glm::uvec2 &window_size, const glm::uvec2 &drawable_size) {
	render_ofb->realloc(drawable_size.x, drawable_size.y);
	threshold_ofb->realloc(GAUSSIAN_BLUR_OUTPUT_WIDTH, GAUSSIAN_BLUR_OUTPUT_HEIGHT);
	tmp_ofb->realloc(GAUSSIAN_BLUR_OUTPUT_WIDTH, GAUSSIAN_BLUR_OUTPUT_HEIGHT);
	add_ofb->realloc(drawable_size.x, drawable_size.y);
}
