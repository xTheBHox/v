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
#include <iomanip>

#define PI 3.1415926f

void GameLevel::init_meshes(std::string level_name) {
  level_name.insert(level_name.size(), ".pnct");

  std::cout << "Loading " << level_name << std::endl;
  meshes = new MeshBuffer(level_name);

  std::cout << "Level meshes loaded" << std::endl;

  vao_color = meshes->make_vao_for_program(flat_program->program);
  vao_outline = meshes->make_vao_for_program(outline_program_0->program);

  std::cout << "VAOs created" << std::endl;

  //collidable objects:

}

void print_mat4(glm::mat4 const &M) {
  std::cout << std::setprecision(4);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::cout << M[j][i] << "\t";
    }
    std::cout << std::endl;
  }
}

void print_vec4(glm::vec4 const &v) {
  std::cout << std::setprecision(4);
  for (int i = 0; i < 4; i++) {
    std::cout << v[i] << "\t";
  }
  std::cout << std::endl;
}

void print_vec3(glm::vec3 const &v) {
  std::cout << std::setprecision(4);
  for (int i = 0; i < 3; i++) {
    std::cout << v[i] << "\t";
  }
  std::cout << std::endl;
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

  GLuint normal_tex_main = 0;
  GLuint position_tex_main = 0;

  //output image gets written to this texture:
  GLuint color_tex_main = 0;
  //depth buffer is shared between objects + lights pass:
  GLuint depth_rb_main = 0;

  GLuint fb_color_main = 0;
  GLuint fb_outline_main = 0;

  GLuint color_tex_sc = 0;
  GLuint normal_tex_sc = 0;
  GLuint position_tex_sc = 0;
  GLuint depth_rb_sc = 0;

  GLuint fb_color_sc = 0;
  GLuint fb_outline_sc = 0;
  GLuint fb_output_sc = 0;

  glm::uvec2 size = glm::uvec2(0);
  glm::uvec2 size_sc = glm::uvec2(640, 480);

  void init_sc() {
    auto alloc_recttex = [&] (GLuint &tex, GLenum internal_format) {
      if (tex == 0) glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_RECTANGLE, tex);
      glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internal_format, size_sc.x, size_sc.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    };
    //set up normal_tex as a 16-bit floating point RGBA texture:
    alloc_recttex(normal_tex_sc, GL_RGBA32F);
    alloc_recttex(position_tex_sc, GL_RGBA32F);
    //set up output_tex as an 8-bit fixed point RGBA texture:
    alloc_recttex(color_tex_sc, GL_RGBA8);
    if (depth_rb_sc == 0) glGenRenderbuffers(1, &depth_rb_sc);
    //set up depth_rb as a 24-bit fixed-point depth buffer:
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb_sc);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size_sc.x, size_sc.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_ERRORS();

    if (fb_color_sc == 0) {
      glGenFramebuffers(1, &fb_color_sc);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_color_sc);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, color_tex_sc, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb_sc);
      glDrawBuffer(GL_COLOR_ATTACHMENT0);
      check_fb();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    GL_ERRORS();

    //if outline framebuffer doesn't have a name, name it and attach textures:
    if (fb_outline == 0) {
      glGenFramebuffers(1, &fb_outline_sc);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_outline_sc);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, normal_tex_sc, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, position_tex_sc, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb_sc);
      GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
      glDrawBuffers(2, bufs);
      check_fb();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL_ERRORS();
    // The output framebuffer is used to hold the flattened textures when rendering
    // the orthographic windows.
    if (fb_output_sc == 0) {
      glGenFramebuffers(1, &fb_output_sc);
      //set up framebuffer: (don't need to do when resizing)
      //glBindFramebuffer(GL_FRAMEBUFFER, fb_output_sc);
      //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb_sc);
      //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL_ERRORS();
  }

  void resize_main(glm::uvec2 const &drawable_size) {
    if (drawable_size == size) return;
    size = drawable_size;

    std::cout << "Resizing draw textures and framebuffers!" << std::endl;

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
    alloc_recttex(normal_tex_main, GL_RGBA32F);
    alloc_recttex(position_tex_main, GL_RGBA32F);

    //set up output_tex as an 8-bit fixed point RGBA texture:
    alloc_recttex(color_tex_main, GL_RGBA8);

    GL_ERRORS();

    //if depth_rb does not have a name, name it:
    if (depth_rb_main == 0) glGenRenderbuffers(1, &depth_rb_main);
    //set up depth_rb as a 24-bit fixed-point depth buffer:
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb_main);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_ERRORS();

    //if color framebuffer doesn't have a name, name it and attach textures:
    if (fb_color_main == 0) {
      glGenFramebuffers(1, &fb_color_main);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_color_main);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, color_tex_main, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb_main);
      glDrawBuffer(GL_COLOR_ATTACHMENT0);
      check_fb();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    GL_ERRORS();

    //if outline framebuffer doesn't have a name, name it and attach textures:
    if (fb_outline_main == 0) {
      glGenFramebuffers(1, &fb_outline_main);
      //set up framebuffer: (don't need to do when resizing)
      glBindFramebuffer(GL_FRAMEBUFFER, fb_outline_main);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, normal_tex_main, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, position_tex_main, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb_main);
      GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
      glDrawBuffers(2, bufs);
      check_fb();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL_ERRORS();
  }

  void set_main() {
    normal_tex = normal_tex_main;
    position_tex = position_tex_main;
    color_tex = color_tex_main;
    depth_rb = depth_rb_main;
    fb_color = fb_color_main;
    fb_outline = fb_outline_main;
  }

  void set_sc() {
    normal_tex = normal_tex_sc;
    position_tex = position_tex_sc;
    color_tex = color_tex_sc;
    depth_rb = depth_rb_sc;
    fb_color = fb_color_sc;
    fb_outline = fb_outline_sc;

  }
} fb;

GameLevel::GameLevel(std::string level_name) {

  init_meshes(level_name);

  level_name.insert(level_name.size(), ".scene");

  goals.reserve(2);
  auto load_fn = [this](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &meshes->lookup(mesh_name);

    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;

    //set up drawable to draw mesh from buffer:
    pipeline = flat_program_pipeline;
    pipeline.vao = vao_color;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;

    if (transform->name.substr(0, 4) == "Move") {
      size_t name_len = transform->name.size();
      if (transform->name.substr(name_len-7, 6) == "Screen") {
        std::cout << "Screen detected: " << transform->name << std::endl;
        screens.emplace_back(transform, &pipeline);
        pipeline.set_uniforms = [](){
          glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_TEX);
        };
      } else {
        std::cout << "Movable detected: " << transform->name << std::endl;

        movable_data.emplace_back(transform);
        Movable &data = movable_data.back();
        size_t mi = movable_data.size() - 1;
        data.index = mi;
        pipeline.set_uniforms = [&, mi](){
          glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
          glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(movable_data[mi].color));
        };

        auto f = mesh_to_collider.find(mesh);
        if (f == mesh_to_collider.end()) {
          mesh_colliders.emplace_back(transform, *mesh, *meshes, (int)mi);
        } else {
          mesh_colliders.emplace_back(transform, *f->second, *meshes, (int)mi);
        }
      }
    } else if (transform->name.substr(0, 5) == "Goal1") {
      goals[0] = transform;

      pipeline.smooth_id = 3.0;
      glm::vec4 tmp_color = p1_color;
      pipeline.set_uniforms = [tmp_color](){
        glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
        glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(tmp_color));

      };
    } else if (transform->name.substr(0, 5) == "Goal2") {
      goals[1] = transform;

      pipeline.smooth_id = 4.0;
      glm::vec4 tmp_color = p2_color;
      pipeline.set_uniforms = [tmp_color](){
        glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
        glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(tmp_color));
      };
    } else if (transform->name.substr(0, 5) == "Body1") {
      body_P1_transform = transform;
      body_P1_start = *transform;

      pipeline.smooth_id = 1.0f;
      glm::vec4 tmp_color = p1_color;
      pipeline.set_uniforms = [tmp_color](){
        glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
        glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(tmp_color));
      };
    } else if (transform->name.substr(0, 5) == "Body2") {
      body_P2_transform = transform;
      body_P2_start = *transform;

      pipeline.smooth_id = 2.0f;
      glm::vec4 tmp_color = p2_color;
      pipeline.set_uniforms = [tmp_color](){
        glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_COL);
        glUniform4fv(flat_program->UNIFORM_COLOR_vec4, 1, glm::value_ptr(tmp_color));
      };
    } else {
      auto f = mesh_to_collider.find(mesh);
      if (f == mesh_to_collider.end()) {
        mesh_colliders.emplace_back(transform, *mesh, *meshes);
      } else {
        mesh_colliders.emplace_back(transform, *f->second, *meshes);
      }

      pipeline.set_uniforms = [](){
        glUniform1ui(flat_program->USE_TEX_uint, FlatProgram::USE_VX_COLORS);
      };
      // if (f != mesh_to_collider.end()) {
      //   mesh_colliders.emplace_back(transform, *f->second, *meshes);
      // } else {
      //   decorations++;
      // }
    }
  };
  //Load scene (using Scene::load function), building proper associations as needed:
  load(level_name, load_fn);
  std::cout << "Level scene loaded" << std::endl;

  // Build the mapping between movables and orthographic cameras

  for (auto &oc : orthocams) {
    for (auto &m : movable_data) {
      std::string &xf_name = m.transform->name;
      if (oc.transform->name.substr(0, xf_name.size()) == xf_name) {
        std::cout << "Matched " << xf_name << " to " << oc.transform->name << std::endl;
        standpoints.emplace_back(&oc, &m);
        Standpoint &stpt = standpoints.back();
        std::cout << "\tTexture #" << stpt.tex << std::endl;
        std::list< Light >::iterator lit = lights.begin();
        while (lit != lights.end()) {
          std::string &mp_name = lit->transform->name;
          if (mp_name.substr(0, mp_name.size()-9) == oc.transform->name) {
            std::cout << "Matched " << mp_name << " to " << oc.transform->name << std::endl;
            stpt.move_pos.emplace_back(&(*lit));
            print_vec3(stpt.move_pos.back().pos);
            print_vec4(stpt.move_pos.back().color);
            lights.erase(lit++);
          } else {
            lit++;
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
      if (cam_name == sc_name.substr(0, sc_name.size()-7)) {
        std::cout << "Matched " << cam_name << " to " << sc_name << std::endl;
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

  fb.init_sc();

}

GameLevel::~GameLevel() {
  delete meshes;
}

bool GameLevel::detect_lose() {
  //std::cout << body_P1_transform->position.z << " "<< body_P2_transform->position.z << std::endl;
  return (body_P1_transform->position.z < die_y || body_P2_transform->position.z < die_y);
}

bool GameLevel::detect_win() {
  glm::vec3 goalPos1 = goals[0].transform->position;
  glm::vec3 goalPos2 = goals[1].transform->position;
  glm::vec3 p1 = body_P1_transform->position;
  glm::vec3 p2 = body_P2_transform->position;
  auto dis1 = glm::distance(goalPos1, p1);
  auto dis2 = glm::distance(goalPos2, p2);
  return (dis1 < goals[0].spin_acc) && (dis2 < goals[1].spin_acc);
}

void GameLevel::reset() {
  for (Movable &m : movable_data) {
    m.transform->position = m.init_pos;
    m.color = glm::vec4(1.0f);
    m.players[0] = nullptr;
    m.players[1] = nullptr;
  }
  body_P1_transform->position = body_P1_start.position;
  body_P1_transform->rotation = body_P1_start.rotation;
  body_P2_transform->position = body_P2_start.position;
  body_P2_transform->rotation = body_P2_start.rotation;
}

void GameLevel::draw(
  glm::vec2 const &drawable_size,
  glm::vec3 const &eye,
  glm::mat4 const &world_to_clip
) {

  fb.resize_main(drawable_size);

  screens_standpoints_texture_update(eye);
  if (first_draw) {
    for (auto &stpt : standpoints) {
      //stpt.resize_texture(drawable_size);
      stpt.update_texture(this);
    }
    first_draw = false;
  } else {
    for (auto &sc : screens) {
      if (sc.draw && !sc.stpt->updated) {
        //sc.stpt->resize_texture(drawable_size);
        sc.stpt->update_texture(this);
      }
    }
  }

  fb.set_main();
  draw_fb(eye, world_to_clip, 0);

}

void GameLevel::draw_fb(
  glm::vec3 const &eye,
  glm::mat4 const &world_to_clip,
  GLuint output_fb
) {
  GL_ERRORS();
  // Color drawing

  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_color);
  GLfloat bg_color[4] = {0.95f, 0.95f, 1.0f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, bg_color);
  glClear(GL_DEPTH_BUFFER_BIT);

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  Scene::draw(world_to_clip);

  GL_ERRORS();

  // Draw the outlines
  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_outline);
  GLfloat bg_normal[4] = {0.0f, 0.0f, 0.0f, 0.5f};
  glClearBufferfv(GL_COLOR, 0, bg_normal);
  GLfloat bg_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  glClearBufferfv(GL_COLOR, 1, bg_pos);

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glUseProgram(outline_program_0->program);
  glBindVertexArray(vao_outline);

  for (auto const &drawable : drawables) {

    assert(drawable.transform); //drawables *must* have a transform
    glm::mat4 object_to_world = drawable.transform->make_local_to_world();

    if (outline_program_0->OBJECT_TO_WORLD_mat4 != -1U) {
      glUniformMatrix4fv(outline_program_0->OBJECT_TO_WORLD_mat4, 1, GL_FALSE, glm::value_ptr(object_to_world));
    }
    if (outline_program_0->OBJECT_TO_CLIP_mat4 != -1U) {
      glm::mat4 object_to_clip = world_to_clip * object_to_world;
      glUniformMatrix4fv(outline_program_0->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(object_to_clip));
    }

    if (outline_program_0->OBJECT_SMOOTH_ID_float != -1U) {
      glUniform1f(outline_program_0->OBJECT_SMOOTH_ID_float, drawable.pipeline.smooth_id);
    }
    // Uses the same pipeline as flat coloring
    Scene::Drawable::Pipeline const &pipeline = drawable.pipeline;
    glDrawArrays(pipeline.type, pipeline.start, pipeline.count);

  }
  GL_ERRORS();
  // Draw to screen
  glBindFramebuffer(GL_FRAMEBUFFER, output_fb);
  GLfloat bg_out[4] = {0.5f, 0.5f, 0.5f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, bg_out);
  glDisable(GL_DEPTH_TEST);

  glUseProgram(outline_program_1->program);
  glBindVertexArray(vao_empty);

  if (outline_program_1->EYE_vec3 != -1U) {
    glUniform3fv(outline_program_1->EYE_vec3, 1, glm::value_ptr(eye));
  }

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
  if (output_fb != 0) glBindFramebuffer(GL_FRAMEBUFFER, 0);
  GL_ERRORS();

}

GameLevel::Movable::Movable(Transform *transform_) : transform(transform_) {

  init_pos = transform->position;
  target_pos = init_pos;
  color = glm::vec4(1.0f);
  print_vec3(target_pos);
  print_vec4(color);

}

void GameLevel::Movable::update(glm::vec3 const &diff) {
  transform->position += diff;
  if (players[0]) players[0]->position += diff;
  if (players[1]) players[1]->position += diff;
}

void GameLevel::Movable::add_player(Transform *pl, uint32_t player_num) {
  players[player_num % 2] = pl;
}

void GameLevel::Movable::remove_player(uint32_t player_num) {
  players[player_num % 2] = nullptr;
}

void GameLevel::Movable::set_target_pos(glm::vec3 const &target, glm::vec4 const &color_) {

  color = color_;
  target_pos = target;
  std::cout << "Set to:" << std::endl;
  print_vec3(target_pos);
  print_vec4(color);

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
  resize_texture(glm::uvec2(640, 480));

}

void GameLevel::Standpoint::resize_texture(glm::uvec2 const &new_size) {

  if (size == new_size) return;
  size = new_size;

  std::cout << "Resizing screen texture #" << tex << std::endl;

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

  // std::cout << "Drawing screen texture #" << tex << std::endl;

  float tex_h = cam->scale;
  float tex_w = (w / h) * tex_h;
  float fp = cam->clip_far;
  float np = cam->clip_near;

  glm::mat4 proj = glm::ortho(-tex_w, tex_w, -tex_h, tex_h, np ,fp);
  glm::mat4 w2l = cam->transform->make_world_to_local();

  glBindFramebuffer(GL_FRAMEBUFFER, fb.fb_output_sc);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  check_fb();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  GL_ERRORS();

  fb.set_sc();
  level->draw_fb(pos, proj * w2l, fb.fb_output_sc);
  updated = true;

}

glm::vec2 GameLevel::Standpoint::movable_center_to_screen() {
  glm::vec4 pos = (
    cam->make_projection() * (
      cam->transform->make_world_to_local() * (
        movable->transform->make_local_to_world()
        * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
  ) ) );
  return glm::vec2(pos.x, pos.y);
}

void GameLevel::Standpoint::move_to(size_t i) {
  movable->set_target_pos(move_pos[i].pos, move_pos[i].color);
}

GameLevel::Standpoint::MovePosition::MovePosition(Light *light) {
  transform = light->transform;
  color = glm::vec4(light->color, 1.0f);
  pos = transform->position;
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

GameLevel::Screen *GameLevel::screen_get(Transform const *transform) {

  glm::vec3 pos = transform->make_local_to_world()[3]; // * (0,0,0,1)
  glm::vec3 axis = transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
  if (axis == glm::vec3(0.0f)) return nullptr;
  axis = glm::normalize(axis);

  //std::cout << "Pos: " << pos.x << "\t" << pos.y << "\t" << pos.z << std::endl;
  //std::cout << "Axis: " << axis.x << "\t" << axis.y << "\t" << axis.z << std::endl;

  for (Screen &sc : screens) {
    //std::cout << "Screen: " << sc.pos.x << "\t" << sc.pos.y << "\t" << sc.pos.z << std::endl;

    glm::vec3 dist = sc.pos - pos;
    //std::cout << glm::dot(dist, dist) << std::endl;
    if (glm::dot(dist, dist) <= sc.pos_tolerance * sc.pos_tolerance) {
      //std::cout << "Position within threshhold. Checking axis..." << std::endl;
      //std::cout << "Dot: " << glm::dot(axis, dist) << std::endl;
      if (glm::dot(axis, glm::normalize(dist)) > sc.axis_tolerance) {
        return &sc;
      }
    }
  }

  return nullptr;

}

void GameLevel::screens_standpoints_texture_update(glm::vec3 const &pos) {

  for (auto &stpt : standpoints) {
    stpt.updated = false;
  }

  for (Screen &sc : screens) {
    glm::vec3 dist = sc.pos - pos;
    sc.draw = (glm::dot(dist, dist) <= sc.draw_distance * sc.draw_distance);
  }

}
