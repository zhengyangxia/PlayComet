#include <unordered_map>
#include <string>
#include "Scene.hpp"

struct Revolve
{
    Revolve();
    virtual ~Revolve();
    
    struct PlanetRevolveAttributes {
        float period_of_revolve = 0.f;
        float distance = 0.f;
        glm::vec3 revolve_vector = glm::vec3(0.f, 0.f, 1.f);
    };

    // glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);
    std::unordered_map<std::string, PlanetRevolveAttributes> attr_map;

    void set_center();
    void register_planet(Scene::Transform *transform, float period, float dist, glm::vec3 revolve_vec);
    void revolve(Scene::Transform *transform, float elapsed);

};
