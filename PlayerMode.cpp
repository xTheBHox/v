#include "PlayerMode.hpp"
#include "demo_menu.hpp"
#include "collide.hpp"

#include <iostream>
#include <algorithm>

#define PI 3.1415926f

PlayerMode::PlayerMode(GameLevel *level_) : level(level_) {
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

PlayerMode::~PlayerMode(){
  SDL_SetRelativeMouseMode(SDL_FALSE);
  delete level; //TODO VVVBad. Remove when network
}

bool PlayerMode::handle_ui(SDL_Event const &evt, glm::uvec2 const &window_size) {
  if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_M) {
		Mode::set_current(demo_menu);
	} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
    pause = !pause;
    if (pause) SDL_SetRelativeMouseMode(SDL_FALSE);
    else SDL_SetRelativeMouseMode(SDL_TRUE);
  } else return false;

  return true;
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
    } else if (evt.key.keysym.sym == SDLK_LSHIFT) {
      controls.sprint = (evt.type == SDL_KEYDOWN);
    } else if (evt.key.keysym.sym == SDLK_SPACE) {
      controls.jump = (evt.type == SDL_KEYDOWN);
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
    pov.elevation = std::max(-85.0f / 180.0f * PI, pov.elevation);
    pov.elevation = std::min(60.0f / 180.0f * PI, pov.elevation);

  } else if (evt.type == SDL_MOUSEBUTTONDOWN || evt.type == SDL_MOUSEBUTTONUP) {
    if (evt.button.button == SDL_BUTTON_LEFT) {
      controls.mouse_down = (evt.type == SDL_MOUSEBUTTONDOWN);
    }
  } else return false;

  return true;

}

void PlayerMode::update(float elapsed) {

  if (pause) return;

  float pl_cosazi = std::cos(pov.azimuth);
  float pl_sinazi = std::sin(pov.azimuth);

  glm::vec3 pl_pos = pov.body->position;
  glm::vec3 pl_vel = pov.vel;

  { // Gravitational accel
    pl_vel += elapsed * pov.gravity;
  }

  { // Jumping force
    if (controls.jump) {
      if (!pov.in_air) {
        pov.in_air = true;
        pl_vel += -0.3f * pov.gravity; // jumping force, constant
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
      pl_vel_move *= 70.0f; // player movement velocity magnitude, constant
      if (controls.sprint) pl_vel_move *= 1.5f; // sprint force multiplier, constant
      //pl_vel += pl_vel_move;
      pl_vel.x = glm::mix(pl_vel.x, pl_vel_move.x, std::pow(0.5f, elapsed / 0.02f));
      pl_vel.y = glm::mix(pl_vel.y, pl_vel_move.y, std::pow(0.5f, elapsed / 0.02f));
    }
    else {

    }
    // pov.vel *= std::pow(0.5f, elapsed / 0.01f); // friction
  }


  // Compute collisions for normal contact force
  {
    float remain = elapsed;

    // Iterate for multiple bounces. Stop at a certain number.
    for (int32_t iter = 0; iter < 5; ++iter) {
      if (remain == 0.0f) break;

      float sphere_radius = 5.0f; // player sphere is radius-1
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
            if (collider.movable) {
              collider.movable->player = pov.body;
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
          pl_vel -= (1.0f - std::pow(0.5f, remain / 0.01f)) * (pl_vel - d * collision_out);
          pl_vel -= 1.01f * d * collision_out;
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

void PlayerMode::draw(glm::uvec2 const &drawable_size) {

  pov.camera->aspect = drawable_size.x / float(drawable_size.y);
  glm::vec4 eye = pov.camera->transform->make_local_to_world()[3];
  glm::mat4 world_to_clip = pov.camera->make_projection() * pov.camera->transform->make_world_to_local();
  level->draw(drawable_size, eye, world_to_clip);

}
