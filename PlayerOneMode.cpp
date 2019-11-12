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
    server->poll([this](Connection *connection, Connection::Event evt){
			if (evt == Connection::OnRecv) {
					//extract and erase data from the connection's recv_buffer:
					std::vector< char > data = connection->recv_buffer;
					char type = data[0];
					if (type == 'C'){
            char *start = &data[1];
            //td::cout << (int)start[0] << " " << (int)start[1] << " " << (int)start[2] << " " << (int)start[3] << std::endl;
            for (auto it = level->standpoints.begin(); it != level->standpoints.end(); ++it){
              float* pos = reinterpret_cast<float*> (start);
              it->offset = *pos;
              it->update();
              start += sizeof(float);
            }
					} else if (type == 'R') {
						std::cout << "Received reset" << std::endl;
						level->reset(true);
					}else{
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
