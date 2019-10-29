#include "PlayerOneMode.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"
#include "collide.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

using namespace std;
#define PI 3.1415926f

PlayerOneMode::PlayerOneMode( GameLevel *level_ , std::string const &port) : level( level_ ){
  pov.camera = &( level->cameras.front() );
  pov.body_transform = level->body_P1_transform;
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
  } else return false;

  return true;

}

void PlayerOneMode::update(float elapsed) {
  if (pause) return;

  glm::vec3 &position = pov.body_transform->position;
  glm::vec3 &velocity = pov.vel;
  glm::quat &rotation = pov.body_transform->rotation;
  glm::vec3 &rotational_velocity = pov.rotational_velocity;

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
      float pl_accel = 50.0f;
      // pov.vel += pl_accel * pl_dvel;
      pl_dvel *= pl_accel;
    }
    // pov.vel *= std::pow(0.5f, elapsed / 0.01f); // friction
    pov.vel.x = glm::mix(pl_dvel.x, pov.vel.x, std::pow(0.5f, elapsed / 0.01f));
    pov.vel.y = glm::mix(pl_dvel.y, pov.vel.y, std::pow(0.5f, elapsed / 0.01f));
  }

	rotational_velocity *= std::pow(0.5f, elapsed / 2.0f);
  //gravity, sprinting and jumping
  velocity += pov.gravity * elapsed * 9.0f;
  if (controls.sprint){
    pov.vel.x *= 1.5f; pov.vel.y *= 1.5f;
  }
  if (controls.jump) {
    if (!pov.in_air){ velocity += -3.0f * pov.gravity; pov.in_air = true; }
    controls.jump = false;
  }
  // Update player position
  // pov.body_transform->position += pov.vel;

  // Update camera rotation
  // {
  //   pov.camera->transform->rotation = glm::angleAxis(
  //     pov.azimuth,
  //     glm::vec3( 0.0f, 0.0f, 1.0f )
  //   ) * glm::angleAxis(
  //     -pov.elevation + 0.5f * PI,
  //     glm::vec3( 1.0f, 0.0f, 0.0f )
  //   );
  // }

  // Compute collisions
  {
    float remain = elapsed;
    for (int32_t iter = 0; iter < 10; ++iter) {
      if (remain == 0.0f) break;

      float sphere_radius = 5.0f; //player sphere is radius-1
      glm::vec3 sphere_sweep_from = position;
      glm::vec3 sphere_sweep_to = position + velocity * remain;

      glm::vec3 sphere_sweep_min = glm::min(sphere_sweep_from, sphere_sweep_to) - glm::vec3(sphere_radius);
      glm::vec3 sphere_sweep_max = glm::max(sphere_sweep_from, sphere_sweep_to) + glm::vec3(sphere_radius);

      bool collided = false;
      float collision_t = 1.0f;
      glm::vec3 collision_at = glm::vec3(0.0f);
      glm::vec3 collision_out = glm::vec3(0.0f);
      bool is_surface_collision = false;
      for (auto const &collider : level->mesh_colliders) {
        glm::mat4x3 collider_to_world = collider.transform->make_local_to_world();

        { //Early discard:
          // check if AABB of collider overlaps AABB of swept sphere:

          //(1) compute bounding box of collider in world space:
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

          //(2) check if bounding boxes overlap:
          bool can_skip = !collide_AABB_vs_AABB(sphere_sweep_min, sphere_sweep_max, world_min, world_max);

          //draw to indicate result of check:
          // if (iter == 0 && DEBUG_draw_lines && DEBUG_show_geometry) {
          //   DEBUG_draw_lines->draw_box(glm::mat4x3(
          //     0.5f * (world_max.x - world_min.x), 0.0f, 0.0f,
          //     0.0f, 0.5f * (world_max.y - world_min.y), 0.0f,
          //     0.0f, 0.0f, 0.5f * (world_max.z - world_min.z),
          //     0.5f * (world_max.x+world_min.x), 0.5f * (world_max.y+world_min.y), 0.5f * (world_max.z+world_min.z)
          //   ), (can_skip ? glm::u8vec4(0x88, 0x88, 0x88, 0xff) : glm::u8vec4(0x88, 0x88, 0x00, 0xff) ) );
          // }

          if (can_skip) {
            //AABBs do not overlap; skip detailed check:
            continue;
          }
        }

        //Full (all triangles) test:
        assert(collider.mesh->type == GL_TRIANGLES); //only have code for TRIANGLES not other primitive types
        for (GLuint v = 0; v + 2 < collider.mesh->count; v += 3) {
          //get vertex positions from associated positions buffer:
          //  (and transform to world space)
          glm::vec3 a = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+0], 1.0f);
          glm::vec3 b = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+1], 1.0f);
          glm::vec3 c = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+2], 1.0f);
          //check triangle:
          bool did_collide = collide_swept_sphere_vs_triangle(
            sphere_sweep_from, sphere_sweep_to, sphere_radius,
            a,b,c,
            &collision_t, &collision_at, &collision_out, &is_surface_collision);

          if (did_collide) {
            collided = true;
            //allow jumping if collision is with triangle with normal in positive z direction
            glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
            if (normal.z > 0.0f) { pov.in_air = false; }
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
        position = sphere_sweep_to;
        remain = 0.0f;
        break;
      } else {
        position = glm::mix(sphere_sweep_from, sphere_sweep_to, collision_t);
        // if (is_surface_collision) pov.gravity = -collision_out;
        float d = glm::dot(velocity, collision_out);
        if (d < 0.0f) {
          velocity -= (1.1f * d) * collision_out;

          //update rotational velocity to reflect relative motion:
          glm::vec3 slip = glm::cross(rotational_velocity, collision_at - position) + velocity;
          //DEBUG: std::cout << "slip: " << slip.x << " " << slip.y << " " << slip.z << std::endl;
          /*if (DEBUG_draw_lines) {
            DEBUG_draw_lines->draw(collision_at, collision_at + slip, glm::u8vec4(0x00, 0xff, 0x00, 0xff));
          }*/
          glm::vec3 change = glm::cross(slip, collision_at - position);
          rotational_velocity += change;
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
        remain = (1.0f - collision_t) * remain;
      }
    }

		//update player rotation (purely cosmetic):
		rotation = glm::normalize(
			  glm::angleAxis(elapsed * rotational_velocity.x, glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::angleAxis(elapsed * rotational_velocity.y, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::angleAxis(elapsed * rotational_velocity.z, glm::vec3(0.0f, 0.0f, 1.0f))
			* rotation
		);
  } // end collision compute

	{ //camera update:
    // float gravity_to_z_angle = glm::acos(glm::dot(-pov.gravity, glm::vec3(0.0f, 0.0f, 1.0f)));
    // if (-pov.gravity.x < 0.0f) gravity_to_z_angle = -gravity_to_z_angle;
    pov.camera->transform->rotation =
      // glm::angleAxis(gravity_to_z_angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::angleAxis( pov.azimuth, glm::vec3(0.0f, 0.0f, 1.0f) )
			* glm::angleAxis(-pov.elevation + 0.5f * 3.1415926f, glm::vec3(1.0f, 0.0f, 0.0f) )
		;
		// glm::vec3 in = pov.camera->transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 top = pov.body_transform->position - pov.gravity * 1.0f;
		pov.camera->transform->position = top; // - 10.0f * in;
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
            auto start = (&data[0]);
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
