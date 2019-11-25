#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"

struct PlayerOneMode : PlayerMode {
	PlayerOneMode(std::string const &server_port = "", uint32_t level_num = 1);

	void update_network() override;
  std::unique_ptr< Server > server;

};
