#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>



struct Flower {
    Scene::Transform* root = nullptr;
    Scene::Drawable*  draw = nullptr;
};

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *bird_root = nullptr;
	Scene::Transform *wing_left = nullptr;
	Scene::Transform *wing_right = nullptr;

	glm::quat wing_left_rotation, wing_right_rotation;
	float curr_phase = 0.0f;
	
	Scene::Camera *camera = nullptr;
	std::vector<Flower> flowers;
};