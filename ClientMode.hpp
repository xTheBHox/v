#pragma once

#include "PlayerMode.hpp"
#include "Connection.hpp"

struct ClientMode : PlayerMode {

  ClientMode(std::string const &host, std::string const &port, uint32_t level_num);

  void handle_reset() override;

  void update_network() override;

  std::unique_ptr< Client > client = nullptr;

};
