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

  std::list< Transform * >movables;

};
