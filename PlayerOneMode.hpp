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
    bool mouse_locked = true;
    bool mouse_down = false;
    float mouse_sensitivity = 4.0f;
    bool flat = false;
	} controls;

  bool pause = false;

  // Player camera tracked using this structure:
  struct {
    Scene::Camera *camera = nullptr;
    glm::vec3 vel = glm::vec3(0.0f, 0.0f, 0.0f);
    float azimuth = 0.0f;
    float elevation = 0.0f;
  } pov;

  GameLevel *level;
  
  std::unique_ptr< Server > server;

	//std::unordered_map< Connection const *, PlayerInfo > connection_infos;

};
