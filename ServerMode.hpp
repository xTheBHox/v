#pragma once

#include "PlayerMode.hpp"
#include "Connection.hpp"

struct ServerMode : PlayerMode {

  ServerMode(std::string const &server_port = "", uint32_t level_num = 1, uint32_t player_num = 1);

  void handle_reset() override;

  void update_network() override;

  std::unique_ptr< Server > server;

};
