#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"

struct PlayerOneMode : PlayerMode {
	PlayerOneMode(std::string const &server_port = "");

	void update_network() override;
  std::unique_ptr< Server > server;

};
