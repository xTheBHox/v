#pragma once

#include "Scene.hpp"
#include "Mesh.hpp"
#include "GL.hpp"

#include <string>
#include <list>

struct GameLevel : Scene {

  GameLevel( std::string const &scene_file );
  virtual ~GameLevel();

  void draw(
    glm::vec2 const &drawable_size,
    glm::vec3 const &eye,
    glm::mat4 const &world_to_clip
  );
  void draw(
    glm::vec2 const &drawable_size,
    glm::vec3 const &eye,
    glm::mat4 const &world_to_clip,
    GLuint output_fb
  );

  void reset(bool resetBySync);
  bool resetSync = false;

	//Goal objects(s) tracked using this structure:
	struct Goal {
		Goal(Scene::Transform *transform_) : transform(transform_) { };
		Scene::Transform *transform;
		float spin_acc = 10.0f;
	};

  void detect_win();

  void detect_winLose();

  float die_y = -40.0;
  void detect_lose();


  struct Movable {

    Movable(Transform *transform_);
    void update(glm::vec3 &diff);
    void set_target_pos(glm::vec3 &target, glm::vec4 &color_);

    // The object's transform.
    Transform *transform = nullptr;
    // The original position of the object
    glm::vec3 init_pos = glm::vec3(0.0f);

    glm::vec4 color = glm::vec4(1.0f);
    glm::vec3 target_pos = glm::vec3(0.0f);
    float vel = 5.0f;

    Transform *player = nullptr;

  };

  struct Standpoint {

    Standpoint(OrthoCam *cam_, Movable *movable);
    void resize_texture(glm::uvec2 const &new_size);
    void update_texture(GameLevel *level);
    OrthoCam *cam;
    Movable *movable;

    // The movement axis, direction away from player 2
    glm::vec3 axis = glm::vec3(0.0f);
    // The position of the orthographic camera
    glm::vec3 pos = glm::vec3(0.0f);

    GLuint tex = 0;
    glm::uvec2 size = glm::uvec2(0);

    struct MovePosition {
      MovePosition(Light *light);
      Transform *transform = nullptr;
      glm::vec3 pos = glm::vec3(0.0f);
      glm::vec4 color = glm::vec4(0.0f);
    };

    std::vector< MovePosition > move_pos;

    // ==== Constants ====

    // The dimensions of the screen
    const float w = 3.0f;
    const float h = 2.25f;

    // The margin of error in position (distance units)
    const float pos_tolerance = 9.0f;
    // The margin of error in viewing direction (cos(max error angle))
    const float axis_tolerance = 0.85f;

  };

  struct Screen {
    Screen(Transform *transform_, Drawable::Pipeline *pipeline_);

    void set_standpoint(Standpoint *stpt);

    Transform *transform = nullptr;
    Drawable::Pipeline *pipeline = nullptr;

    Standpoint *stpt = nullptr;

    // The position player 2 needs to stand to move the object
    glm::vec3 pos = glm::vec3(0.0f);
    // The margin of error in position (distance units)
    const float pos_tolerance = 5.0f;
    // The margin of error in viewing direction (cos(max error angle))
    const float axis_tolerance = 0.9f;
  };

  struct MeshCollider {
    MeshCollider(
      Transform *transform_,
      Mesh const &mesh_,
      MeshBuffer const &buffer_,
      Movable *movable_ = nullptr
    ) :
      transform(transform_),
      mesh(&mesh_),
      buffer(&buffer_),
      movable(movable_)
    {}
    Scene::Transform *transform;
    Mesh const *mesh;
    MeshBuffer const *buffer;
    Movable *movable = nullptr;
  };

  Screen *screen_get(Transform *transform);

  std::vector< MeshCollider > mesh_colliders;
  std::vector< Goal > goals;
  std::vector< Movable > movable_data;
  std::vector< Standpoint > standpoints;
  std::vector< Screen > screens;

  Transform *body_P1_transform;
  Camera *cam_P1;
  Transform *body_P2_transform;
  Camera *cam_P2;

};
