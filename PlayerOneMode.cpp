#include "PlayerOneMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"

#include <iostream>
#include <algorithm>

PlayerOneMode::PlayerOneMode(std::string const &port, uint32_t level_num_) : PlayerMode(level_num_) {
  
  player_num = 1;
  level_change(level_num_);
  connect = nullptr;
  if (port != "") {
	server.reset(new Server(port));
  }
}

void PlayerOneMode::level_change(uint32_t level_num_) {
  
  PlayerMode::level_change(level_num_);
  pov.camera = level->cam_P1;
  pov.body = level->body_P1_transform;
  other_player = level->body_P2_transform;
  
  
  std::cout << "Update!!!"<<std::endl;
  
}

void PlayerOneMode::update_network() {

  update_send();
  server->poll([this](Connection *connection, Connection::Event evt) {
    //Read server state
    if (evt == Connection::OnRecv) {
      update_recv(connection->recv_buffer);
    } else if (evt == Connection::OnClose) {
      connect = nullptr;
    } else if (evt == Connection::OnOpen) {
      if (server->connections.size() == 1) {
        connect = &server->connections.front();
      }
    }
  }, 0.0);

}
