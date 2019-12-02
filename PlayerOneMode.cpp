#include "PlayerOneMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"

#include <iostream>
#include <algorithm>

PlayerOneMode::PlayerOneMode(std::string const &port, uint32_t level_num_) : PlayerMode(level_num_, 1) {

  connect = nullptr;
  if (port != "") {
	  server.reset(new Server(port));
  }
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
