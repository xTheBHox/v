#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include "Mesh.hpp"

#include <string>
#include <list>

struct GameLevel : Scene {

  GameLevel( std::string const &scene_file );
  virtual ~GameLevel();

  void draw( Camera const &camera );
  void draw( Camera const &camera, glm::mat4 world_to_clip );

  struct MeshCollider {
    MeshCollider(Scene::Transform *transform_, Mesh const &mesh_, MeshBuffer const &buffer_) : transform(transform_), mesh(&mesh_), buffer(&buffer_) { }
    Scene::Transform *transform;
    Mesh const *mesh;
    MeshBuffer const *buffer;
  };

	//Goal objects(s) tracked using this structure:
	struct Goal {
		Goal(Scene::Transform *transform_) : transform(transform_) { };
		Scene::Transform *transform;
		float spin_acc = 0.0f;
	};

  struct Movable {

    void update();
    void init_cam(OrthoCam *cam);

    OrthoCam *cam_flat = nullptr;
    // The movement axis, direction away from player 2
    glm::vec3 axis = glm::vec3(0.0f);
    // The position player 2 needs to stand to move the object
    glm::vec3 mover_pos = glm::vec3(0.0f);

    // The object's transform.
    Transform *transform = nullptr;
    // The original position of the object
    glm::vec3 init_pos = glm::vec3(0.0f);
    // The current offset (along axis) of the object
    float offset = 0.0f;

    // The margin of error in position (distance units)
    float pos_tolerance = 9.0f;
    // The margin of error in viewing direction (degrees)
    float axis_tolerance = 0.2f;

  };

  Movable *movable_get( glm::vec3 const pos );

  std::vector< MeshCollider > mesh_colliders;
  std::vector< Goal > goals;
  std::list< Transform * > movables;
  std::list< Movable > movable_data;

  Transform *body_P1_transform;
  Camera *cam_P1;
  Transform *body_P2_transform;
  Camera *cam_P2;


};
