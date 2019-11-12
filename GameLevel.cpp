#include "GameLevel.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "demo_menu.hpp"
#include "OutlineProgram.hpp"
#include "BasicMaterialProgram.hpp"
#include "gl_errors.hpp"
#include "check_fb.hpp"
#include "CopyToScreenProgram.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define PI 3.1415926f

//used for lookup later:
Mesh const *mesh_Goal = nullptr;
//Mesh const *mesh_Body_P1 = nullptr;
//Mesh const *mesh_Body_P2 = nullptr;

//names of mesh-to-collider-mesh:
std::unordered_map< Mesh const *, Mesh const * > mesh_to_collider;

GLuint vao_color = -1U;
GLuint vao_outline = -1U;

Load< MeshBuffer > level1_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer *ret = new MeshBuffer(data_path("level1.pnct"));
	vao_color = ret->make_vao_for_program(flat_program->program);
	vao_outline = ret->make_vao_for_program(outline_program_0->program);

  //key objects:
  mesh_Goal = &ret->lookup("Goal");
  //mesh_Body_P1 = &ret->lookup("Body1");
  //mesh_Body_P2 = &ret->lookup("Body2");

  //collidable objects:
  mesh_to_collider.insert(std::make_pair(&ret->lookup("Platform1"), &ret->lookup("Platform1")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("Platform2"), &ret->lookup("Platform2")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("Platform3"), &ret->lookup("Platform3")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("MovePlatform"), &ret->lookup("MovePlatform")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("ScreenFrame"), &ret->lookup("ScreenFrame")));

	return ret;
});

GameLevel::GameLevel(std::string const &scene_file) {
  uint32_t decorations = 0;

  auto load_fn = [this, &decorations](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &level1_meshes->lookup(mesh_name);

    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;

    //set up drawable to draw mesh from buffer:
    pipeline = flat_program_pipeline;
    pipeline.vao = vao_color;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;

    pipeline.set_uniforms = [](){
      glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_VX_COLORS);
    };

    if (transform->name.substr(0, 4) == "Move") {
      size_t name_len = transform->name.size();
      if (transform->name.substr(name_len - 6, name_len) == "Screen") {
        std::cout << "Screen detected: " << transform->name << std::endl;
        screens.emplace_back(transform, &pipeline);
        pipeline.set_uniforms = [](){
          glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_TEX);
        };
      } else if (transform->name.substr(name_len - 9, name_len - 1) == "Position") {
        std::cout << "MovePosition detected: " << transform->name << std::endl;
        drawables.pop_back();
        move_positions.emplace_back(transform);
        move_positions.back().pos = transform->position;
        move_positions.back().color = mesh->color;
      } else {
        std::cout << "Movable detected: " << transform->name << std::endl;

        movable_data.emplace_back(transform);
        Movable &data = movable_data.back();
        pipeline.set_uniforms = [&data](){
          glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
          glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(data.color));
        };

        auto f = mesh_to_collider.find(mesh);
        // TODO NOT FOUND CHECK!
        mesh_colliders.emplace_back(transform, *f->second, *level1_meshes, &data);
      }
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
  };
	//Load scene (using Scene::load function), building proper associations as needed:
	load(scene_file, load_fn);

  // Build the mapping between movables and orthographic cameras

  for (auto &oc : orthocams) {
    for (auto &m : movable_data) {
      std::string &xf_name = m.transform->name;
      if (oc.transform->name.substr(0, xf_name.size()) == xf_name) {
        std::cout << "Matched " << xf_name << " to " << oc.transform->name << std::endl;
        standpoints.emplace_back(&oc, &m);
        for (Standpoint::MovePosition &mp : move_positions) {
          std::string &mp_name = mp.transform->name;
          if (mp_name.substr(0, mp_name.size()-9) == oc.transform->name) {
            std::cout << "Matched " << mp_name << " to " << oc.transform->name << std::endl;
            standpoints.back().move_pos.emplace_back(&mp);
          }
        }
        break;
      }
    }
  }

  for (auto &sc : screens) {
    for (auto &stpt : standpoints) {
      std::string &cam_name = stpt.cam->transform->name; // xyzCam
      std::string &sc_name = sc.transform->name; // xyzScreen
      if (cam_name.substr(0, cam_name.size()-3) == sc_name.substr(0, sc_name.size()-6)) {
        std::cout << "Matched " << cam_name << " to " << sc_name << std::endl;
        std::cout << "Texture #" << stpt.tex << std::endl;
        sc.set_standpoint(&stpt);
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
void GameLevel::detect_winLose(){
  detect_lose();
  detect_win();
}

void GameLevel::detect_lose(){
  std::cout << body_P1_transform->position.z << " "<< body_P2_transform->position.z << std::endl;
  if (body_P1_transform->position.z < die_y || body_P2_transform->position.z < die_y){
    MenuMode::set_current(nullptr);
  }
}
void GameLevel::detect_win(){
  for (auto &g: goals){
    glm::vec3 goalPos = g.transform->position;
    glm::vec3 p1 = body_P1_transform->position;
    glm::vec3 p2 = body_P2_transform->position;
    auto dis1 = glm::distance(goalPos, p1);
    auto dis2 = glm::distance(goalPos, p2);
    std::cout << dis1 << std::endl;
    if ((dis1 < g.spin_acc) || (dis2 < g.spin_acc)){
      std::cout << "You win!!" << std::endl;
      MenuMode::set_current(nullptr);
      //exit(0);
      //return true;
    }
  }
  //return false;
}

void GameLevel::reset(bool resetBySync) {
  if (!resetBySync){
    resetSync = true;
    std::cout << "Reset in game level" << std::endl;
  }
  for (Movable &m : movable_data) {
    m.transform->position = m.init_pos;
  }
}

//Helper: maintain a framebuffer to hold rendered geometry
struct FB {
	//object data gets stored in these textures:
	GLuint normal_tex = 0;
  GLuint position_tex = 0;

	//output image gets written to this texture:
	GLuint color_tex = 0;

	//depth buffer is shared between objects + lights pass:
	GLuint depth_rb = 0;

	GLuint fb_color = 0;
  GLuint fb_outline = 0;
  GLuint fb_output = 0;

	glm::uvec2 size = glm::uvec2(0);

	void resize(glm::uvec2 const &drawable_size) {
		if (drawable_size == size) return;
		size = drawable_size;

		//helper to allocate a texture:

    auto alloc_recttex = [&] (GLuint &tex, GLenum internal_format) {
      if (tex == 0) glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_RECTANGLE, tex);
      glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internal_format, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    };

		//set up normal_tex as a 16-bit floating point RGBA texture:
		alloc_recttex(normal_tex, GL_RGB32F);
    alloc_recttex(position_tex, GL_RGB32F);

		//set up output_tex as an 8-bit fixed point RGBA texture:
		alloc_recttex(color_tex, GL_RGBA8);

    GL_ERRORS();

		//if depth_rb does not have a name, name it:
		if (depth_rb == 0) glGenRenderbuffers(1, &depth_rb);
		//set up depth_rb as a 24-bit fixed-point depth buffer:
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_ERRORS();

		//if color framebuffer doesn't have a name, name it and attach textures:
		if (fb_color == 0) {
			glGenFramebuffers(1, &fb_color);
			//set up framebuffer: (don't need to do when resizing)
			glBindFramebuffer(GL_FRAMEBUFFER, fb_color);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, color_tex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			check_fb();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
    GL_ERRORS();

    //if outline framebuffer doesn't have a name, name it and attach textures:
    if (fb_outline == 0) {
      glGenFramebuffers(1, &fb_outline);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_outline);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, normal_tex, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, position_tex, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
      GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
			glDrawBuffers(2, bufs);
      check_fb();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL_ERRORS();
    // The output framebuffer is used to hold the flattened textures when rendering
    // the orthographic windows.
    if (fb_output == 0) {
      glGenFramebuffers(1, &fb_output);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_output);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL_ERRORS();
	}
} fb;

void GameLevel::draw(
  glm::vec2 const &drawable_size,
  glm::vec3 const &eye,
  glm::mat4 const &world_to_clip
) {
  draw(drawable_size, eye, world_to_clip, 0);
}

void GameLevel::draw(
  glm::vec2 const &drawable_size,
  glm::vec3 const &eye,
  glm::mat4 const &world_to_clip,
  GLuint output_fb
) {

  fb.resize(drawable_size);
  GL_ERRORS();
  // Color drawing

  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_color);
  GLfloat zeros[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, zeros);
  glClear(GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	Scene::draw(world_to_clip);

  GL_ERRORS();

  // Draw the outlines
  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_outline);
  glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

  glUseProgram(outline_program_0->program);
  glBindVertexArray(vao_outline);

  for (auto const &drawable : drawables) {

		assert(drawable.transform); //drawables *must* have a transform
		glm::mat4 object_to_world = drawable.transform->make_local_to_world();

    if (outline_program_0->OBJECT_TO_CLIP_mat4 != -1U) {
      glm::mat4 object_to_clip = world_to_clip * object_to_world;
      glUniformMatrix4fv(outline_program_0->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(object_to_clip));
    }

    // Uses the same pipeline as flat coloring
    Scene::Drawable::Pipeline const &pipeline = drawable.pipeline;
    glDrawArrays(pipeline.type, pipeline.start, pipeline.count);

  }
  GL_ERRORS();
  // Draw to screen
  glBindFramebuffer(GL_FRAMEBUFFER, output_fb);
  glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vao_empty);
  glUseProgram(outline_program_1->program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE, fb.color_tex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_RECTANGLE, fb.normal_tex);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_RECTANGLE, fb.position_tex);
  glDrawArrays(GL_TRIANGLES, 0, 3);
	GL_ERRORS();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindVertexArray(0);
	glUseProgram(0);
	GL_ERRORS();

}

GameLevel::Movable::Movable(Transform *transform_) : transform(transform_) {

  init_pos = transform->position;

}

void GameLevel::Movable::update(glm::vec3 &diff) {
  transform->position += diff;
  if (player) {
    player->position += diff;
  }
}

void GameLevel::Movable::set_target_pos(glm::vec3 &target, glm::vec4 &color_) {

  color = color_;
  target_pos = target;

}

GameLevel::Standpoint::Standpoint(OrthoCam *cam_, Movable *movable_)
  : cam(cam_), movable(movable_) {

  // Cameras are directed along the -z axis. Get the transformed z-axis.
  axis = cam->transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
  if (axis != glm::vec3(0.0f)) axis = glm::normalize(axis);

  std::cout << "Axis: " << axis.x << "\t" << axis.y << "\t" << axis.z << std::endl;

  // Get the transformed origin;
  pos = cam->transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  std::cout << "Position: " << pos.x << "\t" << pos.y << "\t" << pos.z << std::endl;

  glGenTextures(1, &tex);
  GL_ERRORS();

}

void GameLevel::Standpoint::resize_texture(glm::uvec2 const &new_size) {

  if (size == new_size) return;
  size = new_size;

  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  GL_ERRORS();

}

void GameLevel::Standpoint::update_texture(GameLevel *level) {

  float tex_h = cam->scale;
  float tex_w = (w / h) * tex_h;
  float fp = cam->clip_far;
  float np = cam->clip_near;

  glm::mat4 proj = glm::ortho(-tex_w, tex_w, -tex_h, tex_h, np ,fp);
  glm::mat4 w2l = cam->transform->make_world_to_local();

  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_output);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  check_fb();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  GL_ERRORS();

  level->draw(size, pos, proj * w2l, fb.fb_output);

}

GameLevel::Screen::Screen(Transform *transform_, Drawable::Pipeline *pipeline_)
: transform(transform_), pipeline(pipeline_) {
  pos = transform->make_local_to_world()[3];
}

void GameLevel::Screen::set_standpoint(GameLevel::Standpoint *stpt_) {
  stpt = stpt_;
  pipeline->textures[0].texture = stpt->tex;
  pipeline->textures[0].target = GL_TEXTURE_2D;
}

GameLevel::Screen *GameLevel::screen_get(Transform *transform) {

  glm::vec3 pos = transform->make_local_to_world()[3]; // * (0,0,0,1)
  glm::vec3 axis = transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
  if (axis == glm::vec3(0.0f)) return nullptr;
  axis = glm::normalize(axis);

  std::cout << "Pos: " << pos.x << "\t" << pos.y << "\t" << pos.z << std::endl;
  std::cout << "Axis: " << axis.x << "\t" << axis.y << "\t" << axis.z << std::endl;

  for (Screen &sc : screens) {
    std::cout << "Screen: " << sc.pos.x << "\t" << sc.pos.y << "\t" << sc.pos.z << std::endl;

    glm::vec3 dist = sc.pos - pos;
    std::cout << glm::dot(dist, dist) << std::endl;
    if (glm::dot(dist, dist) <= sc.pos_tolerance * sc.pos_tolerance) {
      std::cout << "Position within threshhold. Checking axis..." << std::endl;
      std::cout << "Dot: " << glm::dot(axis, dist) << std::endl;
      if (glm::dot(axis, dist) > sc.axis_tolerance) {
        return &sc;
      }
    }
  }

  return nullptr;

}
