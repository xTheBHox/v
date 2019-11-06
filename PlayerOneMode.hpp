#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"

struct PlayerOneMode : PlayerMode {
	PlayerOneMode(GameLevel *level_, std::string const &server_port = "");
	//virtual ~PlayerOneMode();

	//bool handle_event(SDL_Event const &, glm::uvec2 const &window_size);
	void update(float elapsed) override;
  //void draw(glm::uvec2 const &drawable_size);

  std::unique_ptr< Server > server;

	//std::unordered_map< Connection const *, PlayerInfo > connection_infos;

};
