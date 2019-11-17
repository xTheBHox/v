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
			if (server) {
				server->poll([this](Connection *connection, Connection::Event evt){
				if (evt == Connection::OnRecv) {
						//extract and erase data from the connection's recv_buffer:
						std::vector< char > data = connection->recv_buffer;
						char type = data[0];
						if (type == 'C') {
							char *start = &data[1];
							size_t* len = reinterpret_cast<size_t*> (start);
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
						} else if (type == 'R') {
							std::cout << "Received reset" << std::endl;
							they_want_reset = true;
							reset_countdown = 0.01f;
						} else if (type == 'P'){
							char *start = &data[1];
							glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
							level->body_P2_transform->position = *pos;
						}

						connection->recv_buffer.clear();
						//send to other connections:
				}
	  		},0.0 //timeout (in seconds)
			  );
			}

		}else{
			if (client){
				client->poll([this](Connection *connection, Connection::Event evt){
  				//Read server state
					if (evt == Connection::OnRecv) {
						std::vector< char > data = connection->recv_buffer;
						char type = data[0];
						if (type == 'C') {
							char *start = &data[1];
							size_t* len = reinterpret_cast<size_t*> (start);
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
						}
						else if (type == 'R'){
							they_want_reset = true;
							reset_countdown = 0.01f;
							std::cout << "Received reset" << std::endl;
						} else if (type == 'P') {
							char *start = &data[1];
							glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
							level->body_P1_transform->position = *pos;
						}
						connection->recv_buffer.clear();
					}
  				}, 0.0);
			}
		}
	}
	//std::unique_ptr< Server > server = nullptr;
	//std::unique_ptr< Client > client = nullptr;

