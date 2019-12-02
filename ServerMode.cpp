#include "ServerMode.hpp"

#include <algorithm>

#include <iostream>

ServerMode::ServerMode(std::string const &port, uint32_t level_num_) : PlayerMode(level_num_, 1) {

  connect = nullptr;
  if (port != "") {
	  server.reset(new Server(port));
  }

}

void ServerMode::handle_reset() {
  if (connect) {
    we_want_reset = true;
    reset_countdown = 0.01f;
    connect->send('R');
    std::cout << "Requested reset" << std::endl;
    // TODO just call resume at the end
    pause = false;
  } else {

  }
}

void ServerMode::update_network() {

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
