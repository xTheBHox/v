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

PlayerTwoMode::PlayerTwoMode(std::string const &host, std::string const &port) {
  player_num = 2;
  pov.camera = level->cam_P2;
  pov.body = level->body_P2_transform;
  other_player = level->body_P1_transform;
  client.reset(new Client(host, port));
  connect = &client->connection;
}

void PlayerTwoMode::update_network() {

  update_send();
  client->poll([this](Connection *connection, Connection::Event evt) {
    //Read server state
    if (evt == Connection::OnRecv) {
      update_recv(connection->recv_buffer);
    } else if (evt == Connection::OnClose) {
      connect = nullptr;
    }
  }, 0.0);

}

void PlayerTwoMode::draw(glm::uvec2 const &drawable_size) {

  for (auto &stpt : level->standpoints) {
    stpt.resize_texture(drawable_size);
    stpt.update_texture(level);
  }

  float aspect = drawable_size.x / float(drawable_size.y);

  if (shift.progress > 0.0f) {
    Scene::OrthoCam *cf = shift.sc->stpt->cam;
    //float h = cf->scale;
    //float w = aspect * h;
    //float fp = cf->clip_far;
    float np = cf->clip_near;

    glm::mat4 proj = cf->make_projection();
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
