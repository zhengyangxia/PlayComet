#include "GravityUtil.hpp"

GravityUtil::GravityUtil() = default;

void GravityUtil::register_planet(const Scene::Transform *transform, float weight_coefficient) {
	planets.push_back(PlanetGravityProperty{transform, weight_coefficient});
}

glm::vec3 GravityUtil::get_acceleration(glm::vec3 position) {
	glm::vec3 total_accel = glm::vec3(0.0f);
	for (const auto &p : planets) {
		float distance = glm::distance(p.transform->position, position);
		glm::vec3 norm = glm::normalize(p.transform->position - position);
		glm::vec3 accel = GRAVITY_COEFFICIENT * p.weight_co * norm / (distance * distance);
		total_accel += accel;
	}
	return total_accel;
}
