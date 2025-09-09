#include <set>
#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint bird_vao_for_lit = 0;
Load<MeshBuffer> bird_meshes(LoadTagDefault, []() -> MeshBuffer const *
							 {
	MeshBuffer const *ret = new MeshBuffer(data_path("bird.pnct"));
	bird_vao_for_lit = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

GLuint flower_vao_for_lit = 0;
Load<MeshBuffer> flower_meshes(LoadTagDefault, []() -> MeshBuffer const *
							   {
	MeshBuffer const *ret = new MeshBuffer(data_path("flower.pnct"));
	flower_vao_for_lit = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

GLuint ground_vao_for_lit = 0;
Load<MeshBuffer> ground_meshes(LoadTagDefault, []() -> MeshBuffer const *
							   {
	MeshBuffer const *ret = new MeshBuffer(data_path("ground.pnct"));
	ground_vao_for_lit = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> garden_scene(LoadTagDefault, []() -> Scene const *
						 {
	Scene *garden = new Scene();

	auto load_scene = [&](const char* scene_path,
									 MeshBuffer const* buf, GLuint vao) {
		garden->load(data_path(scene_path),
			[&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
				Mesh const &mesh = buf->lookup(mesh_name);

				scene.drawables.emplace_back(transform);
				Scene::Drawable &drawable = scene.drawables.back();
				
				drawable.pipeline = lit_color_texture_program_pipeline;
				
				drawable.pipeline.vao   = vao;
				drawable.pipeline.type  = mesh.type;
				drawable.pipeline.start = mesh.start;
				drawable.pipeline.count = mesh.count;
			}
		);
	};
	
			
	auto load_flower = [&](glm::vec3 pos, float yaw_rad, float scale = 1.0f) {
		size_t t0 = garden->transforms.size();

		garden->load(data_path("flower.scene"),
			[&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
				Mesh const &mesh = (&*flower_meshes)->lookup(mesh_name);

				scene.drawables.emplace_back(transform);
				Scene::Drawable &drawable = garden->drawables.back();
				
				drawable.pipeline = lit_color_texture_program_pipeline;
				
				drawable.pipeline.vao   = flower_vao_for_lit;
				drawable.pipeline.type  = mesh.type;
				drawable.pipeline.start = mesh.start;
				drawable.pipeline.count = mesh.count;
			}
		);
		glm::quat R = glm::angleAxis(yaw_rad, glm::vec3(0,1,0));

		auto it = garden->transforms.begin();
		std::advance(it, t0);
		for (; it != garden->transforms.end(); ++it) {
			Scene::Transform &transform = *it;
			transform.position = R * transform.position + pos;
		}
	};

	// Example from stack overflow 
	// https://stackoverflow.com/questions/76745282/c-mt19937-getting-the-same-sequence-multiple-times
	{
		std::mt19937 gen(123); 
		std::uniform_real_distribution<float> x_pos(-10.0f, 10.0f);
		std::uniform_real_distribution<float> y_pos(-10.0f, 10.0f);

		const int num_flowers = 10;
		for (int i = 0; i < num_flowers; ++i) {
			load_flower(glm::vec3(x_pos(gen), y_pos(gen), 0.0f), 0.0f, 1.0f);
		}
	}

	load_scene("bird.scene",   &*bird_meshes,   bird_vao_for_lit);
	load_scene("ground.scene", &*ground_meshes, ground_vao_for_lit);

	return garden; });

PlayMode::PlayMode() : scene(*garden_scene)
{
	// INIT TRANSFORMS
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "wing_left")
			wing_left = &transform;
		else if (transform.name == "wing_right")
			wing_right = &transform;
		else if (transform.name == "bird_root")
			bird_root = &transform;
		else if (transform.name.find("flower") != std::string::npos) {
			Flower f;
			f.root = &transform;
			f.pollinated = false;
			flowers.push_back(f);
		}
	}

	if (wing_left == nullptr)
		throw std::runtime_error("wing_left not found.");
	if (wing_right == nullptr)
		throw std::runtime_error("wing_right not found.");
	if (bird_root == nullptr)
		throw std::runtime_error("bird_root not found.");

	wing_left_rotation = wing_left->rotation;
	wing_right_rotation = wing_right->rotation;

	if (scene.cameras.size() != 1)
		throw std::runtime_error("Scene should have ONE camera");
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_ESCAPE)
		{
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		}
		else if (evt.key.key == SDLK_A)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	if (space.pressed)
	{
		float grow_distance = 5.0f;
		for (auto &flower : flowers) {
			if (!flower.pollinated) {
				float dist = glm::distance(flower.root->position, bird_root->position);
				if (dist < grow_distance) {
					flower.root->scale = glm::vec3(2.f, 2.f, 1.2f); // grow flower
					flower.pollinated = true;
				}
			}
		}
	}

	// animate wings
	if (left.pressed || right.pressed || up.pressed || down.pressed)
	{
		curr_phase += elapsed * 2.0f;
		float amt = 0.6f * std::sin(curr_phase * float(M_PI));
		wing_left->rotation = wing_left_rotation * glm::angleAxis(+amt, glm::vec3(0, 1, 0));
		wing_right->rotation = wing_right_rotation * glm::angleAxis(-amt, glm::vec3(0, 1, 0));
		wing_left->rotation = glm::normalize(wing_left->rotation);
		wing_right->rotation = glm::normalize(wing_right->rotation);
	}

	{
		static float rotation = 0.0f;
		float amt = 0.0f;
		if (left.pressed && !right.pressed)
			amt += 1.0f;
		if (!left.pressed && right.pressed)
			amt -= 1.0f;
		rotation += amt * 1.8f * elapsed;
		bird_root->rotation = glm::angleAxis(rotation, glm::vec3(0, 0, 1));
		amt = 0.0f;
		if (up.pressed && !down.pressed)
			amt = +1.0f;
		else if (!up.pressed && down.pressed)
			amt = -1.0f;
		bird_root->position += bird_root->rotation * glm::vec3(0, -1, 0) * (amt * 5.0f * elapsed);
		bird_root->position.z = -2.0f; // Keep bird slightly above ground
	}

	// Set camera to follow bird
	{
		const glm::vec3 camera_offset(0.0f, +23.0f, +8.0f); // behind & above
		camera->transform->position = bird_root->position + bird_root->rotation * camera_offset;
		camera->transform->rotation = glm::quat_cast(glm::inverse(glm::lookAt(camera->transform->position, bird_root->position, glm::vec3(0, 0, 1))));
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f);
	// 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	// this is the default depth comparison function, but FYI you can change it.
	GL_ERRORS(); // print any errors produced by this setup code
	scene.draw(*camera);

	{ // use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		constexpr float H = 0.09f;
		int num_pollinated = 0;
		for (const auto& flower : flowers) {
			if (flower.pollinated) num_pollinated++;
		}
		std::string text = "Use WASD to move Fauna, press space to pollinate nearby flowers! Pollinated: " + std::to_string(num_pollinated) + "/ 10";
		if (num_pollinated == 10) {
			text = "Yay Fauna! You pollinated all the flowers :D";
		}
		lines.draw_text(text,
						glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text,
						glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
