#include "PlayerTwoMode.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"
#include "collide.hpp"
#include "GameLevel.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>
#include <iomanip>

#define PI 3.1415926f

PlayerTwoMode::PlayerTwoMode( GameLevel *level_ , std::string const &host, std::string const &port)
  : PlayerOneMode(level_, "") {
  pov.camera = level->cam_P2;
  pov.body = level->body_P2_transform;
  SDL_SetRelativeMouseMode(SDL_TRUE);
  //client.reset(new Client(host, port));
}

PlayerTwoMode::~PlayerTwoMode(){
  SDL_SetRelativeMouseMode(SDL_FALSE);
  delete level; //TODO VVVBad. Remove when network
}

bool PlayerTwoMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	//----- leave to menu -----
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_M) {
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
  } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_LCTRL) {
    controls_shift.flat = true;
  } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_LCTRL) {
    controls_shift.flat = false;
  } else if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
    if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
      controls.left = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
      controls.right = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
      controls.forward = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
      controls.backward = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_LSHIFT) {
      controls.sprint = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_SPACE) {
      controls.jump = (evt.type == SDL_KEYDOWN);
    } else return false;
  } else if (evt.type == SDL_MOUSEMOTION) {
    if (shift.progress == 1.0f) {
      if (evt.motion.state & SDL_BUTTON_LMASK) {
        if (shift.moving) {
          float dy = evt.motion.yrel / float(window_size.y) * -2.0f;
          shift.moving->offset += controls_shift.drag_sensitivity * dy;
          shift.moving->update();
        }
      }
    } else { // Shift not in progress
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
    }

  } else if (evt.type == SDL_MOUSEBUTTONDOWN || evt.type == SDL_MOUSEBUTTONUP) {
    if (evt.button.button == SDL_BUTTON_LEFT) {
      controls.mouse_down = (evt.type == SDL_MOUSEBUTTONDOWN);
    }
  } else return false;

  return true;

}

void PlayerTwoMode::update(float elapsed) {

  if (pause) return;

  if (controls_shift.flat) {
    if (shift.progress == 0.0f) {
      shift.moving = level->movable_get( pov.camera->transform->make_local_to_world()[3] );
      if (shift.moving) {
        std::cout << "Got movable!\n" << std::endl;
        shift.progress = std::min(shift.progress + shift.speed * elapsed, 1.0f);
        SDL_SetRelativeMouseMode(SDL_FALSE);
      }
    } else {
      shift.progress = std::min(shift.progress + shift.speed * elapsed, 1.0f);
    }
  }
  else { // !controls_shift.flat
    if (shift.progress > 0.0f) {
      shift.progress = std::max(shift.progress - shift.speed * elapsed, 0.0f);
      if (shift.progress == 0.0f) {
        shift.moving = nullptr;
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    }
  }

  if (shift.progress > 0.0f) {

  } else { // Shift not in progress

    PlayerOneMode::update(elapsed);
    
  }

  if (client) {

    client->connection.send_buffer.emplace_back('C');
    for (auto it = level->movables.begin(); it != level->movables.end(); ++it){
        glm::vec3 position = (*it)->position;
        glm::quat rotation = (*it)->rotation;
        client->connection.send(position);
        client->connection.send(rotation);
    }

  	client->poll([](Connection *, Connection::Event evt){
  		//TODO: eventually, read server state

  	}, 0.0);
  	//if connection was closed,
  	if (!client->connection) {
  		Mode::set_current(nullptr);
  	}
  }
}

void print_mat4(glm::mat4 const &M) {
  std::cout << std::setprecision(4);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::cout << M[j][i] << "\t";
    }
    std::cout << std::endl;
  }
}

void print_vec4(glm::vec4 const &v) {
  std::cout << std::setprecision(4);
  for (int i = 0; i < 4; i++) {
    std::cout << v[i] << "\t";
    std::cout << std::endl;
  }
}

void PlayerTwoMode::draw(glm::uvec2 const &drawable_size) {

  float aspect = drawable_size.x / float(drawable_size.y);

  if (shift.progress > 0.0f) {
    Scene::OrthoCam *cf = shift.moving->cam_flat;
    float h = cf->scale;
    float w = aspect * h;

    glm::mat4 proj = glm::ortho(-w, w, -h, h, cf->near, cf->far);
    glm::mat4 w2l = shift.moving->cam_flat->transform->make_world_to_local();

    if (shift.progress < 1.0f) {
      float f = 1.0f - shift.progress;
      float f3 = f * f * f;
      float interp = 1.0f - f3 * f3;
      float near_p = (cf->near * interp) + (0.01f * (1.0f - interp));
      glm::mat4 reg_proj = glm::infinitePerspective(pov.camera->fovy, pov.camera->aspect, near_p);

      glm::mat4 reg_w2l = pov.camera->transform->make_world_to_local();

      w2l = (w2l * shift.progress) + (reg_w2l * (1.0f - shift.progress));
      proj = (proj * interp) + (reg_proj * (1.0f - interp));
    }
    level->draw(*pov.camera, proj * w2l);

  } else {
    level->draw(*pov.camera);
  }

}
