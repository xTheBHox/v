#include "DrawLines.hpp"
#include "LitColorTextureProgram.hpp"
#include "Mesh.hpp"
#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "data_path.hpp"
#include "Sound.hpp"
#include "collide.hpp"
#include "gl_errors.hpp"

//for glm::pow(quaternion, float):
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <iostream>

Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const * {
	return new SpriteAtlas(data_path("trade-font"));
});


ServerMode::ServerMode(std::string const &port) : start(start_) {
	if (port != "") {
		server.reset(new Server(port));
	}
	//restart();
}

ServerMode::~ServerMode() {
}


bool ServerMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	return true;
}

void ServerMode::update(float elapsed) {
	if (server){
		while (true) {
			server.poll([](Connection *connection, Connection::Event evt){
				if (evt == Connection::OnRecv) {
					//extract and erase data from the connection's recv_buffer:
					std::vector< uint8_t > data = connection->recv_buffer;
					uint8_t type = data[0];
					uint8_t length = data[1];
					if (type == 'C'){

					}else{
						//invalid data type
					}
					connection->recv_buffer.clear();
					//send to other connections:

				}
			},
			1.0 //timeout (in seconds)
			);
		}
	}
}

void ServerMode::draw(glm::uvec2 const &drawable_size) {
	//--- actual drawing ---
}
