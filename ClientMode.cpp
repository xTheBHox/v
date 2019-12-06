#include "ClientMode.hpp"

#include <algorithm>

#include <iostream>

ClientMode::ClientMode(std::string const &host,
                       std::string const &port,
                       uint32_t level_num_,
                       uint32_t player_num_) : PlayerMode(level_num_, player_num_) {

  client.reset(new Client(host, port));
  connect = &client->connection;

}

void ClientMode::handle_reset() {
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

void ClientMode::update_network() {

  if (!connect) return;
  update_send();
  client->poll([this](Connection *connection, Connection::Event evt) {
    //Read server state
    if (evt == Connection::OnRecv) {
      update_recv(connection->recv_buffer);
    } else if (evt == Connection::OnClose) {
      connect = nullptr;
    }
  }, 0.0);

}
