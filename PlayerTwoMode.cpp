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
#include <iomanip>

#define PI 3.1415926f

PlayerTwoMode::PlayerTwoMode(GameLevel *level_ , std::string const &host, std::string const &port)
  : PlayerMode(level_) {
  pov.camera = level->cam_P2;
  pov.body = level->body_P2_transform;
  //client.reset(new Client(host, port));
}

bool PlayerTwoMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (handle_ui(evt, window_size)) return true;

  if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_LCTRL) {
    controls_shift.flat = true;
  } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_LCTRL) {
    controls_shift.flat = false;
  } else if (
    evt.type == SDL_MOUSEMOTION
    && shift.progress == 1.0f
    && evt.motion.state & SDL_BUTTON_LMASK
  ) {
    float dy = evt.motion.yrel / float(window_size.y) * -2.0f;
    shift.moving->offset += controls_shift.drag_sensitivity * dy;
    shift.moving->update();
    //TEMP
    if (client) {

      client->connection.send('C');
      for (auto it = level->movable_data.begin(); it != level->movable_data.end(); ++it){
          float offset = it->offset;
          client->connection.send(offset);
      }

    	client->poll([](Connection *, Connection::Event evt){
    		//TODO: eventually, read server state

    	}, 0.0);
    	//if connection was closed,
    	if (!client->connection) {
    		Mode::set_current(nullptr);
    	}
    }
    //END TEMP
  } else {
    return PlayerMode::handle_event(evt, window_size);
  }

  return true;

}

void PlayerTwoMode::update(float elapsed) {

  if (pause) return;

  if (controls_shift.flat) {
    if (shift.progress == 0.0f) {
      shift.moving = level->movable_get(pov.camera->transform);
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

    PlayerMode::update(elapsed);

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
    float fp = cf->clip_far;
    float np = cf->clip_near;

    glm::mat4 proj = glm::ortho(-w, w, -h, h, np ,fp);
    glm::mat4 w2l = shift.moving->cam_flat->transform->make_world_to_local();

    if (shift.progress < 1.0f) {

      float f = 1.0f - shift.progress;
      float f3 = f * f * f;
      float f6 = f3 * f3;
      float near_p = (np * (1.0f - f6)) + (0.01f * f6);
      glm::mat4 reg_proj = glm::infinitePerspective(pov.camera->fovy, pov.camera->aspect, near_p);

      glm::mat4 reg_w2l = pov.camera->transform->make_world_to_local();

      w2l = (w2l * (1.0f - f3)) + (reg_w2l * f3);
      proj = (proj * (1.0f - f6)) + (reg_proj * f6);
    }
    level->draw(drawable_size, w2l[3], proj * w2l);

  } else {
    PlayerMode::draw(drawable_size);
  }

}
