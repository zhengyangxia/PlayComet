#include "Revolve.hpp"
#include <iostream>

Revolve::Revolve(){
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet1", {200.f, 300.f, glm::vec3(0.f, 1.f, 1.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet2", {150.f, 200.f, glm::vec3(0.f, 1.f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet3", {100.f, 100.f, glm::vec3(0.f, 0.f, 1.f)}));
};

void Revolve::set_center(glm::vec3 global_center){
    center = global_center;
}

Revolve::~Revolve(){

};

void Revolve::revolve(Scene::Transform *transform, float elapsed){
    auto attr = attr_map.find(transform->name);
    if (attr == attr_map.end())
    {
        return;
    }
    float dist_to_sun = attr->second.distance_to_sun;
    float period = attr->second.period_of_revolve;
    glm::vec3 revolve_vector = attr->second.revolve_vector;

    glm::vec3 planet_vector = transform->position - center;
    float degree = elapsed / period * 360.f;
    planet_vector = glm::normalize(planet_vector);
    
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(degree), revolve_vector);

    glm::vec4 vec(planet_vector.x, planet_vector.y, planet_vector.z, 1.0f);

    vec = trans * vec;

    planet_vector = glm::vec3(vec.x, vec.y, vec.z);
    planet_vector = glm::normalize(planet_vector) * dist_to_sun;

    transform->position = center + planet_vector;
    
    return;    
}