#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

static std::map<std::string, float> after_hit;

GLuint comet_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> comet_meshes(LoadTagDefault, []() -> MeshBuffer const * {
    MeshBuffer const *ret = new MeshBuffer(data_path("comet.pnct"));
    comet_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    return ret;
});

Load<Scene> comet_scene(LoadTagDefault, []() -> Scene const * {
    return new Scene(data_path("comet.scene"),
                     [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
                         Mesh const &mesh = comet_meshes->lookup(mesh_name);
                         scene.drawables.emplace_back(transform);
                         Scene::Drawable &drawable = scene.drawables.back();

                         drawable.pipeline = lit_color_texture_program_pipeline;

                         std::map<std::string, std::string> planet_texture_png = {
                                 {"Icosphere", "planet/rocks_ground.png"},
                                 {"Planet1",   "planet/jupiter.png"},
                                 {"Planet2",   "planet/earth.png"},
                                 {"Planet3",   "planet/mars.png"},
                                 {"Asteroid",  "planet/asteroid.png"}
                         };

                         for (const auto &p : planet_texture_png) {
                             if (p.first == mesh_name) {
                                 glm::uvec2 png_size;
                                 std::vector<glm::u8vec4> data;
                                 load_png(data_path(p.second), &png_size, &data, LowerLeftOrigin);
                                 GLuint icosphere_texture_id = 0;
                                 glGenTextures(1, &icosphere_texture_id);
                                 glBindTexture(GL_TEXTURE_2D, icosphere_texture_id);
                                 glTexImage2D(GL_TEXTURE_2D,
                                              0, GL_RGBA, png_size.x, png_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                              data.data()
                                 );
                                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                 GL_ERRORS();
                                 drawable.pipeline.textures[0].texture = icosphere_texture_id;
                                 drawable.pipeline.textures[0].target = GL_TEXTURE_2D;
                                 glBindTexture(GL_TEXTURE_2D, 0);
                             }
                         }

                         // Sphere.001 is the mesh for the sun.
                         // TODO(xiaoqiao, zizhuol): change blender so the mesh for sun has a better name
                         bool is_emissive = mesh_name == "Sphere.001";
                         GLuint program_id = lit_color_texture_program_pipeline.program;
                         after_hit[drawable.transform->name] = 0.0f;
                         drawable.pipeline.set_uniforms = [=]() {
                             glUseProgram(program_id);
                             GLuint is_emissive_uniform_loc = glGetUniformLocation(program_id, "is_emissive");
                             glUniform1i(is_emissive_uniform_loc, is_emissive);
                             GLuint after_hit_uniform_loc = glGetUniformLocation(program_id, "after_hit");
                             glUniform1f(after_hit_uniform_loc, after_hit[drawable.transform->name]);
                         };

                         drawable.pipeline.vao = comet_meshes_for_lit_color_texture_program;
                         drawable.pipeline.type = mesh.type;
                         drawable.pipeline.start = mesh.start;
                         drawable.pipeline.count = mesh.count;

                     });
});

float get_random_float(float max, float min) {
    return (float) (rand() % (int) (max - min) + min);
}

glm::vec3 get_random_vec() {
    return glm::normalize(glm::vec3(rand() % 100, rand() % 100, rand() % 100));
}

void initialize_asteroids(std::vector<Asteroid> &asteroids, std::vector<Scene::Transform *> &planet_transforms) {
    unsigned int j = 0;
    for (unsigned int i = 0; i < asteroids.size(); i++) {
        j = j % planet_transforms.size();
        auto &pt = planet_transforms[j];
//		planet_system.asteroids.push_back(PlayMode::Asteroid(asteroids[i], get_random_float(50.f, 20.f), get_random_float(1000.f, 500.f), get_random_vec()));
        // todo -> if shooting task?
        asteroids[i].transform->parent = pt;
        j += 1;
    }
}

void scale_asteroids(std::vector<Asteroid> &asteroids, float scale) {
    assert(scale >= 0.f);
    for (auto &as: asteroids) {
        as.transform->scale.x *= scale;
        as.transform->scale.y *= scale;
        as.transform->scale.z *= scale;
        as.radius *= scale;
    }
}

Load<Sound::Sample> music_sample(LoadTagDefault, []() -> Sound::Sample const * {
    return new Sound::Sample(data_path("Mana Two - Part 1.wav"));
});

PlayMode::PlayMode() : scene(*comet_scene) {
    std::string planetPrefix = "Planet";
    std::string trajectoryPrefix = "Tra";
    std::set<std::string> planetNames{"Earth", "Jupiter", "Mars"};
    std::string asteroidPrefix = "Asteroid";

    for (const std::string &planet_name : planetNames) {
        trajectory_targets.insert(
                std::pair<std::string, std::vector<TrajectoryTarget>>(planet_name, std::vector<TrajectoryTarget>()));
    }

    for (auto &transform : scene.transforms) {
        if (std::strlen(transform.name.c_str()) >= 6 && std::strncmp(transform.name.c_str(), "Player", 6) == 0) {
            comet.transform = &transform;
            comet_parent = comet.transform->parent;
        } else if (planetNames.find(transform.name) != planetNames.end()) {
            planet_transforms.push_back(&transform);
            planet_name_to_transform[transform.name] = &transform;
        } else if (std::strlen(transform.name.c_str()) >= 3 && std::strncmp(transform.name.c_str(), "Sun", 3) == 0) {
            sun = &transform;
        } else if (transform.name.find(asteroidPrefix) == 0) {
            asteroids.push_back(Asteroid(&transform, get_random_float(50.f, 20.f), get_random_float(1000.f, 500.f),
                                         get_random_vec()));
        } else if (transform.name.find(trajectoryPrefix) == 0) {
            if (transform.name.find("Earth") == trajectoryPrefix.length() + 1) {
                trajectory_targets["Earth"].push_back(TrajectoryTarget(&transform, 1));
            } else if (transform.name.find("Jupiter") == trajectoryPrefix.length() + 1) {
                trajectory_targets["Jupiter"].push_back(TrajectoryTarget(&transform, 1));
            } else if (transform.name.find("Mars") == trajectoryPrefix.length() + 1) {
                trajectory_targets["Mars"].push_back(TrajectoryTarget(&transform, 1));
            }

        }

    }

    auto cur_trajectory_target = trajectory_targets.find("Earth");
    assert(cur_trajectory_target != trajectory_targets.end());

    planet_name_to_task["Earth"] = std::make_shared<TrajectTask>(comet.transform, planet_name_to_transform["Earth"],
                                                                 150.f,
                                                                 &cur_trajectory_target->second); // earth radius?

                                                                 planet_name_to_task["Jupiter"] = std::make_shared<CourtTask>(
                                                                         comet.transform,
                                                                         planet_name_to_transform["Jupiter"],
                                                                         150.f);
    // todo shoot task -> asteroids
    planet_name_to_task["Mars"] = std::make_shared<ShootTask>(comet.transform, planet_name_to_transform["Mars"], 150.f);

    // match asteroids to planets and initialize the related info
    initialize_asteroids(asteroids, planet_transforms);

    // match planet to sun
    for (auto &ps: planet_transforms) {
        ps->parent = sun;
    }

    // scale the radius of the planets
    scale_asteroids(asteroids, 1.f);

    // comet.transform->scale *= 0.1f;
    sun->position = glm::vec3(0.f);
    comet.transform->position = sun->position + glm::vec3(0, -1000.f, 0);

    //create a player camera attached to a child of the player transform:
    scene.transforms.emplace_back();
    scene.cameras.emplace_back(&scene.transforms.back());
    comet.camera = &scene.cameras.back();
    comet.camera->fovy = glm::radians(45.0f);
    comet.camera->near = 0.1f;
    comet.camera->transform->parent = comet.transform;

    comet.camera->transform->position = glm::vec3(0.0f, -25.0f, 5.0f);
    comet.camera->transform->rotation = initial_camera_rotation;

    scene.transforms.emplace_back();
    scene.cameras.emplace_back(&scene.transforms.back());
    universal_camera = &scene.cameras.back();
    universal_camera->fovy = glm::radians(60.0f);
    universal_camera->near = 0.1f;
    universal_camera->transform->position = sun->position + glm::vec3(0, 0, 2500.0f);

    // initialize the revolution
    for (size_t pos = 0; pos < planet_transforms.size(); pos++) {
        auto &pt = planet_transforms[pos];

        revolve.revolve(pt, (float) (std::rand() % 100));
    }

    for (auto &as: asteroids) {
        revolve.register_planet(as.transform, as.period, as.dist, get_random_vec());
        revolve.revolve(as.transform, (float) (std::rand() % 100));
    }

    // adding planet1 and sun to gravityUtil
    gravityUtil.register_planet(sun, 200.0f);
    for (auto &p : planet_transforms) {
        gravityUtil.register_planet(p, 100.f);
    }

    particle_comet_tail = new ParticleGenerator();

    // bgm = Sound::loop_3D(*music_sample, 2.5f, comet.camera->transform->position, 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (state == GameState::Launched || state == GameState::Landed) {
        // not accept any input
        return true;
    }

    if (evt.type == SDL_KEYDOWN) {
        if (state == GameState::Grounded) {
            speed_is_reset = true;
            state = GameState::Launched;
            camera_world_pos = comet.camera->transform->position;
            return true;
        }

        if (evt.key.keysym.sym == SDLK_SPACE) {
            // if (SDL_GetRelativeMouseMode() == SDL_FALSE)
            // {
            // 	SDL_SetRelativeMouseMode(SDL_TRUE);
            // }else{
            // 	comet.camera->transform->rotation = initial_camera_rotation;
            // 	SDL_SetRelativeMouseMode(SDL_FALSE);
            // }
            return true;
        } else if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            return true;
        } else if (evt.key.keysym.sym == SDLK_a) {
            // left.downs += 1;
            left.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_d) {
            // right.downs += 1;
            right.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_w) {
            // up.downs += 1;
            up.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_s) {
            // down.downs += 1;
            down.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_e) {
            key_e.pressed = true;
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
        } else if (evt.key.keysym.sym == SDLK_e) {
            key_e.pressed = false;
            return true;
        }
    }
        else if (evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT) {
		mouse_left.pressed = true;
		return true;

//		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
//			SDL_SetRelativeMouseMode(SDL_TRUE);
//			return true;
//		}
	}
	else if (evt.type == SDL_MOUSEBUTTONUP && evt.button.button == SDL_BUTTON_LEFT) {
		mouse_left.pressed = false;
		return true;
	}
    else if (evt.type == SDL_MOUSEMOTION) {
        mouse_motion = glm::vec2(
                evt.motion.x / float(window_size.x) - 0.5,
                evt.motion.y / float(window_size.y) - 0.5
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

void PlayMode::reset_speed() {
    assert(comet.transform->parent);
    // TODO set position = local_to_world & set parent = null ?
    glm::vec3 center = comet.transform->parent->position;

    comet.transform->position += comet.transform->parent->position;
    comet.transform->parent = comet_parent;

    glm::vec3 speed_vector = glm::normalize(comet.transform->position - center);
    glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(speed_vector));
    comet.transform->rotation = rotation * comet.transform->rotation;
    comet.transform->scale = glm::vec3(1.0f);
    comet.dirx = rotation * comet.dirx;
    comet.dirz = rotation * comet.dirz;
    constexpr float LaunchSpeed = 10.f;
    comet_velocity = speed_vector * LaunchSpeed;

    return;
}

void PlayMode::update(float elapsed) {
    if (state == GameState::EndLose || state == GameState::EndWin) {
        return;
    }

    if (speed_is_reset) {
        speed_is_reset = false;
        reset_speed();
        return;
    }

    //player walking:
    if (state != GameState::Grounded && state != GameState::Landed) {
        //combine inputs into a move:
        constexpr float PlayerSpeed = 1.f;
        glm::vec3 move = glm::vec3(0.0f);
        if (down.pressed && !up.pressed) move.y = -1.0f;
        if (!down.pressed && up.pressed) move.y = 1.0f;
        if (right.pressed && !left.pressed) move.z = 1.0f;
        if (!right.pressed && left.pressed) move.z = -1.0f;
        glm::vec3 new_comet_velocity = comet_velocity;
        // add gravity to new_comet_velocity
        new_comet_velocity += gravityUtil.get_acceleration(comet.transform->position) * elapsed;
        glm::quat rotation = glm::rotation(glm::normalize(comet_velocity), glm::normalize(new_comet_velocity));
        glm::vec3 deltav = glm::normalize(new_comet_velocity) * move.y * PlayerSpeed;
        if (glm::length(new_comet_velocity) > 5.0f || move.y >= 0.0f)
            comet_velocity = (deltav + new_comet_velocity) * 0.995f;
        else
            comet_velocity = new_comet_velocity * 0.995f;
        comet.dirx = rotation * comet.dirx;
        comet.dirz = rotation * comet.dirz;

        comet.transform->rotation = rotation * comet.transform->rotation;
        comet.transform->position += comet_velocity * elapsed;

        rotation = glm::angleAxis(glm::radians(move.z * elapsed * 45), glm::normalize(comet_velocity));
        comet.transform->rotation = rotation * comet.transform->rotation;
        comet.dirx = rotation * comet.dirx;
        comet.dirz = rotation * comet.dirz;

        glm::quat rotation_x = glm::angleAxis(-mouse_motion.x * comet.camera->fovy * elapsed, comet.dirz);
        glm::quat rotation_y = glm::angleAxis(-mouse_motion.y * comet.camera->fovy * elapsed, comet.dirx);

        comet_velocity = rotation_x * rotation_y * comet_velocity;
        comet.transform->rotation = rotation_x * rotation_y * comet.transform->rotation;
        comet.dirx = rotation_x * comet.dirx;
        comet.dirz = rotation_y * comet.dirz;

    }

    // 0.0 loop planets
    for (auto &pt : planet_transforms) {
        revolve.revolve(pt, elapsed);
    }
    for (auto &as : asteroids) {
        revolve.revolve(as.transform, elapsed);
    }

    // 0.1 update position
    glm::vec3 next_pos = comet.transform->position;
    particle_comet_tail->Update(elapsed, next_pos, comet.camera->transform->position,
                                glm::mat4(comet.camera->transform->make_world_to_local()),
                                comet.camera->make_projection());

    // 1. collision -> sun & asteroids
    if (state == GameState::Flying)
        detect_failure_collision();

    if (state == GameState::Landed) {
        land_duration += elapsed;
        comet.camera->transform->position += elapsed * glm::normalize(comet.camera->transform->position) * 1000.f;
        if (land_duration < land_limit) {
            return;
        } else {
            // std::cout << comet.camera->transform->position.x << " " << comet.camera->transform->position.y << " " << comet.camera->transform->position.z << std::endl;
            // std::cout << comet.transform->position.x << " " << comet.transform->position.y << " " << comet.transform->position.z << std::endl;
            // std::cout << comet.transform->make_local_to_world()[3].x << " " << comet.transform->make_local_to_world()[3].y << " " << comet.transform->make_local_to_world()[3].z << std::endl;
            camera_world_pos = comet.camera->transform->make_local_to_world()[3];
            comet.camera->transform->parent = comet_parent;
            comet.camera->transform->position = camera_world_pos;
            comet.camera->transform->rotation = comet.transform->rotation * initial_camera_rotation;
            // std::cout << comet.camera->transform->make_local_to_world()[3].x << " " << comet.camera->transform->make_local_to_world()[3].y << " " << comet.camera->transform->make_local_to_world()[3].z << std::endl;
            // std::cout << comet.camera->transform->position.x << " " << comet.camera->transform->position.y << " " << comet.camera->transform->position.z << std::endl;
        }
        land_duration = 0.f;
        state = GameState::Grounded;
        /*
         * deprecated
        courting = planets.planet_num;
         */
        return;
    }

    if (state == GameState::Launched) {
        launch_duration += elapsed;
        comet.camera->transform->position = (camera_world_pos - comet.transform->make_local_to_world()[3]) *
                                            (1.f - launch_duration / launch_limit) +
                                            comet.transform->make_local_to_world()[3];
        if (launch_duration < launch_limit) {
            return;
        } else {
            comet.camera->transform->parent = comet.transform;
            comet.camera->transform->position = glm::vec3(0.f, -25.f, 5.f);
            comet.camera->transform->rotation = initial_camera_rotation;
        }
        launch_duration = 0.f;
        state = GameState::Flying;

        return;
    }

    // 2. if there's ongoing task -> do task
    //      if task finished -> reset current state
    //      else return
    if (ongoing_task_planet != nullptr) {
        auto task_pair = planet_name_to_task.find(ongoing_task_planet->name);
        assert(task_pair != planet_name_to_task.end());
        auto task = task_pair->second;
        ResultType result = task->UpdateTask(elapsed);
        notice_str = task->GetNotice();
        if (result == ResultType::NOT_COMPLETE) {
            return;
        }
        if (result == ResultType::NOT_COMPLETE_LANDED) {
            state = GameState::Landed;
            comet.transform->parent = ongoing_task_planet;
            glm::vec3 planet_world_position = ongoing_task_planet->position;
            glm::vec3 comet_world_position = comet.transform->position;

            comet.transform->position = comet_world_position - planet_world_position;
            return;
        }
        // task succeeded
        score += task->GetScore();

        up.pressed = false;
        down.pressed = false;
        left.pressed = false;
        right.pressed = false;

        state = GameState::Landed;

        comet.transform->parent = ongoing_task_planet;
        glm::vec3 planet_world_position = ongoing_task_planet->position;
        glm::vec3 comet_world_position = comet.transform->position;

        comet.transform->position = comet_world_position - planet_world_position;

        ongoing_task_planet = nullptr;
        finished_task += 1;

        if (finished_task == planet_transforms.size()) {
            state = GameState::EndWin;
        }

        return;
    }

    // 3. if key E pressed -> within a certain distance to one planet -> set ongoing task
    notice_str = "";
    for (auto transform : planet_transforms) {
        BaseTask *t = planet_name_to_task[transform->name].get();
        assert(t);
        if (t->GetState() != ResultType::SUCCESS) {
            glm::vec3 planet_pos = transform->position;
            float comet_planet_dist = glm::distance(comet.transform->position, planet_pos);
            if (comet_planet_dist < 1000.f) {
                notice_str = "Press E to start the task";
                if (key_e.pressed) {
                    ongoing_task_planet = transform;
                    break;
                }
            }
        }
    }

    if (ongoing_task_planet) {
        return;
    }


    // 4. ？
    if (state == GameState::Flying) {
        update_arrow();
        shooter.updateAndGetBeamIntersection(elapsed);
    }

    // bgm->set_position(comet.camera->transform->position, 1.0f / 60.0f);
}

void PlayMode::update_arrow() {
    for (Scene::Transform *transform : planet_transforms) {
        BaseTask *t = planet_name_to_task[transform->name].get();
        assert(t);
        if (t->GetState() != ResultType::SUCCESS) {
            glm::vec3 planet_pos = transform->position;
            float comet_planet_dist = glm::distance(comet.transform->position, planet_pos);
            nearest_3.push(std::make_pair(comet_planet_dist, transform));
            if (nearest_3.size() > 3) {
                nearest_3.pop();
            }
//            finish = false;
        }
    }

    while (!nearest_3.empty()) {
        auto p = nearest_3.top();
        auto t = p.second;
        // 左右: x, 上下: y
        // glm::vec4 planet_position_in_camera_space =
        // 	glm::mat4(comet.camera->transform->make_world_to_local()) *
        // 	glm::vec4(t->position, 1.0f);

        glm::vec4 planet_position_in_clip_space =
                comet.camera->make_projection() *
                glm::mat4(comet.camera->transform->make_world_to_local()) *
                glm::vec4(t->position, 1.0f);
        planet_position_in_clip_space /= planet_position_in_clip_space.w;

        if (planet_position_in_clip_space.x > -1 && planet_position_in_clip_space.x < 1 &&
            planet_position_in_clip_space.y > -1 && planet_position_in_clip_space.y < 1 &&
            planet_position_in_clip_space.z > -1 && planet_position_in_clip_space.z < 1) {
            //in camera show hud
        } else {
            //show arrow
            float x = planet_position_in_clip_space.x;
            float y = planet_position_in_clip_space.y;
            x = std::min(0.95f, x);
            x = std::max(-0.95f, x);
            y = std::min(0.95f, y);
            y = std::max(-0.95f, y);
            arrow_pos.push_back(glm::vec2(x, y));
        }
        nearest_3.pop();
    }

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
    //update camera aspect ratio for drawable:
    universal_camera->aspect = comet.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    //set up light type and position for lit_color_texture_program:
    // TODO: consider using the Light(s) in the scene to do this
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
    auto light_loc = sun->position;
    auto light_energy = glm::vec3(1e7f);
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

    scene.draw(*comet.camera);
    // todo
    particle_comet_tail->Draw();
    // if (state == GameState::Flying || state == GameState::EndLose || state == GameState::EndWin || state == GameState::Landed) {

    // } else {
    // 	scene.draw(*universal_camera);
    // }

    shooter.drawBeam();

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

    draw_arrow.draw(arrow_pos);
    arrow_pos.clear();

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
        if (state == GameState::Grounded) {
            lines.draw_text("Press any key to launch",
                            glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));

            lines.draw_text("Press any key to launch",
                            glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
        } else if (notice_str.empty()) {
//            std::string delimiter = "\n";
//            std::string token = s.substr(0, s.find(delimiter));
        }

        if (landing_dis < 3000.f && state == GameState::Flying) {
            /* todo
            std::string dis_str = "Distance: " + std::to_string((int) landing_dis);
            if (landing_dis < 1000.f && courting < planets.planet_num && planets.hit_bitmap[courting] == false) {
                dis_str += "/100km";
                std::string court_str = "Courted: " + std::to_string((int) court_time) + "/10s";
                lines.draw_text(court_str.c_str(),
                                glm::vec3(-1.6f + 0.1f * H, 0.25f + 0.1f * H, 0.0),
                                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                                glm::u8vec4(0x00, 0x00, 0x00, 0x00));
                lines.draw_text(court_str.c_str(),
                                glm::vec3(-1.6f + 0.1f * H + ofs, 0.25f + 0.1f * H + ofs, 0.0),
                                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                                glm::u8vec4(0xff, 0xff, 0xff, 0x00));
                std::string target_str = "Target: " + planets.planet_systems.at(courting).transform->name;
                lines.draw_text(target_str.c_str(),
                                glm::vec3(-1.6f + 0.1f * H, 0.4f + 0.1f * H, 0.0),
                                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                                glm::u8vec4(0x00, 0x00, 0x00, 0x00));
                lines.draw_text(target_str.c_str(),
                                glm::vec3(-1.6f + 0.1f * H + ofs, 0.4f + 0.1f * H + ofs, 0.0),
                                glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                                glm::u8vec4(0xff, 0xff, 0xff, 0x00));
            }
            lines.draw_text(dis_str.c_str(),
                            glm::vec3(-1.6f + 0.1f * H, 0.55f + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
            lines.draw_text(dis_str.c_str(),
                            glm::vec3(-1.6f + 0.1f * H + ofs, 0.55f + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
                            */
        }
        /*
        if (task_index == 0) {
            std::string prompt = "Follow the trajectory over the planet";
            lines.draw_text(prompt.c_str(),
                            glm::vec3(-1.6f + 0.1f * H, 0.1f + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
            lines.draw_text(prompt.c_str(),
                            glm::vec3(-1.6f + 0.1f * H + ofs, 0.1f + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
        }
         */

        if (state == GameState::EndWin) {
            std::string prompt = "You have courted all the planets!";
            constexpr float H = 0.20f;
            lines.draw_text(prompt.c_str(),
                            glm::vec3(-1.2f + 0.1f * H, -0.0f + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
            float ofs = 2.0f / drawable_size.y;
            lines.draw_text(prompt.c_str(),
                            glm::vec3(-1.2f + 0.1f * H + ofs, -0.0f + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
        }
        if (state == GameState::EndLose) {
            std::string prompt = "R I P";
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
        std::string score_str = "Score: " + std::to_string(score);
        if (state == GameState::EndLose || state == GameState::EndWin) {
            lines.draw_text(score_str.c_str(),
                            glm::vec3(-0.2f + 0.1f * H, -0.3f + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
            lines.draw_text(score_str.c_str(),
                            glm::vec3(-0.2f + 0.1f * H + ofs, -0.3f + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
        } else {
            lines.draw_text(score_str.c_str(),
                            glm::vec3(-1.6f + 0.1f * H, 0.7f + 0.1f * H, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0x00, 0x00, 0x00, 0x00));
            lines.draw_text(score_str.c_str(),
                            glm::vec3(-1.6f + 0.1f * H + ofs, 0.7f + 0.1f * H + ofs, 0.0),
                            glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
                            glm::u8vec4(0xff, 0xff, 0xff, 0x00));
        }

    }
}

void PlayMode::detect_failure_collision() {
    if (state == GameState::EndLose) { return; }

    glm::vec3 comet_pos = comet.transform->position;

    // for sun
    glm::vec3 sun_pos = sun->position;

    float comet_sun_dist = glm::distance(comet_pos, sun_pos);
    if (comet_sun_dist <= COMET_RADIUS + SUN_RADIUS) {
        state = GameState::EndLose;
    }

    if (state == GameState::EndLose) { return; }

    // for asteroids
    for (auto i = 0; i < asteroids.size(); ++i) {
        auto &t = asteroids[i];
        if (glm::distance(comet_pos, t.transform->make_local_to_world()[3]) <= COMET_RADIUS + t.radius) {
            state = GameState::EndLose;
            break;
        }
    }

    // for planets
    for (Scene::Transform *transform : planet_transforms) {
        if (transform == ongoing_task_planet) {
            continue;
        }

        BaseTask *t = planet_name_to_task[transform->name].get();
        assert(t);
        glm::vec3 planet_pos = transform->position;
        float comet_planet_dist = glm::distance(comet.transform->position, planet_pos);
        if (comet_planet_dist <= COMET_RADIUS + t->planet_radius) {
            state = GameState::Landed;
            comet.transform->parent = transform;
            glm::vec3 planet_world_position = transform->position;
            glm::vec3 comet_world_position = comet.transform->position;

            comet.transform->position = comet_world_position - planet_world_position;
            break;
        }
    }

}

void PlayMode::on_resize(const glm::uvec2 &window_size, const glm::uvec2 &drawable_size) {
    render_ofb->realloc(drawable_size.x, drawable_size.y);
    threshold_ofb->realloc(GAUSSIAN_BLUR_OUTPUT_WIDTH, GAUSSIAN_BLUR_OUTPUT_HEIGHT);
    tmp_ofb->realloc(GAUSSIAN_BLUR_OUTPUT_WIDTH, GAUSSIAN_BLUR_OUTPUT_HEIGHT);
    add_ofb->realloc(drawable_size.x, drawable_size.y);
}

PlayMode::Shooter::Shooter(PlayMode *enclosing_play_mode) {
	this->enclosing_play_mode_ = enclosing_play_mode;
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vertex_position_vbo_);
	glGenBuffers(1, &vertex_color_vbo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_position_vbo_);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_color_vbo_);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	program_ = gl_compile_program(
		R"glsl(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;

out vec4 Color;

void main()
{
    gl_Position = aPos;
    Color = aColor;
}
)glsl",
R"glsl(
#version 330 core
in vec4 Color;
out vec4 FragColor;
void main()
{
  FragColor = Color;
}
)glsl"
	);
	GL_ERRORS();
}
PlayMode::Shooter::~Shooter() {
	glDeleteVertexArrays(1, &vao_);
	glDeleteBuffers(1, &vertex_color_vbo_);
	glDeleteBuffers(1, &vertex_position_vbo_);
	glDeleteProgram(program_);
	GL_ERRORS();
}

void PlayMode::Shooter::drawBeam() {
	if (!is_enabled_ || !is_shooting_) {
		return;
	}
	GL_ERRORS();
	glUseProgram(program_);
	GL_ERRORS();

	glBindVertexArray(vao_);
	GL_ERRORS();

	Scene::Camera *camera = enclosing_play_mode_->comet.camera;
	/*
	 * [0]: near left
	 * [1]: near right
	 * [2]: far left
	 * [3]: far right
	 */
	glm::vec4 beam_position_in_clip[4];
	glm::mat4 world_to_view = glm::mat4(camera->transform->make_world_to_local());
	beam_position_in_clip[0] = world_to_view * beam_start_ + glm::vec4(-BEAM_WIDTH/2, 0.0f, 0.0f, 0.0f);
	beam_position_in_clip[1] = world_to_view * beam_start_ + glm::vec4(BEAM_WIDTH/2, 0.0f, 0.0f, 0.0f);
	beam_position_in_clip[2] = world_to_view * beam_end_ + glm::vec4(-BEAM_WIDTH/2, 0.0f, 0.0f, 0.0f);
	beam_position_in_clip[3] = world_to_view * beam_end_ + glm::vec4(BEAM_WIDTH / 2, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < 4; i++) {
		beam_position_in_clip[i] = enclosing_play_mode_->comet.camera->make_projection() * beam_position_in_clip[i];
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertex_position_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(beam_position_in_clip), beam_position_in_clip, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_color_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(beam_colors_), beam_colors_, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL_ERRORS();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	GL_ERRORS();
}

std::optional<PlayMode::ShootingTarget> PlayMode::Shooter::updateAndGetBeamIntersection(float elapsed) {
	if (!is_enabled_) { return std::nullopt; }

	if (remaining_capacity_ < CAPACITY_THRESHOLD || !enclosing_play_mode_->mouse_left.pressed) {
		remaining_capacity_ = std::min<float>(CAPACITY_MAX, remaining_capacity_ + CAPACITY_RECOVER_SPEED * elapsed);
		is_shooting_ = false;
		return std::nullopt;
	}


	assert(remaining_capacity_ >= CAPACITY_THRESHOLD && enclosing_play_mode_->mouse_left.pressed);
	is_shooting_ = true;
	remaining_capacity_ = std::max<float>(CAPACITY_MIN, remaining_capacity_ - CAPACITY_DRAIN_SPEED * elapsed);

	glm::vec3 ray_start = enclosing_play_mode_->comet.camera->transform->make_local_to_world()[3];
	glm::vec4 ray_direction_homogeneous = glm::mat4(enclosing_play_mode_->comet.camera->transform->make_local_to_world())
		* glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
	auto ray_direction = glm::normalize(glm::vec3(ray_direction_homogeneous));
	auto raySphere = [](glm::vec3 ray_start,
	                    glm::vec3 ray_direction, //< normalized ray direction vector
	                    glm::vec3 sphere_pos,
	                    float sphere_radius) -> std::optional<float> {
		// adapted from:
		// https://github.com/SebLague/Solar-System/blob/0c60882be69b8e96d6660c28405b9d19caee76d5/Assets/Scripts/Celestial/Shaders/Includes/Math.cginc
		glm::vec3 offset = ray_start - sphere_pos;
		float a = 1; // Set to dot(rayDir, rayDir) if rayDir might not be normalized
		float b = 2 * dot(offset, ray_direction);
		float c = glm::dot(offset, offset) - sphere_radius * sphere_radius;
		float d = b * b - 4 * a * c; // Discriminant from quadratic formula

		// Number of intersections: 0 when d < 0; 1 when d = 0; 2 when d > 0
		if (d > 0) {
			float s = sqrt(d);
			float dstToSphereNear = (-b - s) / (2 * a);
//			float dstToSphereFar = (-b + s) / (2 * a);

			// Ignore intersections that occur behind the ray
			if (dstToSphereNear >= 0) {
				return std::make_optional<float>(dstToSphereNear);
			}
		}
		// Ray did not intersect sphere
		return std::nullopt;
	};

	std::optional<ShootingTarget> current_target;

    /* intersection with planet is delayed */
    for (size_t astroid_idx = 0; astroid_idx < enclosing_play_mode_->asteroids.size(); astroid_idx++) {
        auto &astroid = enclosing_play_mode_->asteroids.at(astroid_idx);
        glm::vec3 sphere_pos = astroid.transform->make_local_to_world()[3];
        float sphere_radius = astroid.radius;
        std::optional<float> astroid_distance = raySphere(ray_start, ray_direction, sphere_pos, sphere_radius);
        if (astroid_distance.has_value() && astroid_distance.value() < BEAM_MAX_LEN &&
            (!current_target.has_value() || astroid_distance.value() < current_target->distance)) {
            current_target =
                ShootingTarget{
                    ShootingTargetType::ASTROID,
                    0,
                    (int) astroid_idx,
                    *astroid_distance};
        }
    }

	// set the beam position
	beam_start_ = glm::vec4(enclosing_play_mode_->comet.transform->make_local_to_world()[3], 1.0f);

	if (current_target.has_value()) {
		beam_end_ = glm::vec4(ray_start + ray_direction * current_target->distance, 1.0f);
	} else {
		beam_end_ = glm::vec4(ray_start + ray_direction * BEAM_MAX_LEN, 1.0f);
	}

	return current_target;
}

void PlayMode::Shooter::drawHud() {
	// TODO(xiaoqiao)
}
