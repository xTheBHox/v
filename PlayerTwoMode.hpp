#pragma once

#include "Mode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "PlayerMode.hpp"

struct PlayerTwoMode : PlayerMode {

  PlayerTwoMode(std::string const &host, std::string const &port, uint32_t level_num);

  void level_change(uint32_t level_num_) override;

  void update_network() override;
  std::unique_ptr< Client > client = nullptr;

};
