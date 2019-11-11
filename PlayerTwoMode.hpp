#pragma once

#include "Mode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "PlayerMode.hpp"

struct PlayerTwoMode : PlayerMode {
  PlayerTwoMode(GameLevel *level_, std::string const &host, std::string const &port);
	//virtual ~PlayerTwoMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
  virtual void draw(glm::uvec2 const &drawable_size) override;

  struct {
    bool flat = false;
    float drag_sensitivity = 20.0f;
  } controls_shift;

  struct {
    GameLevel::Screen *sc = nullptr;
    // How much the camera has shifted
    float progress = 0.0f;
    float speed = 4.0f; // 1/t, t is the time taken to shift
  } shift;

  std::list< GameLevel::Movable *> currently_moving;
  std::unique_ptr< Client > client = nullptr;

};
