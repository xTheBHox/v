#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"

struct PlayerOneMode : PlayerMode {

  PlayerOneMode(std::string const &server_port = "", uint32_t level_num = 1);
  
  void level_change(uint32_t level_num_) override;
  
  void update_network() override;
  
  std::unique_ptr< Server > server;

};
