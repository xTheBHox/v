#include "SinglePlayerMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"

#include <iostream>
#include <algorithm>

SinglePlayerMode::SinglePlayerMode(uint32_t level_num_) : PlayerMode(level_num_, 1) {
  player_set();
}

void SinglePlayerMode::player_set() {
  if (player_num == 1) {
    pov.camera = level->cam_P1;
    pov.body = level->body_P1_transform;
    other_pov.camera = level->cam_P2;
    other_pov.body = level->body_P2_transform;
  } else if (player_num == 2) {
    pov.camera = level->cam_P2;
    pov.body = level->body_P2_transform;
    other_pov.camera = level->cam_P1;
    other_pov.body = level->body_P1_transform;
  }
  other_player = other_pov.body;
}

void SinglePlayerMode::player_switch() {
  if (player_num == 1) {
    player_num = 2;
  } else if (player_num == 2) {
    player_num = 1;
  }
  PlayerData temp = other_pov;
  other_pov = pov;
  pov = temp;

}

void SinglePlayerMode::handle_reset() {
  level_reset();
}

bool SinglePlayerMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

  if (PlayerMode::handle_event(evt, window_size)) return true;

  if (evt.type == SDL_KEYUP && evt.key.keysym.scancode == SDL_SCANCODE_Q) {
    player_switch();
  } else return false;
  return true;

}

void SinglePlayerMode::update_other_move(float elapsed) {

  glm::vec3 pl_pos = other_pov.body->position;
  glm::vec3 pl_vel = other_pov.vel;

  { // Gravitational accel
    pl_vel += elapsed * gravity;
  }

  if (!other_pov.in_air) { // Movement forces
    glm::vec3 pl_vel_move = glm::vec3(0.0f);

    pl_vel.x = glm::mix(pl_vel.x, pl_vel_move.x, std::pow(0.5f, elapsed / 0.01f));
    pl_vel.y = glm::mix(pl_vel.y, pl_vel_move.y, std::pow(0.5f, elapsed / 0.01f));
  }

  // Compute collisions for normal contact force
  {
    float remain = elapsed;

    if (other_pov.on_movable) {
      other_pov.on_movable->remove_player(player_num ^ 3);
      other_pov.on_movable = nullptr;
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
              other_pov.on_movable = &level->movable_data[collider.movable_index];
              other_pov.on_movable->add_player(other_pov.body, player_num ^ 3);
              //std::cout << "Got player!" << std::endl;
            }
          }
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
          other_pov.in_air = false;
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

      }
    }
    other_pov.body->position = pl_pos + pl_vel * remain;
    other_pov.vel = pl_vel;
  } // end collision compute

}

void SinglePlayerMode::update(float elapsed) {
  if (pause) return;
  if (!won && level->detect_win()) {
    won = true;
    to_next_level = 0.0f;
  }
  else if (level->detect_lose()) {
    lost = true;
  }

  update_shift(elapsed);
  if (shift.progress == 0.0f) {
    update_me_move(elapsed);
  }
  update_other_move(elapsed);
  update_movables_move(elapsed);

}

void SinglePlayerMode::update_network() {}
