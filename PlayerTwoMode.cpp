#include "PlayerTwoMode.hpp"
//#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"
#include "collide.hpp"
#include "GameLevel.hpp"
#include "Scene.hpp"

#include <algorithm>

#include <iostream>

PlayerTwoMode::PlayerTwoMode(std::string const &host, std::string const &port, uint32_t level_num_) : PlayerMode(level_num_, 2) {
  client.reset(new Client(host, port));
  connect = &client->connection;
}

void PlayerTwoMode::update_network() {
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
