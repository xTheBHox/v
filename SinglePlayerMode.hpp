#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"

struct SinglePlayerMode : PlayerMode {
	SinglePlayerMode(uint32_t level_num_);

  void level_reset() override;
  void player_set() override;
  void player_switch();
  void handle_reset() override;
  bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

  void update_other_move(float elapsed);
  void update_network() override;

  void update(float elapsed) override;
  
  PlayerData other_pov;

  std::unique_ptr< Server > server;

};
