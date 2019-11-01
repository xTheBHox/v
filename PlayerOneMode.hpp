#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include "Mesh.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "GameLevel.hpp"

#include <list>

struct PlayerOneMode : Mode {
	PlayerOneMode(GameLevel *level_, std::string const &server_port = "");
	virtual ~PlayerOneMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
  virtual void draw(glm::uvec2 const &drawable_size) override;

  //Current control signals:
	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
    bool sprint = false;
    bool jump = false;
    bool mouse_down = false;
    float mouse_sensitivity = 4.0f;
	} controls;

	bool DEBUG_fly = false;  //fly around for collsion debug
  bool pause = false;

  // Player camera tracked using this structure:
  struct {
    Scene::Camera *camera = nullptr;
    Scene::Transform *body = nullptr;
    glm::vec3 vel = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, 0.0f, -100.0f);
    float azimuth = 0.0f;
    float elevation = 30.0f / 180.0f * 3.1415926f;
    bool in_air = false;
  } pov;

  GameLevel *level;

  std::unique_ptr< Server > server;

	//std::unordered_map< Connection const *, PlayerInfo > connection_infos;

};
