#include "PlayerOneMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"

#include <iostream>
#include <algorithm>

#define PI 3.1415926f

PlayerOneMode::PlayerOneMode( GameLevel *level_ , std::string const &port) : PlayerMode(level_){
  player_num = 1;
  pov.camera = level->cam_P1;
  pov.body = level->body_P1_transform;
  if (port != "") {
		server.reset(new Server(port));
	}
}

void PlayerOneMode::update(float elapsed) {

  PlayerMode::update(elapsed);

  if (server) {
    /**
    auto remove_player = [this](Connection *c) {
			auto f = connection_infos.find(c);
			if (f != connection_infos.end()) {
				connection_infos.erase(f);
			}
		};**/

		//syncing player pos

		if (server->connections.size() != 0){
			server->connections.begin()->send('P');
			//std::cout << "Sent P1 pos" << std::endl;
			auto pos = level->body_P1_transform->position;
			//std::cout << pos.x <<" " << pos.y << " " <<  pos.z << std::endl;
			server->connections.begin()->send(pos);
		}


		if (level->resetSync)
		{
			//TEMP
			server->connections.begin()->send('R');
			level->resetSync = false;
		}
    server->poll([this](Connection *connection, Connection::Event evt){
			if (evt == Connection::OnRecv) {
					//extract and erase data from the connection's recv_buffer:
					std::vector< char > data = connection->recv_buffer;
					char type = data[0];
					if (type == 'C') {
            char *start = &data[1];
            //td::cout << (int)start[0] << " " << (int)start[1] << " " << (int)start[2] << " " << (int)start[3] << std::endl;
            for (auto it = level->movable_data.begin(); it != level->movable_data.end(); ++it){
              glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
              glm::vec3 offset = *pos - it->transform->position;
              it->update(offset);
              start += sizeof(glm::vec3);
            }
					} else if (type == 'R') {
						// std::cout << "Received reset" << std::endl;
						level->reset(true);
					} else if (type == 'P'){
						// std::cout << "Received P2 pos" << std::endl;
						char *start = &data[1];
						glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
						level->body_P2_transform->position = *pos;
					}
					else {
						//invalid data type
					}
					connection->recv_buffer.clear();
					//send to other connections:

			}
	  },
		0.0 //timeout (in seconds)
		);
  }
}
