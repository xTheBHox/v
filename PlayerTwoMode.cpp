#include "PlayerTwoMode.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

#define PI 3.1415926f

PlayerTwoMode::PlayerTwoMode( GameLevel *level_ ) : level( level_ ){
  pov.camera = &( level->cameras.front() );
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

PlayerTwoMode::~PlayerTwoMode(){
  SDL_SetRelativeMouseMode(SDL_FALSE);
  delete level; //TODO VVVBad. Remove when network
}

bool PlayerTwoMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
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
  } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_SPACE) {
    controls.flat = true;
    SDL_SetRelativeMouseMode(SDL_FALSE);
  } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_SPACE) {
    controls.flat = false;
    SDL_SetRelativeMouseMode(SDL_TRUE);
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
    
    if (controls.flat) return true;
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

void PlayerTwoMode::update(float elapsed) {

  if (pause) return;

  if (controls.flat) {
  

  } else { // !flat 
    float pl_ca = std::cos(pov.azimuth);
    float pl_sa = std::sin(pov.azimuth);

  /**
    glm::mat3 player_frame = glm::mat3_cast(
      glm::angleAxis(pov.azimuth, glm::vec3(0.0f, 0.0f, 1.0f)) *
      glm::angleAxis(-pov.elevation + 0.5f * PI, glm::vec3(1.0f, 0.0f, 0.0f))
    );
  **/

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
  }

}

void PlayerTwoMode::draw(glm::uvec2 const &drawable_size) {
    
  pov.camera->aspect = drawable_size.x / float(drawable_size.y);

  if (controls.flat) {
    
    //std::cout << pov.camera->aspect << std::endl;
    //std::cout << pov.camera->fovy << std::endl;
    
    float height = 30.0f;
    float width = pov.camera->aspect * height;
    float nearPlaneDist = 30.0f;
    
    glm::mat4 proj_ortho = glm::ortho( -width, width, -height, height, nearPlaneDist, 1000.0f);
    glm::mat4 world_to_clip = proj_ortho * pov.camera->transform->make_world_to_local();
    level->draw( *pov.camera, world_to_clip );
    
  } else {
    level->draw( *pov.camera );
  }

}
