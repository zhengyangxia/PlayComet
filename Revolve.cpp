#include "Revolve.hpp"
#include <iostream>

glm::vec3 get_random_vec(){
    return glm::normalize(glm::vec3(rand()%100, rand()%100, rand()%100));
}

float get_random_dist(float max, float min){
    return (float)(rand()%(int)(max-min)+min);
}

Revolve::Revolve(){
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet1", {200.f, 3000.f, glm::vec3(0.f, 1.f, 1.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet2", {150.f, 2000.f, glm::vec3(0.f, 1.f, 0.f)}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Planet3", {100.f, 1000.f, glm::vec3(0.f, 0.f, 1.f)}));

    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid", {150.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.001", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.002", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.003", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.004", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.005", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.006", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.007", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.008", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.009", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));

    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.010", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.011", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.012", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.013", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.014", {100.f, get_random_dist(2500.f, 600.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.015", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.016", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.017", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.018", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.019", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.020", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.021", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.022", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.023", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));
    attr_map.insert(std::pair<std::string, PlanetRevolveAttributes>("Asteroid.024", {50.f, get_random_dist(800.f, 500.f), get_random_vec()}));

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