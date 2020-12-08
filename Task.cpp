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
