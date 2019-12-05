#include "PlayerMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"
#include "data_path.hpp"
#include <iostream>
#include <algorithm>

#define PI 3.1415926f

extern void print_mat4(glm::mat4 const &M);
extern void print_vec4(glm::vec4 const &v);
extern void print_vec3(glm::vec3 const &v);

PlayerMode::PlayerMode(uint32_t level_num_, uint32_t player_num_)
: player_num(player_num_) {
  level_change(level_num_);
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

PlayerMode::~PlayerMode(){
  SDL_SetRelativeMouseMode(SDL_FALSE);
  delete level;
}

bool PlayerMode::handle_ui(SDL_Event const &evt, glm::uvec2 const &window_size) {
  if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_M) {
        MenuMode::set_current(nullptr);
    } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
    pause = !pause;
    if (pause) SDL_SetRelativeMouseMode(SDL_FALSE);
    else SDL_SetRelativeMouseMode(SDL_TRUE);
  } else return false;

  return true;
}

void PlayerMode::level_change(uint32_t level_num_) {
  std::cout << "Deleting old level" << std::endl;
  if (level) delete level;

  level_num = level_num_;
  std::string level_str ("level");
  level_str = level_str + std::to_string(level_num);
  std::cout << "Loading new level" << std::endl;
  level = new GameLevel(data_path(level_str));
  player_set();
  level_reset();
}

void PlayerMode::level_reset() {

  std::cout << "Resetting level" << std::endl;
  level->reset();

  won = false;
  to_next_level = 0.0f;
  lost = false;
  pause = false;
  we_want_reset = false;
  they_want_reset = false;
  reset_countdown = 0.0f;

  std::cout << "Clearing old moving data" << std::endl;

  currently_moving.clear();
  pov.on_movable = nullptr;

}

void PlayerMode::player_set() {
  assert((player_num == 1 || player_num == 2) && "Invalid player number");
  if (player_num == 1) {
    pov.camera = level->cam_P1;
    pov.body = level->body_P1_transform;
    other_player = level->body_P2_transform;
  } else if (player_num == 2) {
    pov.camera = level->cam_P2;
    pov.body = level->body_P2_transform;
    other_player = level->body_P1_transform;
  }
}

void PlayerMode::handle_reset() {
  //Needs inherited class to implement!
}

bool PlayerMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

  if (handle_ui(evt, window_size)) return true;

  if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
    if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
      controls.left = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
      controls.right = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
      controls.forward = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
      controls.backward = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_LCTRL) {
      controls.sprint = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_SPACE) {
      controls.jump = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_LSHIFT) {
      controls_shift.flat = (evt.type == SDL_KEYDOWN);
    } else return false;
  } else if (evt.type == SDL_MOUSEMOTION) {
    if (shift.progress == 0.0f) {
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
      pov.elevation = std::max(-85.0f / 180.0f * PI, pov.elevation);
      pov.elevation = std::min(60.0f / 180.0f * PI, pov.elevation);
    }
  } else if (evt.type == SDL_MOUSEBUTTONDOWN || evt.type == SDL_MOUSEBUTTONUP) {
    if (evt.button.button == SDL_BUTTON_LEFT) {
      controls.mouse_down = (evt.type == SDL_MOUSEBUTTONDOWN);
    }
  } else return false;

  return true;

}

void PlayerMode::update(float elapsed) {
  if (pause) return;
  if (!won && level->detect_win()) {
    won = true;
    to_next_level = 0.0f;
  }
  else if (level->detect_lose()) {
    lost = true;
  }

  update_reset_timer(elapsed);
  update_shift(elapsed);
  if (shift.progress == 0.0f) {
    update_me_move(elapsed);
  }
  update_movables_move(elapsed);

  update_network();

}

void PlayerMode::update_shift(float elapsed) {

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
}

void PlayerMode::update_reset_timer(float elapsed) {

  if (we_want_reset && they_want_reset) {
    level_reset();
    std::cout << "Reset!" << std::endl;
  }
  else if (we_want_reset || they_want_reset) {
    reset_countdown += elapsed;
    if (reset_countdown > 15.0f) {
      std::cout << "Reset timed out" << std::endl;
      we_want_reset = false;
      they_want_reset = false;
      reset_countdown = 0.0f;
    }
  }
}

void PlayerMode::update_movables_move(float elapsed) {
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
      m.update(diff);
    } else {
      diff = dpos * glm::normalize(diff);
      m.update(diff);
    }
    // need network update later
    mi_ptr++;
  }
}

void PlayerMode::update_me_move(float elapsed) {

  float pl_cosazi = std::cos(pov.azimuth);
  float pl_sinazi = std::sin(pov.azimuth);

  glm::vec3 pl_pos = pov.body->position;
  glm::vec3 pl_vel = pov.vel;

  { // Gravitational accel
    pl_vel += elapsed * gravity;
  }

  { // Jumping force
    if (controls.jump) {
      if (!pov.in_air) {
        pov.in_air = true;
        pl_vel += -0.3f * gravity; // jumping force, constant
      }
      controls.jump = false;
    }
  }

  if (!pov.in_air) { // Movement forces
    glm::vec3 pl_vel_move = glm::vec3(0.0f);
    if (controls.left) pl_vel_move.x -= 1.0f;
    if (controls.right) pl_vel_move.x += 1.0f;
    if (controls.backward) pl_vel_move.y -= 1.0f;
    if (controls.forward) pl_vel_move.y += 1.0f;

    if (pl_vel_move != glm::vec3(0.0f)) {
      pl_vel_move = glm::normalize(pl_vel_move);
      pl_vel_move =
        glm::vec3(pl_cosazi, pl_sinazi, 0.0f) * pl_vel_move.x +
        glm::vec3(-pl_sinazi, pl_cosazi, 0.0f) * pl_vel_move.y;
      pl_vel_move *= 15.0f; // player movement velocity magnitude, constant
      if (controls.sprint) pl_vel_move *= 2.0f; // sprint force multiplier, constant
      //pl_vel += pl_vel_move;
    }
    pl_vel.x = glm::mix(pl_vel.x, pl_vel_move.x, std::pow(0.5f, elapsed / 0.01f));
    pl_vel.y = glm::mix(pl_vel.y, pl_vel_move.y, std::pow(0.5f, elapsed / 0.01f));
    // pov.vel *= std::pow(0.5f, elapsed / 0.01f); // friction
  }

  // Compute collisions for normal contact force
  {
    float remain = elapsed;

    if (pov.on_movable) {
      pov.on_movable->remove_player(player_num);
      pov.on_movable = nullptr;
    }

    // Iterate for multiple bounces. Stop at a certain number.
    for (int32_t iter = 0; iter < 5; ++iter) {
      if (remain == 0.0f) break;

      float sphere_radius = 1.0f; // player sphere is radius-1
      glm::vec3 sphere_from = pl_pos;
      glm::vec3 sphere_to = pl_pos + pl_vel * remain;

      bool collided = false;
      float collision_t = 1.0f;
      glm::vec3 collision_at = glm::vec3(0.0f);
      glm::vec3 collision_out = glm::vec3(0.0f);
      bool is_surface_collision = false;

      glm::vec3 sphere_min = glm::min(sphere_from, sphere_to) - glm::vec3(sphere_radius);
      glm::vec3 sphere_max = glm::max(sphere_from, sphere_to) + glm::vec3(sphere_radius);
      for (auto const &collider : level->mesh_colliders) {
        glm::mat4x3 collider_to_world = collider.transform->make_local_to_world();

        { // Early discard:
          // check if AABB of collider overlaps AABB of swept sphere:

          // (1) compute bounding box of collider in world space:
          glm::vec3 local_center = 0.5f * (collider.mesh->max + collider.mesh->min);
          glm::vec3 local_radius = 0.5f * (collider.mesh->max - collider.mesh->min);

          glm::vec3 world_min = collider_to_world * glm::vec4(local_center, 1.0f)
            - glm::abs(local_radius.x * collider_to_world[0])
            - glm::abs(local_radius.y * collider_to_world[1])
            - glm::abs(local_radius.z * collider_to_world[2]);
          glm::vec3 world_max = collider_to_world * glm::vec4(local_center, 1.0f)
            + glm::abs(local_radius.x * collider_to_world[0])
            + glm::abs(local_radius.y * collider_to_world[1])
            + glm::abs(local_radius.z * collider_to_world[2]);

          // (2) check if bounding boxes overlap:
          bool can_skip = !collide_AABB_vs_AABB(sphere_min, sphere_max, world_min, world_max);

          // draw to indicate result of check:
          // if (iter == 0 && DEBUG_draw_lines && DEBUG_show_geometry) {
          //   DEBUG_draw_lines->draw_box(glm::mat4x3(
          //     0.5f * (world_max.x - world_min.x), 0.0f, 0.0f,
          //     0.0f, 0.5f * (world_max.y - world_min.y), 0.0f,
          //     0.0f, 0.0f, 0.5f * (world_max.z - world_min.z),
          //     0.5f * (world_max.x+world_min.x), 0.5f * (world_max.y+world_min.y), 0.5f * (world_max.z+world_min.z)
          //   ), (can_skip ? glm::u8vec4(0x88, 0x88, 0x88, 0xff) : glm::u8vec4(0x88, 0x88, 0x00, 0xff) ) );
          // }

          if (can_skip) {
            // AABBs do not overlap; skip detailed check:
            continue;
          }
        }

        // Full (all triangles) test:

        assert(collider.mesh->type == GL_TRIANGLES); //only have code for TRIANGLES not other primitive types

        for (GLuint v = 0; v + 2 < collider.mesh->count; v += 3) {
          // get vertex positions from associated positions buffer:
          // (and transform to world space)
          glm::vec3 a = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+0], 1.0f);
          glm::vec3 b = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+1], 1.0f);
          glm::vec3 c = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+2], 1.0f);
          // check triangle:
          bool did_collide = collide_swept_sphere_vs_triangle(
            sphere_from, sphere_to, sphere_radius,
            a, b, c,
            &collision_t, &collision_at, &collision_out, &is_surface_collision
          );

          if (did_collide) {
            collided = true;
            if (collider.movable_index >= 0) {
              pov.on_movable = &level->movable_data[collider.movable_index];
              pov.on_movable->add_player(pov.body, player_num);
              //std::cout << "Got player!" << std::endl;
            }
          }

          //draw to indicate result of check:
          // if (iter == 0 && DEBUG_draw_lines) {
          //   glm::u8vec4 color = (did_collide ? glm::u8vec4(0x88, 0x00, 0x00, 0xff) : glm::u8vec4(0x88, 0x88, 0x00, 0xff));
          //   if (DEBUG_show_geometry || (did_collide && DEBUG_show_collision)) {
          //     DEBUG_draw_lines->draw(a,b,color);
          //     DEBUG_draw_lines->draw(b,c,color);
          //     DEBUG_draw_lines->draw(c,a,color);
          //   }
          //   //do a bit more to highlight colliding triangles (otherwise edges can be over-drawn by non-colliding triangles):
          //   if (did_collide && DEBUG_show_collision) {
          //     glm::vec3 m = (a + b + c) / 3.0f;
          //     DEBUG_draw_lines->draw(glm::mix(a,m,0.1f),glm::mix(b,m,0.1f),color);
          //     DEBUG_draw_lines->draw(glm::mix(b,m,0.1f),glm::mix(c,m,0.1f),color);
          //     DEBUG_draw_lines->draw(glm::mix(c,m,0.1f),glm::mix(a,m,0.1f),color);
          //   }
          // }
        }
      }

      if (!collided) {
        pl_pos = sphere_to;
        remain = 0.0f;
        break;
      } else {

        // allow jumping if collision is with triangle with normal in positive z direction
        // glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
        if (collision_out.z > 0.0f) {
          pov.in_air = false;
        }
        // Get intermediate position
        pl_pos = glm::mix(sphere_from, sphere_to, collision_t);
        // Get remaining elapsed time
        remain = (1.0f - collision_t) * remain;
        // Get normal vector
        float d = glm::dot(pl_vel, collision_out); // collision_out already normalized
        // Adjust for surface friction
        if (d < 0.0f) {
          float friction_coeff = std::pow(0.5f, remain / 0.01f);
          glm::vec3 forward = pl_vel - d * collision_out;
          float forward_magnitude_2 = glm::dot(forward, forward);
          float backward_magnitude = friction_coeff * d;
          if (forward_magnitude_2 < backward_magnitude * backward_magnitude) {
            // Friction dominates
            pl_vel -= forward;
          } else {
            pl_vel += backward_magnitude * glm::normalize(forward);
          }
          pl_vel -= 1.001f * d * collision_out;
        }

        // if (DEBUG_draw_lines && DEBUG_show_collision) {
        //   //draw a little gadget at the collision point:
        //   glm::vec3 p1;
        //   if (std::abs(collision_out.x) <= std::abs(collision_out.y) && std::abs(collision_out.x) <= std::abs(collision_out.z)) {
        //     p1 = glm::vec3(1.0f, 0.0f, 0.0f);
        //   } else if (std::abs(collision_out.y) <= std::abs(collision_out.z)) {
        //     p1 = glm::vec3(0.0f, 1.0f, 0.0f);
        //   } else {
        //     p1 = glm::vec3(0.0f, 0.0f, 1.0f);
        //   }
        //   p1 = glm::normalize(p1 - glm::dot(p1, collision_out)*collision_out);
        //   glm::vec3 p2 = glm::cross(collision_out, p1);
        //
        //   float r = 0.25f;
        //   glm::u8vec4 color = glm::u8vec4(0xff, 0x00, 0x00, 0xff);
        //   DEBUG_draw_lines->draw(collision_at + r*p1, collision_at + r*p2, color);
        //   DEBUG_draw_lines->draw(collision_at + r*p2, collision_at - r*p1, color);
        //   DEBUG_draw_lines->draw(collision_at - r*p1, collision_at - r*p2, color);
        //   DEBUG_draw_lines->draw(collision_at - r*p2, collision_at + r*p1, color);
        //   DEBUG_draw_lines->draw(collision_at, collision_at + collision_out, color);
        // }

        // if (iter == 0) {
        //   std::cout << collision_out.x << ", " << collision_out.y << ", " << collision_out.z
        //   << " / " << velocity.x << ", " << velocity.y << ", " << velocity.z << std::endl; //DEBUG
        // }
      }
    }
    pov.body->position = pl_pos + pl_vel * remain;
    pov.vel = pl_vel;
  } // end collision compute

    {
    // body rotation update:
    glm::quat rot_h = glm::angleAxis(pov.azimuth, glm::vec3(0.0f, 0.0f, 1.0f));
    pov.body->rotation = rot_h;
    //camera update:
    pov.camera->transform->rotation =
      glm::angleAxis(-pov.elevation + 0.5f * PI, glm::vec3(1.0f, 0.0f, 0.0f));
    }

}

// Returns -1 for invalid message, 1 for message incomplete, 0 for message read
int PlayerMode::update_recv_msg(char msg_type, char *buf, size_t buf_len, size_t *used_len) {

  size_t msg_size;
  size_t msg_unit_size;

  if (msg_type == 'R') {
    msg_size = 0;
    //if (buf_len < msg_size) return 1;

    they_want_reset = true;
    reset_countdown = 0.01f;
    std::cout << "Received reset" << std::endl;

  } else if (msg_type == 'P') {
    msg_size = sizeof(glm::vec3) + sizeof(glm::quat);
    if (buf_len < msg_size) return 1;

    glm::vec3* pos = reinterpret_cast<glm::vec3*> (buf);
    buf += sizeof(glm::vec3);

    glm::quat* rot = reinterpret_cast<glm::quat*> (buf);
    buf += sizeof(glm::quat);

    other_player->position = *pos;
    other_player->rotation = *rot;

  } else if (msg_type == 'C') {
    play_moving_sound = 1;
    msg_unit_size = sizeof(size_t) + sizeof(glm::vec3) + sizeof(glm::vec4);
    size_t* len = reinterpret_cast<size_t*> (buf);
    msg_size = sizeof(size_t) + *len * msg_unit_size;
    if (buf_len < msg_size) return 1;

    //std::cout << *len << std::endl;
    buf += sizeof(size_t);
    for (size_t i = 0 ; i < *len; i++) {

      size_t* index = reinterpret_cast<size_t*> (buf);
      buf += sizeof(size_t);

      glm::vec3* pos = reinterpret_cast<glm::vec3*> (buf);
      buf += sizeof(glm::vec3);

      glm::vec4* color = reinterpret_cast<glm::vec4*> (buf);
      buf += sizeof(glm::vec4);

      glm::vec3 offset = *pos - (&(level->movable_data[*index]))->transform->position;
      level->movable_data[*index].update(offset);
      level->movable_data[*index].color = *color;

    }

  } else {
    std::cout << "ERROR: Invalid message type!" << std::endl;
    return -1;
  }

  // Return the number of bytes used up
  if (used_len) *used_len = msg_size;
  return 0;

}

void PlayerMode::update_recv(std::vector< char >& data) {
  while (!data.empty()) {
    char msg_type = data[0];
    size_t msg_size;
    int status = update_recv_msg(msg_type, &data[1], data.size() - 1, &msg_size);
    if (status == 0) {
      data.erase(data.begin(),data.begin() + 1 + msg_size);
    } else if (status == 1) {
      break;
    } else if (status == -1) {
      std::cout << "Resetting message buffer..." << std::endl;
      data.clear();
    }

  }
}

void PlayerMode::update_send() {

  if (!connect) return;

  if (!currently_moving.empty()) {
    connect->send('C');
    size_t len = currently_moving.size();
    //send number of moved objects
    connect->send(len);
    for (auto it = currently_moving.begin(); it != currently_moving.end(); ++it) {
      //send index
      connect->send(*it);
      //send pos
      glm::vec3 pos = level->movable_data[*it].transform->position;
      connect->send(pos);
      //send color
      glm::vec4 color = level->movable_data[*it].color;
      connect->send(color);
    }
  }

  //syncing player pos
  connect->send('P');
  auto pos = pov.body->position;
  connect->send(pos);
  auto rot = pov.body->rotation;
  connect->send(rot);

}

void PlayerMode::update_network() {
  update_send();
  // Can't receive. Needs inherited class to implement!
}

void PlayerMode::draw(glm::uvec2 const &drawable_size) {

  float aspect = drawable_size.x / float(drawable_size.y);

  if (shift.progress > 0.0f) {
    Scene::OrthoCam *cf = shift.sc->stpt->cam;
    cf->aspect = aspect;
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
    pov.camera->aspect = aspect;
    glm::vec4 eye = pov.camera->transform->make_local_to_world()[3];
    glm::mat4 world_to_clip = pov.camera->make_projection() * pov.camera->transform->make_world_to_local();
    level->draw(drawable_size, eye, world_to_clip);
  }

}
