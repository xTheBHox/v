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


extern void print_mat4(glm::mat4 const &M);
extern void print_vec4(glm::vec4 const &v);
extern void print_vec3(glm::vec3 const &v);

PlayerTwoMode::PlayerTwoMode(GameLevel *level_ , std::string const &host, std::string const &port)
  : PlayerMode(level_) {
  player_num = 2;
  pov.camera = level->cam_P2;
  pov.body = level->body_P2_transform;
  client.reset(new Client(host, port));
}

bool PlayerTwoMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (handle_ui(evt, window_size)) return true;

  if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_LSHIFT) {
    controls_shift.flat = true;
  } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_LSHIFT) {
    controls_shift.flat = false;
  } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_1
    && shift.progress == 1.0f) {
    std::cout << "1" << std::endl;
    GameLevel::Standpoint::MovePosition &mp = shift.sc->stpt->move_pos[0];
    shift.sc->stpt->movable->set_target_pos(mp.pos, mp.color);
    currently_moving.emplace_back(shift.sc->stpt->movable->index);
  } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_2
    && shift.progress == 1.0f) {
    std::cout << "2" << std::endl;
    GameLevel::Standpoint::MovePosition &mp = shift.sc->stpt->move_pos[1];
    shift.sc->stpt->movable->set_target_pos(mp.pos, mp.color);
    currently_moving.emplace_back(shift.sc->stpt->movable->index);
  } else if (evt.type == SDL_MOUSEMOTION && shift.progress > 0.0f) {
    // ignore
  } else {
    return PlayerMode::handle_event(evt, window_size);
  }

  return true;

}

void PlayerTwoMode::update(float elapsed) {

  if (controls_shift.flat) {
    if (shift.progress == 0.0f) {
      shift.sc = level->screen_get(pov.camera->transform);
      if (shift.sc) {
        std::cout << "Got screen!\n" << std::endl;
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
        shift.sc = nullptr;
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    }
  }

  { // Update movables
    std::list< size_t >::iterator mi_ptr = currently_moving.begin();
    while (mi_ptr != currently_moving.end()) {
      GameLevel::Movable &m = level->movable_data[*mi_ptr];
      glm::vec3 diff = m.target_pos - m.transform->position;
      if (diff == glm::vec3(0.0f)) {
        currently_moving.erase(mi_ptr++);
        continue;
      }
      float dpos = elapsed * m.vel;
      if (glm::dot(diff, diff) < dpos * dpos) {
        m.transform->position = m.target_pos;
      } else {
        diff = glm::normalize(diff);
        m.transform->position += diff * dpos;
      }
      // need network update later
      mi_ptr++;
    }
  }

  if (shift.progress > 0.0f) {

  } else { // Shift not in progress

    PlayerMode::update(elapsed);

  }

  if (client) {

    if ((reset_countdown == 0.0f || they_want_reset) && we_want_reset) {
      client->connection.send('R');
      std::cout << "Requested reset" << std::endl;
      reset_countdown = 0.01f;
    }

    if (!currently_moving.empty()) {
      client->connection.send('C');
      for (auto it = level->movable_data.begin(); it != level->movable_data.end(); ++it){
        glm::vec3 pos = it->transform->position;
        client->connection.send(pos);
        glm::vec4 color = it->color;
        client->connection.send(color);
      }
    }

    //syncing player pos
    client->connection.send('P');
    auto pos = level->body_P2_transform->position;
    client->connection.send(pos);

  	client->poll([this](Connection *connection, Connection::Event evt){
  		//Read server state
      if (evt == Connection::OnRecv) {
        std::vector< char > data = connection->recv_buffer;
          char type = data[0];
          if (type == 'R'){
            they_want_reset = true;
            reset_countdown = 0.01f;
            std::cout << "Received reset" << std::endl;
          } else if (type == 'P') {
            //std::cout << "Received P1 pos" << std::endl;
            char *start = &data[1];
					  glm::vec3* pos = reinterpret_cast<glm::vec3*> (start);
					  //std::cout << pos->x <<" " << pos->y << " " <<  pos->z << std::endl;
            level->body_P1_transform->position = *pos;
          }
          connection->recv_buffer.clear();
      }
  	}, 0.0);
  	//if connection was closed,
  	if (!client->connection) {
  		Mode::set_current(nullptr);
  	}
  }

  if (we_want_reset || they_want_reset) {
    reset_countdown += elapsed;
    if (reset_countdown > 15.0f) {
      std::cout << "Reset timed out" << std::endl;
      we_want_reset = false;
      they_want_reset = false;
      reset_countdown = 0.0f;
    }
  }
}

void PlayerTwoMode::draw(glm::uvec2 const &drawable_size) {

  for (auto &stpt : level->standpoints) {
    stpt.resize_texture(drawable_size);
    stpt.update_texture(level);
  }

  float aspect = drawable_size.x / float(drawable_size.y);

  if (shift.progress > 0.0f) {
    Scene::OrthoCam *cf = shift.sc->stpt->cam;
    float h = cf->scale;
    float w = aspect * h;
    float fp = cf->clip_far;
    float np = cf->clip_near;

    glm::mat4 proj = glm::ortho(-w, w, -h, h, np ,fp);
    glm::mat4 w2l = shift.sc->stpt->cam->transform->make_world_to_local();

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
