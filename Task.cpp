//
// Created by Yunfei Cheng on 12/7/20.
//

#include "Task.h"

ResultType TrajectTask::UpdateTask(float elapsed) {
    if (state == ResultType::SUCCESS){
        return state;
    }

    if (trajectory_next_index >= 0 && trajectory_next_index < targets->size()){
        auto &t = (*targets)[trajectory_next_index];
        t.transform->scale = glm::vec3(5.f, 5.f, 5.f);
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

    if (true && !has_item){
        has_item = true;
        flower->scale = glm::vec3(10.f);
        flower->position = asteroids[item_idx].transform->make_local_to_world()[3];
        flower->parent = comet->transform;
        flower->position = glm::vec4(flower->position.x, flower->position.y, flower->position.z, 1.f) * glm::mat4(flower->make_world_to_local());
    }

    if (has_item && flower_time > 0.f){
        
        glm::vec3 delta = flower->position - comet->dirz;
        flower->position -= delta * 5.f * elapsed;
        flower_time -= elapsed;
        if (flower_time <= 0.f){
            flower_time = 0.f;
            flower->position = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }
    // std::cout << flower_time << " " << flower->position.x << " " << flower->position.y << " " << flower->position.z << std::endl;
    if (CheckLanded()){
        if (has_item){
            state = ResultType::SUCCESS;
            score = 100;
        }
    }

    return ResultType::NOT_COMPLETE;
}
