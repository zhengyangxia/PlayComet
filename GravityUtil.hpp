#pragma once

#include <vector>
#include "Scene.hpp"

class GravityUtil {
public:
	GravityUtil();

	struct PlanetGravityProperty {
		const Scene::Transform *transform;
		float weight_co;
	};

	void register_planet(const Scene::Transform *transform, float weight_coefficient);

	glm::vec3 get_acceleration(glm::vec3 position);

private:
	static constexpr float GRAVITY_COEFFICIENT = 500.0f;
	std::vector<PlanetGravityProperty> planets;
};