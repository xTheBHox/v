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
			if ((reset_countdown == 0.0f || they_want_reset) && we_want_reset) {
				server->connections.begin()->send('R');
				reset_countdown = 0.01f;
				std::cout << "Requested reset" << std::endl;
      		}

			server->connections.begin()->send('P');
			//std::cout << "Sent P1 pos" << std::endl;
			auto pos = level->body_P1_transform->position;
			//std::cout << pos.x <<" " << pos.y << " " <<  pos.z << std::endl;
			server->connections.begin()->send(pos);

		}
    server->poll([this](Connection *connection, Connection::Event evt){
			if (evt == Connection::OnRecv) {
					//extract and erase data from the connection's recv_buffer:
					std::vector< char > data = connection->recv_buffer;
					while (data.size() >= 1){
						char type = data[0];
						char *start = &data[1];
						if (type == 'C') {
							//char *start = &data[1];
							size_t* len = reinterpret_cast<size_t*> (start);
							if (data.size() < *len * (sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(size_t)) + sizeof(size_t) + 1){
								break;
							}
							//std::cout << *len << std::endl;
							start += sizeof(size_t);
							for (size_t i = 0 ; i < *len; i++){
								size_t* index = reinterpret_cast<size_t*> (start);
								start += sizeof(size_t);
								glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
								glm::vec3 offset = *pos - (&(level->movable_data[*index]))->transform->position;
								start += sizeof(glm::vec3);
								glm::vec4* color = reinterpret_cast<glm::vec4*> (start);
								start += sizeof(glm::vec4);
								(&(level->movable_data[*index]))->update(offset);
								(&(level->movable_data[*index]))->color = *color;
							}
						data.erase(data.begin(), data.begin() + *len * (sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(size_t)) + sizeof(size_t) + 1);
					} else if (type == 'R') {
						std::cout << "Received reset" << std::endl;
            			they_want_reset = true;
            			reset_countdown = 0.01f;
						data.erase(data.begin(),data.begin()+1);
					} else if (type == 'P'){
						// std::cout << "Received P2 pos" << std::endl;
						//char *start = &data[1];
						if (data.size() < sizeof(glm::vec3) + 1){
							break;
						}
						glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
						level->body_P2_transform->position = *pos;
						data.erase(data.begin(),data.begin() + sizeof(glm::vec3) + 1);
					}
					else {
						//invalid data type
					}
					//connection->recv_buffer.clear();

					}
					
					//send to other connections:

			}
	  },
		0.0 //timeout (in seconds)
		);
  }
}
