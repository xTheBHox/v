#include "GameLevel.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "BasicMaterialProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

GLuint vao_level = -1U;

Load< MeshBuffer > level1_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer *ret = new MeshBuffer(data_path("level1.pnct"));
	vao_level = ret->make_vao_for_program(basic_material_program->program);
	return ret;
});
/*
Load< Scene > level1_scene(LoadTagLate, []() -> Scene const * {
	return new Scene(data_path("level1.scene"), [](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = level1_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;
		pipeline = basic_material_program_pipeline;
		pipeline.vao = vao_level;
		pipeline.type = mesh.type;
		pipeline.start = mesh.start;
		pipeline.count = mesh.count;

	});
});*/


GameLevel::GameLevel(std::string const &scene_file) {
  auto load_fn = [ this ]( Scene &, Transform *transform, std::string const &mesh_name ){
    Mesh const *mesh = &level1_meshes->lookup( mesh_name );

    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;

    //set up drawable to draw mesh from buffer:
    pipeline = basic_material_program_pipeline;
    pipeline.vao = vao_level;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;

    if (transform->name.substr(0, 7) == "Movable") {
        movables.emplace_back(transform);
        std::cout << "Movable detected!" << std::endl; 
    }
    pipeline.set_uniforms = [](){ 
        glUniform1f(basic_material_program->ROUGHNESS_float, 1.0f);
    };
  };
	//Load scene (using Scene::load function), building proper associations as needed:
	load(scene_file, load_fn);
}

GameLevel::~GameLevel() {
}


void GameLevel::draw( Camera const &camera ) {
  draw( camera, camera.make_projection() * camera.transform->make_world_to_local() );
}

void GameLevel::draw( Camera const &camera, glm::mat4 world_to_clip) {

	//--- actual drawing ---
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
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
