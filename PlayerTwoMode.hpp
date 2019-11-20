#pragma once

#include "Mode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "PlayerMode.hpp"

struct PlayerTwoMode : PlayerMode {
  PlayerTwoMode(std::string const &host, std::string const &port);
	//virtual ~PlayerTwoMode();

	void update_network() override;
  void draw(glm::uvec2 const &drawable_size) override;

  std::unique_ptr< Client > client = nullptr;

};
