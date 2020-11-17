#include "Revolve.hpp"
#include <iostream>

glm::vec3 get_random_vec(){
    return glm::normalize(glm::vec3(rand()%100, rand()%100, rand()%100));
}

float get_random_float(float max, float min){
    return (float)(rand()%(int)(max-min)+min);
}

Revolve::Revolve(){
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet1", {200.f, 3000.f, glm::vec3(0.f, 1.f, 1.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet2", {150.f, 2000.f, glm::vec3(0.f, 1.f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet3", {150.f, 2000.f, glm::vec3(0.f, 0.f, 1.f)}));

    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.001", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.002", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.003", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.004", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.005", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.006", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.007", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.008", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.009", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));

    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.010", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.011", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.012", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.013", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.014", {get_random_float(50.f, 20.f), get_random_float(3600.f, 2500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.015", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.016", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.017", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.018", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.019", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.020", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.021", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.022", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.023", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.024", {get_random_float(50.f, 20.f), get_random_float(800.f, 500.f), get_random_vec()}));

};

void Revolve::set_center(){
// (glm::vec3 global_center){
    // center = global_center;
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