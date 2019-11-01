#include "GameLevel.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define PI 3.1415926f

//used for lookup later:
Mesh const *mesh_Goal = nullptr;
//Mesh const *mesh_Body_P1 = nullptr;
//Mesh const *mesh_Body_P2 = nullptr;

//names of mesh-to-collider-mesh:
std::unordered_map< Mesh const *, Mesh const * > mesh_to_collider;

GLuint vao_level = -1U;

Load< MeshBuffer > level1_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer *ret = new MeshBuffer(data_path("level1.pnct"));
	vao_level = ret->make_vao_for_program(basic_material_program->program);

  //key objects:
  mesh_Goal = &ret->lookup("Goal");
  //mesh_Body_P1 = &ret->lookup("Body1");
  //mesh_Body_P2 = &ret->lookup("Body2");

  //collidable objects:
  mesh_to_collider.insert(std::make_pair(&ret->lookup("Room"), &ret->lookup("Room")));

	return ret;
});

GameLevel::GameLevel(std::string const &scene_file) {
  uint32_t decorations = 0;

  auto load_fn = [this, &decorations](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &level1_meshes->lookup(mesh_name);

    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;

    //set up drawable to draw mesh from buffer:
    pipeline = basic_material_program_pipeline;
    pipeline.vao = vao_level;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;

    if (transform->name.substr(0, 4) == "Move") {
      std::cout << "Movable detected!" << std::endl;
      movables.emplace_back(transform);
      movable_data.emplace_back();
      Movable &data = movable_data.back();
      data.transform = transform;
      data.init_pos = transform->position;
      auto f = mesh_to_collider.find(mesh);
      mesh_colliders.emplace_back(transform, *f->second, *level1_meshes);
    } else if (transform->name.substr(0, 4) == "Goal") {
      goals.emplace_back(transform);
    } else if (transform->name.substr(0, 5) == "Body1") {
      body_P1_transform = transform;
    } else if (transform->name.substr(0, 5) == "Body2") {
      body_P2_transform = transform;
    } else {
      auto f = mesh_to_collider.find(mesh);
      if (f != mesh_to_collider.end()) {
        mesh_colliders.emplace_back(transform, *f->second, *level1_meshes);
      } else {
        decorations++;
      }
    }

    pipeline.set_uniforms = [](){
        glUniform1f(basic_material_program->ROUGHNESS_float, 1.0f);
    };
  };
	//Load scene (using Scene::load function), building proper associations as needed:
	load(scene_file, load_fn);

  // Build the mapping between movables and orthographic cameras
  for (auto &m : movable_data) {
    std::string &xf_name = m.transform->name;
    for (auto &oc : orthocams) {
      if (oc.transform->name.substr(0, xf_name.size()) == xf_name) {
        m.init_cam(&oc);
        break;
      }
    }
  }

  // Get the player cameras
  for (auto &c : cameras) {
    if (c.transform->name.substr(0, 7) == "Player1") {
      cam_P1 = &c;
    } else if (c.transform->name.substr(0, 7) == "Player2") {
      cam_P2 = &c;
    }
  }

}

GameLevel::~GameLevel() {
}


void GameLevel::draw( Camera const &camera ) {
  draw( camera, camera.make_projection() * camera.transform->make_world_to_local() );
}

void GameLevel::draw( Camera const &camera, glm::mat4 world_to_clip) {

	//--- actual drawing ---
	glClearColor(0.8f, 0.8f, 0.95f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glm::vec3 eye = camera.transform->make_local_to_world()[3];

	for (auto const &light : lights) {
		glm::mat4 light_to_world = light.transform->make_local_to_world();
		//set up lighting information for this light:
		glUseProgram(basic_material_program->program);
		glUniform3fv(basic_material_program->EYE_vec3, 1, glm::value_ptr(eye));
		glUniform3fv(basic_material_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(glm::vec3(light_to_world[3])));
		glUniform3fv(basic_material_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(-light_to_world[2])));
		glUniform3fv(basic_material_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light.energy));
		if (light.type == Scene::Light::Point) {
			glUniform1i(basic_material_program->LIGHT_TYPE_int, 0);
			glUniform1f(basic_material_program->LIGHT_CUTOFF_float, 1.0f);
		} else if (light.type == Scene::Light::Hemisphere) {
			glUniform1i(basic_material_program->LIGHT_TYPE_int, 1);
			glUniform1f(basic_material_program->LIGHT_CUTOFF_float, 1.0f);
		} else if (light.type == Scene::Light::Spot) {
			glUniform1i(basic_material_program->LIGHT_TYPE_int, 2);
			glUniform1f(basic_material_program->LIGHT_CUTOFF_float, std::cos(0.5f * light.spot_fov));
		} else if (light.type == Scene::Light::Directional) {
			glUniform1i(basic_material_program->LIGHT_TYPE_int, 3);
			glUniform1f(basic_material_program->LIGHT_CUTOFF_float, 1.0f);
		}
		Scene::draw(world_to_clip);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);
	}
}

void GameLevel::Movable::init_cam(OrthoCam *cam) {
  cam_flat = cam;

  std::cout << "One" << std::endl;
  // Cameras are directed along the -z axis. Get the transformed z-axis.
  axis = cam->transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

  // Get the transformed origin;
  mover_pos = cam->transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  std::cout << "Two" << std::endl;
}

void GameLevel::Movable::update() {
  transform->position = init_pos + offset * axis;
}

GameLevel::Movable *GameLevel::movable_get( glm::vec3 const pos ) {

  std::cout << "Pos: " << pos.x << "\t" << pos.y << "\t" << pos.z << std::endl;

  for (Movable &m : movable_data) {
    std::cout << "Mover: " << m.mover_pos.x << "\t" << m.mover_pos.y << "\t" << m.mover_pos.z << std::endl;

    glm::vec3 dist = m.mover_pos - pos;
    std::cout << glm::dot( dist, dist ) << std::endl;
    if ( glm::dot( dist, dist ) <= m.pos_tolerance * m.pos_tolerance ){
      return &m;
    }
  }

  return nullptr;

}
