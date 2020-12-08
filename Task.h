//
// Created by Yunfei Cheng on 12/7/20.
//

#ifndef INC_15_466_F20_BASE5_TASK_H
#define INC_15_466_F20_BASE5_TASK_H

#endif //INC_15_466_F20_BASE5_TASK_H

#pragma once

#include "Scene.hpp"
#include "Sound.hpp"
#include "PngView.hpp"
#include "data_path.hpp"
#include <memory>
#include <iostream>
#include <random>
#include <optional>
#include "Load.hpp"
#include <set>

static constexpr float COMET_RADIUS = 1.f;

//player info:
struct Comet {
    //transform is at player's feet and will be yawed by mouse left/right motion:
    Scene::Transform *transform = nullptr;
    //camera is at player's head and will be pitched by mouse up/down motion:
    Scene::Camera *camera = nullptr;
    glm::vec3 dirx = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 dirz = glm::vec3(0.0f, 0.0f, 1.0f);
    std::vector<glm::vec2> arrow_pos;

    void add_arrow(glm::vec4 pos){
        glm::vec4 planet_position_in_clip_space =
                camera->make_projection() *
                glm::mat4(camera->transform->make_world_to_local()) *
                pos;
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
            if (z > 1 || z < -1) {
                float x_abs = std::abs(x);
                float y_abs = std::abs(y);
                if (x_abs < 0.95 && y_abs < 0.95) {
                    if (x > 0) {
                        x = -0.95f;
                    } else {
                        x = 0.95f;
                    }
                }
            }
            arrow_pos.push_back(glm::vec2(x, y));
        }
    }
};

enum class ResultType {
    NOT_COMPLETE, NOT_COMPLETE_LANDED, SUCCESS
};

struct Asteroid {
    Asteroid(Scene::Transform *t, float p, float d, glm::vec3 vec) : transform(t), dist(d), period(p),
                                                                     revolve_vec(vec) {};
    Scene::Transform *transform;
    float radius = 10.f;
    float dist = 0.f;
    float period = 0.f;
    glm::vec3 revolve_vec;
	bool destroyed = false;
	float health = 1.0f;
	void health_damage(float delta) {
		if (destroyed) {
			return;
		}
		health = std::max(0.0f, health - delta);
		if (health < 1e-7) {
			destroy();
		}
	}

	void destroy() {
		if (destroyed) {
			return;
		}
		destroyed = true;
		transform->scale = glm::vec3(0.0f);
	}
};

struct TrajectoryTarget {
    TrajectoryTarget(Scene::Transform *t, int s) : transform(t), state(s) {};
    Scene::Transform *transform;
    float radius = 25.f; // todo distance
    int state = 1; // 1 = present, 0 = has been hit
};

// forward declaration for Shooter class
class Shooter;


class BaseTask {
public:
    float planet_radius{};

    BaseTask(Comet *c, Scene::Transform *p, float pr) : planet_radius{pr}, comet{c}, planet{p},
                                                                   state{ResultType::NOT_COMPLETE} {};

    virtual ~BaseTask() = default;

    virtual ResultType UpdateTask(float elapsed) = 0; //< abstract function

    size_t GetScore() { return score; };

    ResultType GetState() { return state; };

    std::string GetNotice() { return notice; };

protected:
    Comet *comet;
    Scene::Transform *planet;
    ResultType state{};
    std::string notice{};
    size_t score{};

    float GetDistance() {
        return glm::distance(comet->transform->make_local_to_world()[3], planet->make_local_to_world()[3]);
    }

    bool CheckLanded() {
        float comet_planet_dist = glm::distance(comet->transform->make_local_to_world()[3], planet->make_local_to_world()[3]);
        return comet_planet_dist <= COMET_RADIUS + planet_radius;
    };
};

class TrajectTask : public BaseTask {
public:
    TrajectTask(Comet *c, Scene::Transform *p, float pr, std::vector<TrajectoryTarget> *trajectory_targets)
            : BaseTask(c, p, pr) {
        assert(trajectory_targets);
        targets = trajectory_targets;
        for (auto &t: *targets) {
            t.transform->parent = planet;
            t.transform->scale = glm::vec3{0.f, 0.f, 0.f};
        }

        if (targets->size() >= 0) {
            trajectory_next_index = 0;
            // todo 第一个对象设置cale
            // cur_trajectory_target_vec[0].transform->scale = glm::vec3(5.f, 5.f, 5.f);
        }
    };

    ~TrajectTask() override = default;

    ResultType UpdateTask(float elapsed) override;

private:
    // variables
    size_t trajectory_next_index{0}; // -1=no targets; index >= 0 =next target; size of trajectory vector -> hit all trajectory targets
    std::vector<TrajectoryTarget> *targets{nullptr};
};

class CourtTask : public BaseTask {
public:
    CourtTask(Comet *c, Scene::Transform *p, float pr) : BaseTask(c, p, pr) {
        court_limit = (rand()%5+1) * 5.f;
        court_dist = (rand()%3+1) * 50.f;
    };

    ~CourtTask() override = default;

    ResultType UpdateTask(float elapsed) override;

private:
    // variables
    float court_time = 0.f;
    float court_limit;
    float court_dist ;
};

class ShootTask : public BaseTask {
public:
	ShootTask(Comet *c, Scene::Transform *p, float pr, std::vector<Asteroid> *ast, std::vector<Scene::Transform*> f, Shooter *shooter)
		: BaseTask(c, p, pr), shooter{shooter}, asteroids{ast} {
		for (size_t i = 0; i < ast->size(); i++) {
			if (ast->at(i).transform->parent == p) {
				asteroids_indices_current_task.push_back((int)i);
			}
		}
		num_flower = 0;
        
        for (auto flower : f){
            flower->scale *= 0.f;
            flowers.push_back(flower);
            flower_times.push_back(-1.f);
        }
        while (flower_indices.size() < flowers.size()){
            int idx = rand()%asteroids_indices_current_task.size();
            flower_indices.insert(idx);
        }
	};

    ~ShootTask() override = default;

    ResultType UpdateTask(float elapsed) override;

private:
    // variables
    Shooter *shooter = nullptr;
    std::vector<Asteroid> *asteroids = nullptr;
    std::vector<int> asteroids_indices_current_task;
    std::vector<Scene::Transform*> flowers;
    std::set<int> flower_indices;
    std::vector<float> flower_times;
    size_t num_flower;
};

enum class ShootingTargetType { SUN, PLANET, ASTEROID };
struct ShootingTarget {
    ShootingTargetType type;
    // only valid when type == PLANET
    int planet_system_index;
    // only valid when type == ASTEROID
    int asteroid_index;
    float distance;
};

class ProgressBarView {
public:
	ProgressBarView(glm::vec2 position,
	                glm::vec2 size,
	                float percentage,
	                glm::vec4 foreground_color,
	                glm::vec4 background_color);
	~ProgressBarView();

	ProgressBarView(const ProgressBarView &) = delete;
	ProgressBarView& operator=(const ProgressBarView&) = delete;
	ProgressBarView(ProgressBarView &&) = delete;
	ProgressBarView& operator=(ProgressBarView &&) = delete;

	void setPercentage(float percentage) { percentage_ = percentage; }
	void draw();

private:
	glm::vec2 position_;
	glm::vec2 size_;
	float percentage_ = 0.0f;
	glm::vec4 foreground_color_;
	glm::vec4 background_color_;
	GLuint vao_;
	GLuint position_vbo_;
	GLuint color_vbo_;
	GLuint program_;
};

class Shooter {
public:
    Shooter(Comet *comet, std::vector<Asteroid> *asteroids);
    ~Shooter();
    Shooter(const Shooter &) = delete;
    Shooter& operator=(const Shooter&) = delete;
    Shooter(Shooter &&) = delete;
    Shooter& operator=(Shooter &&) = delete;

    /* if set to true, the light beam (shooting mechanism) will be enabled */
	void setEnabled(bool value) {
		if (!value) {
			setShooting(false);
		}
		is_enabled_ = value;
    }

    /* update the beam and shot object if shooter is activated. do nothing otherwise */
    std::optional<ShootingTarget> updateAndGetBeamIntersection(float elapsed);

    /* draw the shooter-related information, e.g remaining capacity */
    void drawHud();

    /* draw the beam if activation is enabled, do nothing otherwise */
    void drawBeam();

	void notify_target_health(std::optional<float> health) { this->target_health_ = health; }

    bool mouse_left_button_pressed = false;
private:

    bool is_enabled_ = false;

    /* when set to true, there's a visible beam */
    bool is_shooting_ = false;

	void setShooting(bool value);

	static constexpr float AIM_SIZE_Y = 0.1f;
	static constexpr float AIM_SIZE_X = AIM_SIZE_Y * 9 / 16;
	PngView aim_png{glm::vec2( - AIM_SIZE_X / 2, AIM_SIZE_Y / 2), glm::vec2(AIM_SIZE_X, AIM_SIZE_Y), data_path("aim.png")};

    std::shared_ptr<Sound::PlayingSample> sound_effect;

    float remaining_capacity_ = 1.0f;
    static constexpr float CAPACITY_MIN = 0.0f;
    static constexpr float CAPACITY_MAX = 1.0f;
    static constexpr float CAPACITY_RECOVER_SPEED = 0.05f;
    static constexpr float CAPACITY_DRAIN_SPEED = 0.2f;
//    static constexpr float CAPACITY_THRESHOLD = 0.1f;

	static constexpr float BEAM_ENERGY_BAR_TOP_POS = -0.45f;
	static constexpr float BEAM_ENERGY_BAR_LEFT_POS = 0.08f;
	static constexpr float BEAM_ENERGY_BAR_HEIGHT = 0.03f;
	static constexpr float BEAM_ENERGY_BAR_WIDTH = 0.1f;
	static constexpr glm::vec4 white{1.0f};
	static constexpr glm::vec4 black{0.0f, 0.0f, 0.0f, 1.0f};
	static constexpr glm::vec4 color_lightblue{42.f/256, 72.f/256, 193.f/256, 1.0f};
	static constexpr glm::vec4 color_lightred{0.84f, 0.33f, 0.32f, 1.0f};
	ProgressBarView energy_bar{glm::vec2(BEAM_ENERGY_BAR_LEFT_POS, BEAM_ENERGY_BAR_TOP_POS),
	                           glm::vec2(BEAM_ENERGY_BAR_WIDTH, BEAM_ENERGY_BAR_HEIGHT),
	                           remaining_capacity_,
	                           color_lightblue,
	                           black};
	ProgressBarView target_health_bar{glm::vec2{0.05f, 0.01f},
	                                  glm::vec2{0.05f, 0.02f},
	                                  0.0f, color_lightred, black};

    std::optional<float> target_health_ = std::nullopt;

    Comet *comet_ = nullptr;
    std::vector<Asteroid> *asteroids_ = nullptr;

    // [0]: start position, [1] end position
    glm::vec4 beam_start_ = glm::vec4(-1000.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 beam_end_ = glm::vec4(1000.0f, 0.0f, 0.0f, 1.0f);

    static constexpr float BEAM_MAX_LEN = 1000.0f;
    static constexpr float BEAM_WIDTH = 0.5f;
    glm::vec4 beam_colors_[4] = {
        glm::vec4(0.5f, 0.8f, 5.2f, 1.0f),
        glm::vec4(0.5f, 0.8f, 5.2f, 1.0f),
        glm::vec4(0.5f, 0.8f, 5.2f, 1.0f),
        glm::vec4(0.5f, 0.8f, 5.2f, 1.0f),
    };

    GLuint beam_vao_;
    GLuint beam_position_vbo_;
    GLuint beam_color_vbo_;

    GLuint hud_vao_;
    GLuint hud_position_vbo_;
    GLuint hud_color_vbo_;

    GLuint program_;
};

extern Load<Sound::Sample> landing_sample;
