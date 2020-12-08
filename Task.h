//
// Created by Yunfei Cheng on 12/7/20.
//

#ifndef INC_15_466_F20_BASE5_TASK_H
#define INC_15_466_F20_BASE5_TASK_H

#endif //INC_15_466_F20_BASE5_TASK_H

#pragma once

#include "Scene.hpp"
#include "Sound.hpp"
#include <memory>
#include <iostream>
#include <random>
#include <optional>
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
};

enum class ResultType {
    NOT_COMPLETE, NOT_COMPLETE_LANDED, SUCCESS
};

struct Asteroid {
    Asteroid(Scene::Transform *t, float p, float d, glm::vec3 vec) : transform(t), dist(d), period(p),
                                                                     revolve_vec(vec) {};
    Scene::Transform *transform;
    float radius = 15.f;
    float dist = 0.f;
    float period = 0.f;
    glm::vec3 revolve_vec;
	bool destroyed = false;

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
    int trajectory_next_index = -1; // -1=no targets; index >= 0 =next target; size of trajectory vector -> hit all trajectory targets
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
            std::cout << "inserting "<< idx << std::endl;
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
    int num_flower;
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
class Shooter {
public:
    explicit Shooter(Comet *comet, std::vector<Asteroid> *asteroids);
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

    bool mouse_left_button_pressed = false;
private:

    bool is_enabled_ = false;

    /* when set to true, there's a visible beam */
    bool is_shooting_ = false;

	void setShooting(bool value);

    std::shared_ptr<Sound::PlayingSample> sound_effect;

    float remaining_capacity_ = 1.0f;
    static constexpr float CAPACITY_MIN = 0.0f;
    static constexpr float CAPACITY_MAX = 1.0f;
    static constexpr float CAPACITY_RECOVER_SPEED = 0.1f;
    static constexpr float CAPACITY_DRAIN_SPEED = 0.1f;
    static constexpr float CAPACITY_THRESHOLD = 0.1f;

    Comet *comet_ = nullptr;
    std::vector<Asteroid> *asteroids_ = nullptr;

    // [0]: start position, [1] end position
    glm::vec4 beam_start_ = glm::vec4(-1000.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 beam_end_ = glm::vec4(1000.0f, 0.0f, 0.0f, 1.0f);

    static constexpr float BEAM_MAX_LEN = 10000.0f;
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
