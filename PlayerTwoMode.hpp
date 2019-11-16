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

  std::unique_ptr< Client > client = nullptr;

};
