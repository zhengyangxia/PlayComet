//
// Created by Yunfei Cheng on 12/7/20.
//

#ifndef INC_15_466_F20_BASE5_TASK_H
#define INC_15_466_F20_BASE5_TASK_H

#endif //INC_15_466_F20_BASE5_TASK_H

#pragma once

#include "Scene.hpp"

static constexpr float COMET_RADIUS = 1.f;

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
};

struct TrajectoryTarget {
    TrajectoryTarget(Scene::Transform *t, int s) : transform(t), state(s) {};
    Scene::Transform *transform;
    float radius = 25.f; // todo distance
    int state = 1; // 1 = present, 0 = has been hit
};

class BaseTask {
public:
    float planet_radius{};

    BaseTask(Scene::Transform *c, Scene::Transform *p, float pr) : planet_radius{pr}, comet{c}, planet{p},
                                                                   state{ResultType::NOT_COMPLETE} {};
    virtual ~BaseTask() = default;
    virtual ResultType UpdateTask(float elapsed) = 0; //< abstract function

    size_t GetScore(){return score;};

    ResultType GetState(){ return state;};

protected:
    Scene::Transform *comet;
    Scene::Transform *planet;
    ResultType state{};
    std::string notice{};
    size_t score{};

    float GetDistance(){
        return glm::distance(comet->make_local_to_world()[3], planet->make_local_to_world()[3]);
    }

    bool CheckLanded() {
        float comet_planet_dist = glm::distance(comet->make_local_to_world()[3], planet->make_local_to_world()[3]);
        return comet_planet_dist <= COMET_RADIUS + planet_radius;
    };
};

class TrajectTask : public BaseTask {
public:
    TrajectTask(Scene::Transform *c, Scene::Transform *p, float pr, std::vector<TrajectoryTarget> *trajectory_targets)
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
    CourtTask(Scene::Transform *c, Scene::Transform *p, float pr) : BaseTask(c, p, pr) {};

    ~CourtTask() override = default;

    ResultType UpdateTask(float elapsed) override;

private:
    // variables
    float court_time = 0.f;
//    float court_limit = 0.f;
    float court_dist = 100.f;
};

class ShootTask : public BaseTask {
public:
    ShootTask(Scene::Transform *c, Scene::Transform *p, float pr) : BaseTask(c, p, pr) {};

    ~ShootTask() override = default;
    ResultType UpdateTask(float elapsed) override;

private:
    // variables
    std::vector<Asteroid> asteroids;

};