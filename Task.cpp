//
// Created by Yunfei Cheng on 12/7/20.
//

#include "Task.h"
#include "gl_errors.hpp"
#include "gl_compile_program.hpp"

ResultType TrajectTask::UpdateTask(float elapsed) {
    if (state == ResultType::SUCCESS){
        return state;
    }

    if (trajectory_next_index >= 0 && trajectory_next_index < targets->size()){
        auto &t = (*targets)[trajectory_next_index];
        t.transform->scale = glm::vec3(5.f, 5.f, 5.f);
        
        glm::vec4 planet_position_in_clip_space =
                comet->camera->make_projection() *
                glm::mat4(comet->camera->transform->make_world_to_local()) *
                glm::vec4(t.transform->make_local_to_world()[3], 1.0f);
        planet_position_in_clip_space /= planet_position_in_clip_space.w;

        if (planet_position_in_clip_space.x > -1 && planet_position_in_clip_space.x < 1 &&
            planet_position_in_clip_space.y > -1 && planet_position_in_clip_space.y < 1 &&
            planet_position_in_clip_space.z > -1 && planet_position_in_clip_space.z < 1) {
            //in camera show hud
        } else {
            //show arrow
            float x = planet_position_in_clip_space.x;
            float y = planet_position_in_clip_space.y;
			float z = planet_position_in_clip_space.z;
            x = std::min(0.95f, x);
            x = std::max(-0.95f, x);
            y = std::min(0.95f, y);
            y = std::max(-0.95f, y);
			if (z>1 || z<-1){
				float x_abs = std::abs(x);
				float y_abs = std::abs(y);
				if (x_abs < 0.95 && y_abs <0.95){
					if (x>0){
						x = -0.95f;
					}else{
						x = 0.95f;
					}
				}
			}
            comet->arrow_pos.push_back(glm::vec2(x, y));
        }

        if (glm::distance(comet->transform->make_local_to_world()[3], t.transform->make_local_to_world()[3]) <= COMET_RADIUS + t.radius) {
            score += 10;
            t.transform->scale *= 0.f;
            trajectory_next_index += 1;
        }
    }

    notice = "Follow the trajectory over the planet\nYou have finished " + std::to_string(trajectory_next_index) + "/" + std::to_string(targets->size());

    state = ResultType::NOT_COMPLETE;

    if (trajectory_next_index == targets->size()){
        notice = "Land to gain the score! ";
    }

    if (CheckLanded()){
        if (trajectory_next_index >= targets->size()){
            notice = "";
            state = ResultType::SUCCESS;
            for (auto& t: *targets) {
                t.transform->scale = glm::vec3(5.f, 5.f, 5.f);
            }
        }else{
            state = ResultType::NOT_COMPLETE_LANDED;
        }
    }

    return state;
}

ResultType CourtTask::UpdateTask(float elapsed) {
    if (state == ResultType::SUCCESS){
        return state;
    }

    court_time += elapsed;
    court_time = std::min(10.f, court_time);

    float dist = GetDistance();
    if (dist > court_dist){
        court_time = 0.f;
    }

    if (CheckLanded()){
        state = ResultType::SUCCESS;
        score = (size_t) (court_time * 10.f);
    }

    return state;
}

ResultType ShootTask::UpdateTask(float elapsed) {
    if (state == ResultType::SUCCESS){
        return state;
    }

	shooter->updateAndGetBeamIntersection(elapsed);

    if (true){
        
        has_item = true;
        flower->scale = glm::vec3(1.f);
        flower->position = asteroids[item_idx].transform->make_local_to_world()[3];
    }

    // if (has_item){
    //     glm::vec3 delta = flower->position - comet->transform->make_local_to_world()[3] - comet->dirz;
    // }

    if (CheckLanded()){
        if (has_item){
            state = ResultType::SUCCESS;
            score = 100;
        }
    }

    return ResultType::NOT_COMPLETE;
}

Shooter::Shooter(Comet *comet, std::vector<Asteroid> *astroids) {
    this->comet_ = comet;
    this->astroids_ = astroids;

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

Shooter::~Shooter() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vertex_color_vbo_);
    glDeleteBuffers(1, &vertex_position_vbo_);
    glDeleteProgram(program_);
    GL_ERRORS();
}

void Shooter::drawBeam() {
    if (!is_enabled_ || !is_shooting_) {
        return;
    }
    GL_ERRORS();
    glUseProgram(program_);
    GL_ERRORS();

    glBindVertexArray(vao_);
    GL_ERRORS();

    Scene::Camera *camera = comet_->camera;
    /*
     * [0]: near left
     * [1]: near right
     * [2]: far left
     * [3]: far right
     */
    glm::vec4 beam_position_in_clip[4];
    glm::mat4 world_to_view = glm::mat4(camera->transform->make_world_to_local());
    beam_position_in_clip[0] = world_to_view * beam_start_ + glm::vec4(-BEAM_WIDTH / 2, 0.0f, 0.0f, 0.0f);
    beam_position_in_clip[1] = world_to_view * beam_start_ + glm::vec4(BEAM_WIDTH / 2, 0.0f, 0.0f, 0.0f);
    beam_position_in_clip[2] = world_to_view * beam_end_ + glm::vec4(-BEAM_WIDTH / 2, 0.0f, 0.0f, 0.0f);
    beam_position_in_clip[3] = world_to_view * beam_end_ + glm::vec4(BEAM_WIDTH / 2, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; i++) {
        beam_position_in_clip[i] = camera->make_projection() * beam_position_in_clip[i];
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

std::optional<ShootingTarget> Shooter::updateAndGetBeamIntersection(float elapsed) {
    if (!is_enabled_) { return std::nullopt; }

    if (remaining_capacity_ < CAPACITY_THRESHOLD || !mouse_left_button_pressed) {
        remaining_capacity_ = std::min<float>(CAPACITY_MAX, remaining_capacity_ + CAPACITY_RECOVER_SPEED * elapsed);
        is_shooting_ = false;
        return std::nullopt;
    }


    assert(remaining_capacity_ >= CAPACITY_THRESHOLD && mouse_left_button_pressed);
    is_shooting_ = true;
    remaining_capacity_ = std::max<float>(CAPACITY_MIN, remaining_capacity_ - CAPACITY_DRAIN_SPEED * elapsed);

    glm::vec3 ray_start = comet_->camera->transform->make_local_to_world()[3];
    glm::vec4 ray_direction_homogeneous =
        glm::mat4(comet_->camera->transform->make_local_to_world())
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
    for (size_t astroid_idx = 0; astroid_idx < astroids_->size(); astroid_idx++) {
        auto &astroid = astroids_->at(astroid_idx);
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
    beam_start_ = glm::vec4(comet_->transform->make_local_to_world()[3], 1.0f);

    if (current_target.has_value()) {
        beam_end_ = glm::vec4(ray_start + ray_direction * current_target->distance, 1.0f);
    } else {
        beam_end_ = glm::vec4(ray_start + ray_direction * BEAM_MAX_LEN, 1.0f);
    }

    return current_target;
}

void Shooter::drawHud() {
    // TODO(xiaoqiao)
}
