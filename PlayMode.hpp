#include "Mode.hpp"

#include "Scene.hpp"
#include "ParticleGenerator.hpp"
#include "DrawArrow.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <set>
#include <queue>
#include <utility>
#include <algorithm>

#include "Revolve.hpp"
#include "GravityUtil.hpp"
#include "PostProcessor.hpp"
#include "TexFramebuffer.hpp"
#include "SkyBox.hpp"
#include "Sound.hpp"

// xiaoqiao: dirty workaround for namespace stuff.. should fix it later if i have time
using namespace game_graphics;
using namespace game_graphics::post_processor;

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual void on_resize(glm::uvec2 const &window_size, glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	enum class GameState {
		Flying,
		Grounded,
		Launched,
		Landed,
		EndLose,
		EndWin
	};

	GameState state = GameState::Flying;

	bool speed_is_reset = false;
	float launch_duration = 0.f;
	float launch_limit = 2.f;
	float land_duration = 0.f;
	float land_limit = 5.f;
	float landing_dis = FLT_MAX;

	//player info:
	struct Comet {
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} comet;

	Scene::Transform *comet_parent = nullptr;

	Scene::Camera *universal_camera = nullptr;

	static constexpr float COMET_RADIUS = 1.f;
	static constexpr float PLANET_RADIUS = 20.0f;
	static constexpr float SUN_RADIUS = 300.0f;

	Scene::Transform *sun = nullptr;

	glm::quat initial_camera_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 camera_world_pos;
	glm::vec3 camera_world_rot;

	glm::vec3 comet_velocity = glm::vec3(0.0f, 0.5f, 0.0f);
	glm::vec3 dirx = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 dirz = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec2 mouse_motion;
	// glm::vec3 diry = glm::vec3(0.0f, 0.0f, 1.0f);
	Revolve revolve;
	GravityUtil gravityUtil;

	std::shared_ptr< Sound::PlayingSample > bgm;

	size_t score = 0;
	float court_time = 0.f;
	size_t courting = 0;

	std::vector<Scene::Transform*> asteroids;

	Scene::Transform* item;
	float item_flying;

	struct Asteroid
	{
		Asteroid(Scene::Transform* t, float p, float d, glm::vec3 vec):transform(t), dist(d), period(p), revolve_vec(vec){};
		Scene::Transform* transform;
		float radius = 15.f;
		float dist = 0.f;
		float period = 0.f;
		glm::vec3 revolve_vec;
	};

	

	struct TrajectoryTarget
	{
		TrajectoryTarget(Scene::Transform* t, int s):transform(t), state(s){};
		Scene::Transform* transform;
		float radius = 5.f; // todo distance
		int state = 1; // 1 = present, 0 = has been hit
	};

	struct PlanetSystem
	{
		PlanetSystem(Scene::Transform* t):transform(t){};
		Scene::Transform *transform;
		std::vector<Asteroid> asteroids;
		int trajectory_state = 0; // 1 -> hit all trajectory targets
		// std::vector<Scene::TrajectoryTarget> trajectory_targets; 
	};

    std::unordered_map<std::string, std::vector<TrajectoryTarget>> trajectory_targets;

	struct Planets{
		std::vector<PlanetSystem> planet_systems;
		std::vector<bool> hit_bitmap;
		std::vector<int> radius{200, 150, 100};
		size_t planet_num = 0;
	} planets;

	std::priority_queue<std::pair< float, Scene::Transform* >> nearest_3;
	std::vector<glm::vec2> arrow_pos;

	ParticleGenerator *particle_comet_tail;

	DrawArrow draw_arrow;

private:
	void detect_collision_and_update_state();
	void shoot();
	void reset_speed();

	static constexpr int GAUSSIAN_BLUR_OUTPUT_WIDTH = 480;
	static constexpr int GAUSSIAN_BLUR_OUTPUT_HEIGHT = 270;

	SkyBox skybox{};
	Threshold threshold_processor{1.0f};
	GaussianBlur gaussian_processor{1};
	Identity identity_processor{};
	ToneMapping tone_mapping_processor{};
	AddTwo add_processor{};

	// note: 100x100 is only a temporary value. the real value will be set at
	// on_resize function
	TexFramebufferPtr render_ofb = std::make_shared<TexFramebuffer>(100, 100);
	TexFramebufferPtr threshold_ofb = std::make_shared<TexFramebuffer>(100, 100);
	TexFramebufferPtr tmp_ofb = std::make_shared<TexFramebuffer>(100, 100);
	TexFramebufferPtr add_ofb = std::make_shared<TexFramebuffer>(100, 100);
};
