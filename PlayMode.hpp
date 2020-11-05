#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include "Revolve.hpp"
#include "GravityUtil.hpp"

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, clock, aclock;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	enum class GameState {
		Flying,
		Grounded,
		Launched,
		EndLose,
		EndWin
	};

	GameState state = GameState::Flying;

	bool speed_is_reset = false;
	float launch_duration = 0.f;
	float launch_limit = 2.f;

	//player info:
	struct Comet {
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} comet;

	Scene::Transform *comet_parent = nullptr;

	Scene::Camera *universal_camera = nullptr;

	static constexpr float COMET_RADIUS = 1.0f;
	static constexpr float PLANET_RADIUS = 20.0f;
	static constexpr float SUN_RADIUS = 30.0f;

	Scene::Transform *sun = nullptr;

	glm::quat initial_camera_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::vec3 comet_velocity = glm::vec3(0.0f, 0.5f, 0.0f);
	glm::vec3 dirx = glm::vec3(1.0f, 0.0f, 0.0f);
	// glm::vec3 diry = glm::vec3(0.0f, 0.0f, 1.0f);
	Revolve revolve;
	GravityUtil gravityUtil;

	size_t score = 0;
	struct Planets{
		std::vector<Scene::Transform *> transforms;
		std::vector<bool> hit_bitmap;
		std::vector<int> radius{20, 15, 10};
		size_t planet_num = 0;
	} planets;
	

private:
	void detect_collision_and_update_state();
	void shoot();
	void reset_speed();
};
