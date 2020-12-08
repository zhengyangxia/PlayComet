#include "Revolve.hpp"
#include <iostream>

void Revolve::register_planet(Scene::Transform* t, float period, float dist, glm::vec3 revolve_vec){
    assert(t);
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>(t->name, {period, dist, revolve_vec}));
}

Revolve::Revolve(){
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Mercury", {200.f, 2000.f, glm::vec3(0.f, 1.f, 1.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Venus", {175.f, 3000.f, glm::vec3(0.f, 1.f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Earth", {150.f, 4000.f, glm::vec3(0.f, 0.f, 1.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Mars", {300.f, 5000.f, glm::vec3(0.25f, 0.f, 0.75f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Jupiter", {400.f, 6000.f, glm::vec3(0.75f, 0.25f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Uranus", {600.f, 7000.f, glm::vec3(0.4f, 0.15f, 0.45f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Saturn", {420.f, 8000.f, glm::vec3(0.1f, 0.35f, 0.55f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Venus2", {520.f, 9000.f, glm::vec3(0.55f, 0.25f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Neptune", {480.f, 10000.f, glm::vec3(0.3f, 0.45f, 0.25f)}));
};

void Revolve::set_center(){
}

Revolve::~Revolve(){

};

void Revolve::revolve(Scene::Transform *transform, float elapsed){
    auto attr = attr_map.find(transform->name);
    if (attr == attr_map.end())
    {
        return;
    }
    float dist_to_sun = attr->second.distance;
    float period = attr->second.period_of_revolve;
    glm::vec3 revolve_vector = attr->second.revolve_vector;

    glm::vec3 planet_vector = transform->position;
    float degree = elapsed / period * 360.f;
    planet_vector = glm::normalize(planet_vector);
    
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(degree), revolve_vector);

    glm::vec4 vec(planet_vector.x, planet_vector.y, planet_vector.z, 1.0f);

    vec = trans * vec;

    planet_vector = glm::vec3(vec.x, vec.y, vec.z);
    planet_vector = glm::normalize(planet_vector) * dist_to_sun;

    transform->position = planet_vector;
    return;
}