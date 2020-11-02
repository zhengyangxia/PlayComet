#include <unordered_map>
#include <string>
#include "Scene.hpp"

struct Revolve
{
    Revolve();
    virtual ~Revolve();
    
    struct PlanetRevolveAttributes {
        float period_of_revolve = 0.f;
        float distance_to_sun = 0.f;
        glm::vec3 revolve_vector = glm::vec3(0.f, 0.f, 1.f);
    };

    glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);
    std::unordered_map<std::string, PlanetRevolveAttributes> attr_map;

    void set_center(glm::vec3 center);
    void revolve(Scene::Transform *transform, float elapsed);
};
