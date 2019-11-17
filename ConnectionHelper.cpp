#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "ConnectionHelper.hpp"

#include <iostream>

	ConnectionHelper::ConnectionHelper(bool isServer_, GameLevel *level_, std::string const &host, std::string const &port){
		isServer = isServer_;
		level = level_;
		if (isServer){
			if (port != "") {
				server.reset(new Server(port));
			}
		}else{
			client.reset(new Client(host, port));
		}
	}

	void ConnectionHelper::send_reset(float reset_countdown, bool they_want_reset, bool we_want_reset){
		if (isServer){
			if (server){
				if (server->connections.size() != 0){
					if ((reset_countdown == 0.0f || they_want_reset) && we_want_reset) {
						server->connections.begin()->send('R');
						reset_countdown = 0.01f;
						std::cout << "Requested reset" << std::endl;
					}
				}
			}
			
		}else{
			if (client){
				if ((reset_countdown == 0.0f || they_want_reset) && we_want_reset) {
				client->connection.send('R');
				std::cout << "Requested reset" << std::endl;
				//reset_countdown = 0.01f;
				}
			}
		}
	}

	void ConnectionHelper::send_player_pos(){
		if (isServer){
			if (server){
				server->connections.begin()->send('P');
				auto pos = level->body_P1_transform->position;
				server->connections.begin()->send(pos);
			}
			
		}else{
			if (client){
				client->connection.send('P');
    			auto pos = level->body_P2_transform->position;
    			client->connection.send(pos);
			}
		}

	}

	void ConnectionHelper::send_movable(std::list<size_t> currently_moving){
		if (isServer){
			if (server){
				server->connections.begin()->send('C');
				size_t len = currently_moving.size();
				//send number of moved objects
				server->connections.begin()->send(len);
				for (auto it = currently_moving.begin(); it != currently_moving.end(); ++it){
					//send index
					server->connections.begin()->send(*it);
					//send pos
					glm::vec3 pos = level->movable_data[*it].transform->position;
					server->connections.begin()->send(pos);
					//send color
					glm::vec4 color = level->movable_data[*it].color;
					server->connections.begin()->send(color);
				}
			}
			
		}else{
			if (client){
				client->connection.send('C');
      			size_t len = currently_moving.size();
				//send number of moved objects
				client->connection.send(len);
				for (auto it = currently_moving.begin(); it != currently_moving.end(); ++it){
					//send index
					client->connection.send(*it);
					//send pos
					glm::vec3 pos = level->movable_data[*it].transform->position;
					client->connection.send(pos);
					//send color
					glm::vec4 color = level->movable_data[*it].color;
					client->connection.send(color);
				}
			}
			
		}

	}
	void ConnectionHelper::receive_data(){
		if (isServer){

		}else{

		}
	}
	//std::unique_ptr< Server > server = nullptr;
	//std::unique_ptr< Client > client = nullptr;

