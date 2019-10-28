#include "PlayerOneMode.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

using namespace std;
#define PI 3.1415926f

PlayerOneMode::PlayerOneMode( GameLevel *level_ , std::string const &port) : level( level_ ){
  pov.camera = &( level->cameras.front() );
  SDL_SetRelativeMouseMode(SDL_TRUE);
  if (port != "") {
		server.reset(new Server(port));
	}
}

PlayerOneMode::~PlayerOneMode(){
  SDL_SetRelativeMouseMode(SDL_FALSE);
  delete level; //TODO VVVBad. Remove when network
}

bool PlayerOneMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//----- leave to menu -----
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_m) {
		Mode::set_current(demo_menu);
		return true;
	}
  if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
    pause = !pause;
    if (pause) SDL_SetRelativeMouseMode(SDL_FALSE);
    else SDL_SetRelativeMouseMode(SDL_TRUE);
    return true;
  }
  if (pause) return false;

  if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_1) {
    pov.camera = &( level->cameras.front() );
  } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_2) {
    pov.camera = &( level->cameras.back() );
  } else if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
    if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
      controls.left = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
      controls.right = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
      controls.forward = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
      controls.backward = (evt.type == SDL_KEYDOWN);
    } else return false;
  } else if (evt.type == SDL_MOUSEMOTION) {
    //based on trackball camera control from ShowMeshesMode:
    //figure out the motion (as a fraction of a normalized [-a,a]x[-1,1] window):
    glm::vec2 delta;
    delta.x = evt.motion.xrel / float(window_size.x) * 2.0f;
    delta.x *= float(window_size.y) / float(window_size.x);
    delta.y = evt.motion.yrel / float(window_size.y) * -2.0f;

    delta *= controls.mouse_sensitivity;

    pov.azimuth -= delta.x;
    pov.elevation -= delta.y;

    // Normalize to [-pi, pi)
    pov.azimuth /= 2.0f * PI;
    pov.azimuth -= std::round(pov.azimuth);
    pov.azimuth *= 2.0f * PI;

    // Clamp to [-89deg, 89deg]
    pov.elevation = std::max(-89.0f / 180.0f * PI, pov.elevation);
    pov.elevation = std::min( 89.0f / 180.0f * PI, pov.elevation);

  } else if (evt.type == SDL_MOUSEBUTTONDOWN || evt.type == SDL_MOUSEBUTTONUP) {
    if (evt.button.button == SDL_BUTTON_LEFT) {
      controls.mouse_down = (evt.type == SDL_MOUSEBUTTONDOWN);
    }
  }
  else return false;

  return true;

}

void PlayerOneMode::update(float elapsed) {
  if (pause) return;
  float pl_ca = std::cos(pov.azimuth);
  float pl_sa = std::sin(pov.azimuth);

  // Update player velocity
  {
    glm::vec3 pl_dvel = glm::vec3(0.0f);
    if (controls.left) pl_dvel.x -= 1.0f;
    if (controls.right) pl_dvel.x += 1.0f;
    if (controls.backward) pl_dvel.y -= 1.0f;
    if (controls.forward) pl_dvel.y += 1.0f;

    if (pl_dvel != glm::vec3(0.0f)) {
      pl_dvel = glm::normalize(pl_dvel);
      pl_dvel =
        glm::vec3(pl_ca, pl_sa, 0.0f) * pl_dvel.x +
        glm::vec3(-pl_sa, pl_ca, 0.0f) * pl_dvel.y;
      float pl_accel = 0.1f;
      pov.vel += pl_accel * pl_dvel;
    }
    pov.vel *= std::pow(0.5f, elapsed / 0.05f); // friction
  }

  // Update player position
  pov.camera->transform->position += pov.vel;

  // Update camera rotation
  {
    pov.camera->transform->rotation = glm::angleAxis(
      pov.azimuth,
      glm::vec3( 0.0f, 0.0f, 1.0f )
    ) * glm::angleAxis(
      -pov.elevation + 0.5f * PI,
      glm::vec3( 1.0f, 0.0f, 0.0f )
    );
  }

  if (server){
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
					char type = (data[0]);
					if (type == 'C'){
            auto start = (&data[1]);
            for (auto it = level->movables.begin(); it != level->movables.end(); ++it){
              glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
              glm::quat* rot = reinterpret_cast<glm::quat*> (start + sizeof(glm::vec3));
              (*it)->position = *pos;
              (*it)->rotation = *rot;
              start += sizeof(glm::vec3) + sizeof(glm::quat);
            }
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

void PlayerOneMode::draw(glm::uvec2 const &drawable_size) {

  pov.camera->aspect = drawable_size.x / float(drawable_size.y);

  level->draw( *pov.camera );

}
