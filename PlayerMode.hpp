#pragma once

#include "Mode.hpp"
#include "GameLevel.hpp"

struct PlayerMode : Mode {
	PlayerMode(GameLevel *level_);
	virtual ~PlayerMode();

  bool handle_ui(SDL_Event const &, glm::uvec2 const &window_size);

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
  bool won = false;
  bool lost = false;
  bool we_want_reset = false;
  bool they_want_reset = false;
  float reset_countdown = 0.0f;
  uint32_t player_num = 0;

  // Player camera tracked using this structure:
  struct {
    Scene::Camera *camera = nullptr;
    Scene::Transform *body = nullptr;
    glm::vec3 vel = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, 0.0f, -100.0f);
    float azimuth = 90.0f;
    float elevation = 30.0f / 180.0f * 3.1415926f;
    bool in_air = false;
  } pov;

  GameLevel::Movable *on_movable = nullptr;

  GameLevel *level;

};
