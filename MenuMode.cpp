#include "MenuMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for easy sprite drawing:
#include "DrawSprites.hpp"

//for playing movement sounds:
#include "Sound.hpp"
#include "data_path.hpp"
//for loading:
#include "Load.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <random>

Load< Sound::Sample > music_ambient(LoadTagDefault, []() -> Sound::Sample * {
  return new Sound::Sample(data_path("ambient.wav"));
});

Load< Sound::Sample > sound_move(LoadTagDefault, []() -> Sound::Sample * {
  return new Sound::Sample(data_path("movesh.wav"));
});

Load< Sound::Sample > sound_win(LoadTagDefault, []() -> Sound::Sample * {
  return new Sound::Sample(data_path("ding.wav"));
});

Load< Sound::Sample > sound_click(LoadTagDefault, []() -> Sound::Sample *{
	std::vector< float > data(size_t(48000 * 0.2f), 0.0f);
	for (uint32_t i = 0; i < data.size(); ++i) {
		float t = i / float(48000);
		//phase-modulated sine wave (creates some metal-like sound):
		data[i] = std::sin(3.1415926f * 2.0f * 440.0f * t + std::sin(3.1415926f * 2.0f * 450.0f * t));
		//quadratic falloff:
		data[i] *= 0.3f * std::pow(std::max(0.0f, (1.0f - t / 0.2f)), 2.0f);
	}
	return new Sound::Sample(data);
});

Load< Sound::Sample > sound_clonk(LoadTagDefault, []() -> Sound::Sample *{
	std::vector< float > data(size_t(48000 * 0.2f), 0.0f);
	for (uint32_t i = 0; i < data.size(); ++i) {
		float t = i / float(48000);
		//phase-modulated sine wave (creates some metal-like sound):
		data[i] = std::sin(3.1415926f * 2.0f * 220.0f * t + std::sin(3.1415926f * 2.0f * 200.0f * t));
		//quadratic falloff:
		data[i] *= 0.3f * std::pow(std::max(0.0f, (1.0f - t / 0.2f)), 2.0f);
	}
	return new Sound::Sample(data);
});

std::unique_ptr< PlayerMode > MenuMode::current;

void MenuMode::start_game(uint32_t level_num) {
  if (menu_player == MENU_PLAYER_SOLO) {
    current.reset(new SinglePlayerMode(level_num));
  } else if (current && current->connect != nullptr) {
    current->pause = false;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    current->level_change(level_num);
  } else if (menu_player == MENU_PLAYER_SERVER) {
    current.reset(new ServerMode("12345", level_num, selected_player));
  } else if (menu_player == MENU_PLAYER_CLIENT) {
    current.reset(new ClientMode(stages[MENU_IP].items[1].name, "12345", level_num, selected_player));
  }
	
}

MenuMode::MenuMode() {

  current = nullptr;

  { // Main Menu
    Stage &stage = stages[MENU_MAIN];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ V ]]"));
    items.emplace_back(Item::NavigationButtonItem("Play Solo", this, MENU_START));
    items.emplace_back(Item::NavigationButtonItem("Play Co-op", this, MENU_CONNECT));
    items.emplace_back(Item::ExitButtonItem());
    stage.set_items();
  }
  { // Connect Menu
    Stage &stage = stages[MENU_CONNECT];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ Co-op ]]"));
    items.emplace_back(Item::NavigationButtonItem(
      "Create Game", this, MENU_PLAYER,
      [this](Item const &) {
        menu_player = MENU_PLAYER_SERVER;
      }
    ));
    items.emplace_back(Item::NavigationButtonItem("Join Game", this, MENU_IP));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }
  { // IP Menu
    Stage &stage = stages[MENU_IP];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ Join Network Game ]]"));
    items.emplace_back(Item::TextItem("Enter IP Address:"));
    items.emplace_back(Item::TextEntryItem(
      "localhost",
      [](SDL_Keycode keysym) {
        return keysym == SDLK_PERIOD || keysym >= SDLK_0 && keysym <= SDLK_9;
      }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Connect", this, MENU_PLAYER,
      [](Item const &) {
        
      }
    ));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }
  { // Player Menu
    Stage &stage = stages[MENU_PLAYER];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ Select Player ]]"));
    items.emplace_back(Item::NavigationButtonItem(
      "Player 1", this, MENU_START,
      [this](Item const &) {
        selected_player = 1;
      }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Player 2", this, MENU_START,
      [this](Item const &) {
      selected_player = 2;
    }
    ));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }
  { // Start Menu
    Stage &stage = stages[MENU_START];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ Game Menu ]]"));
    items.emplace_back(Item::NavigationButtonItem(
      "Start Game", this, MENU_PAUSE, [this](Item const &) { start_game(1); }
    ));
    items.emplace_back(Item::NavigationButtonItem("Select Level", this, MENU_LEVEL));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }
  { // Level Menu
    Stage &stage = stages[MENU_LEVEL];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ Level Selection ]]"));
    items.emplace_back(Item::NavigationButtonItem(
      "Level 1", this, MENU_PAUSE, [this](Item const &) { start_game(1); }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Level 2", this, MENU_PAUSE, [this](Item const &) { start_game(2); }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Level 3", this, MENU_PAUSE, [this](Item const &) { start_game(3); }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Level 4", this, MENU_PAUSE, [this](Item const &) { start_game(4); }
    ));
    items.emplace_back(Item::NavigationButtonItem(
      "Level 5", this, MENU_PAUSE, [this](Item const &) { start_game(5); }
    ));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }
  { // Pause Menu
    Stage &stage = stages[MENU_PAUSE];
    std::vector< Item > &items = stage.items;
    items.emplace_back("[[ PAUSED ]]");
    items.emplace_back("Resume");
    items.back().on_select = [&](Item const &){
      if (current) {
        current->pause = false;
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    };
    items.emplace_back("Reset");
    items.back().on_select = [&](Item const &){
      if (current) {
        current->handle_reset();
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    };
    items.emplace_back(Item::NavigationButtonItem("Controls", this, MENU_HELP));
    items.emplace_back(Item::NavigationButtonItem("Main Menu", this, MENU_START));
    items.emplace_back(Item::ExitButtonItem());
    stage.set_items(1);
  }
  { // Help Menu
    Stage &stage = stages[MENU_HELP];
    std::vector< Item > &items = stage.items;
    items.emplace_back(Item::TextItem("[[ CONTROLS ]]"));
    items.emplace_back(Item::TextItem("WASD to move"));
    items.emplace_back(Item::TextItem("Space to jump"));
    items.emplace_back(Item::TextItem("LCtrl to sprint"));
    items.emplace_back(Item::TextItem("Q to switch player (solo mode)"));
    items.emplace_back(Item::BackButtonItem(this));
    stage.set_items();
  }

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);
		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);
		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		//set up the vertex array object to describe arrays of FroggerMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]
		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);
		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);
		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);
		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

}

MenuMode::~MenuMode() {
}

bool MenuMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

  if (current && !current->pause) {

    // Reset if R is pressed
    if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_R) {
      if (current) {
        current->handle_reset();
        SDL_SetRelativeMouseMode(SDL_TRUE);
      }
    }
    // Check UI elements:
    // TODO: find a way to not keep calling screen_get? Same issue in draw_ui.
    else if (evt.type == SDL_MOUSEBUTTONUP && evt.button.button == SDL_BUTTON_LEFT) {
      if (current->shift.progress == 1.0f) {
      // Player is completed shift mode: check color wheel interaction
        GameLevel::Standpoint *stpt = current->shift.sc->stpt;
        glm::vec2 mpos = glm::vec2(evt.button.x, window_size.y - evt.button.y);
        glm::vec3 center_clip = glm::vec3(current->shift.sc->stpt->movable_center_to_screen(), 1.0f);
        view_to_drawable = glm::vec2(
          window_size.x / (view_max.x - view_min.x),
          window_size.y / (view_max.y - view_min.y)
        );
        glm::vec2 center = view_to_drawable * (clip_to_court * center_clip);
        // TODO: Make this a function of wheel_radius
        float threshold = window_size.y / 15.0f;
        glm::vec2 dist = mpos - center;
        if (glm::dot(dist, dist) < threshold * threshold) {
          // TODO: Make this work for multiple colors
          float angle = -glm::atan(dist.y, dist.x);
          if (angle < 0.0f) angle += 2.0f * 3.1415926f;
          float sector_angle = 2.0f * 3.1415926f / (float) stpt->move_pos.size();
          size_t index = (size_t) (angle / sector_angle);
          stpt->move_to(index);
          current->currently_moving.emplace_back(stpt->movable->index);
        }
      }
    } else if (current->handle_event(evt, window_size)) {
    } else return false;
  } else {
    if (evt.type == SDL_KEYDOWN) {
      if (active_item) {
        if (evt.key.keysym.sym == SDLK_ESCAPE) {
          active_item = nullptr;
        } else if (active_item->event_handler(*active_item, evt)) {
        } else return false;
      } else {
        if (evt.key.keysym.sym == SDLK_UP || evt.key.keysym.scancode == SDL_SCANCODE_W) {
          for (uint32_t i = curr_select() - 1; i < curr_items().size(); --i) {
            if (curr_items()[i].on_select || curr_items()[i].event_handler) { //skip non-selectable items
              curr_select() = i; break;
            }
          }
        } else if (evt.key.keysym.sym == SDLK_DOWN || evt.key.keysym.scancode == SDL_SCANCODE_S) {
          for (uint32_t i = curr_select() + 1; i < curr_items().size(); ++i) {
            if (curr_items()[i].on_select || curr_items()[i].event_handler) {
              curr_select() = i; break;
            }
          }
        } else if (evt.key.keysym.sym == SDLK_RETURN || evt.key.keysym.sym == SDLK_SPACE) {
          if (curr_item().on_select) {
            curr_item().on_select(curr_item());
          } else if (curr_item().event_handler) {
            active_item = &curr_item();
          }
        } else if (evt.key.keysym.sym == SDLK_ESCAPE) {
          Item &it = curr_items()[curr_stage().escape];
          it.on_select(it);
        } else return false;
      }
    } else return false;
  }

  return true;
  
}

void MenuMode::update(float elapsed) {

	select_bounce_acc = select_bounce_acc + elapsed / 0.7f;
	select_bounce_acc -= std::floor(select_bounce_acc);

	if (!background_music || background_music->stopped){
    background_music = Sound::play(*music_ambient, 1.0f);
  }

	if (current) {
    current->play_moving_sound = 0;
    current->update(elapsed);
    if (!current->currently_moving.empty() || current->play_moving_sound){
      if (!moving_sound || moving_sound->stopped) {
        moving_sound = Sound::play(*sound_move, 2.0f);
      }
    } else if (moving_sound && !moving_sound->stopped) {
      moving_sound->stop();
    }

		if ((current->we_reached_goal)&& (!we_just_reached_goal)){
			win_sound = Sound::play(*sound_win, 2.0f);
			we_just_reached_goal = true;
		} else if (!current->we_reached_goal){
			we_just_reached_goal = false;
		}

    if (current->won) {
      current->to_next_level += elapsed;
      // std::cout << current->to_next_level << std::endl;
      if (current->to_next_level >= 5.0f) {
        uint32_t level_num = (current->level_num == 5) ? 1 : current->level_num + 1;
        current->level_change(level_num);
      }
  	}
  }
}

void MenuMode::draw_menu(glm::uvec2 const &drawable_size, MenuMode::Stage const &stage) {

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	float bounce = (0.25f - (select_bounce_acc - 0.5f) * (select_bounce_acc - 0.5f)) / 0.25f * select_bounce_amount;

	{ //draw the menu using DrawSprites:
		assert(atlas && "it is an error to try to draw a menu without an atlas");
		DrawSprites draw_sprites(*atlas, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);

		for (uint32_t i = 0; i < stage.items.size(); i++) {
      MenuMode::Item const &item = stage.items[i];
			bool is_selected = i == stage.selected;
			glm::u8vec4 color = (is_selected ? item.selected_tint : item.tint);
			float left, right;
			if (!item.sprite) {
				//draw item.name as text:
				draw_sprites.draw_text(
					item.name, item.at, item.scale, color
				);
				glm::vec2 min,max;
				draw_sprites.get_text_extents(
					item.name, item.at, item.scale, &min, &max
				);
				left = min.x;
				right = max.x;
			} else {
				draw_sprites.draw(*item.sprite, item.at, item.scale, color);
				left = item.at.x + item.scale * (item.sprite->min_px.x - item.sprite->anchor_px.x);
				right = item.at.x + item.scale * (item.sprite->max_px.x - item.sprite->anchor_px.x);
			}
			if (is_selected) {
				if (left_select) {
					draw_sprites.draw(*left_select, glm::vec2(left - bounce, item.at.y) + left_select_offset, item.scale, left_select_tint);
				}
				if (right_select) {
					draw_sprites.draw(*right_select, glm::vec2(right + bounce, item.at.y) + right_select_offset, item.scale, right_select_tint);
				}
			}

		}
	} //<-- gets drawn here!
}

void MenuMode::draw_ui(glm::uvec2 const &drawable_size) {
  assert(current);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  std::vector< Vertex > vertices;
  auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
    //split rectangle into two CCW-oriented triangles:
    vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

    vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
  };
  /* auto draw_triangle = [&vertices](glm::vec2 const &a, glm::vec2 const &b, glm::vec2 const &c, glm::u8vec4 const &color) {
    //a, b, c should be CCW-oriented. BUGGY?
    vertices.emplace_back(glm::vec3(a.x, a.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(b.x, b.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    vertices.emplace_back(glm::vec3(c.x, c.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
  }; */
  // TODO: draw_circle_sector now handles angle and angle_offset in clockwise direction
  auto draw_circle_sector = [&vertices](glm::vec2 const &center, float const &radius, float angle_offset, float angle, glm::u8vec4 const &color) {
    static const uint8_t sides = 36;
    static const float x_offsets[sides+1] =
      { 1.0f,  0.98480775301f,  0.93969262078f,  0.86602540378f, 0.76604444311f, 0.64278760968f,
        0.5f, 0.34202014332f, 0.17364817766f, 0.0f, -0.17364817766f, -0.34202014332f,
        -0.5f, -0.64278760968f, -0.76604444311f, -0.86602540378f, -0.93969262078f, -0.98480775301f,
        -1.0f, -0.98480775301f, -0.93969262078f, -0.86602540378f, -0.76604444311f, -0.64278760968f,
        -0.5f, -0.34202014332f, -0.17364817766f, 0.0f, 0.17364817766f, 0.34202014332f,
        0.5f, 0.64278760968f, 0.76604444311f, 0.86602540378f, 0.93969262078f, 0.98480775301f, 1.0f
      };
    static const float y_offsets[sides+1] =
      { 0.0f, -0.17364817766f, -0.34202014332f, -0.5f, -0.64278760968f, -0.76604444311f,
        -0.86602540378f, -0.93969262078f, -0.98480775301f, -1.0f, -0.98480775301f, -0.93969262078f,
        -0.86602540378f, -0.76604444311f, -0.64278760968f, -0.5f, -0.34202014332f, -0.17364817766f,
        0.0f, 0.17364817766f, 0.34202014332f, 0.5f, 0.64278760968f, 0.76604444311f,
        0.86602540378f, 0.93969262078f, 0.98480775301f, 1.0f,  0.98480775301f,  0.93969262078f,
        0.86602540378f, 0.76604444311f, 0.64278760968f, 0.5f, 0.34202014332f, 0.17364817766f, 0.0f
      };
    //just draw a <sides>-gon with CCW-oriented triangles:
    uint8_t start = (uint8_t)(angle_offset/10.0f);
    uint8_t end = (uint8_t)((angle_offset+angle)/10.0f);
    for (uint8_t i = start; i < end; i++) {
      vertices.emplace_back(glm::vec3(center.x+x_offsets[i]*radius, center.y+y_offsets[i]*radius, 0.0f), color, glm::vec2(0.5f, 0.5f));
      vertices.emplace_back(glm::vec3(center.x+x_offsets[i+1]*radius, center.y+y_offsets[i+1]*radius, 0.0f), color, glm::vec2(0.5f, 0.5f));
      vertices.emplace_back(glm::vec3(center.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    }
  };

  float aspect = drawable_size.x / float(drawable_size.y); //compute window aspect ratio: (scale the x coordinate by 1.0 / aspect to make sure things stay square.)
  float scale = std::min( //compute scale factor for court given that...
    (2.0f * aspect) / (view_max.x - view_min.x), //... x must fit in [-aspect,aspect] ...
    (2.0f) / (view_max.y - view_min.y) //... y must fit in [-1,1].
  );
  glm::vec2 center = 0.5f * (view_max + view_min);
  glm::vec2 textbox_border  = glm::vec2(1,1);
  glm::vec2 textbox_padding = glm::vec2(5,2);

  glm::u8vec4 black = glm::u8vec4(0,0,0,255);
  glm::u8vec4 white = glm::u8vec4(255,255,255,255);
  glm::u8vec4 gray = glm::u8vec4(30,30,30,255);

  assert(atlas && "it is an error to try to draw a menu without an atlas");
  DrawSprites draw_sprites(*atlas, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);

  glm::mat4 court_to_clip = glm::mat4(
    glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, scale, 0.0f, 0.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
    glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
  );
  clip_to_court = glm::mat3x2(
    glm::vec2(aspect / scale, 0.0f),
    glm::vec2(0.0f, 1.0f / scale),
    glm::vec2(center.x, center.y)
  );

  // TODO: find a way to not keep calling screen_get? Same issue in handle_event.
  if (current->level->screen_get(current->pov.camera->transform) && current->shift.progress == 0.0f) {
    // Player is in position to shift but hasn't started it: draw LSHIFT prompt
    // TODO: shift entire textbox drawing to another function?
    glm::vec2 textbox_center = glm::vec2(0.5f*(view_min.x+view_max.x), 0.2f*(view_min.y+view_max.y));
    std::string text = "LSHIFT";  float text_scale = 0.7f; glm::vec2 text_min, text_max;
    draw_sprites.get_text_extents(text, textbox_center, text_scale, &text_min, &text_max);
    glm::vec2 textbox_radius  = glm::vec2(0.5*(text_max.x-text_min.x)+textbox_padding.x, 0.5*(text_max.y-text_min.y)+textbox_padding.y);
    glm::vec2 text_offset = glm::vec2(0.5f*(text_min.x-text_max.x), text_min.y-text_max.y);

    draw_rectangle(textbox_center, textbox_radius+textbox_border, black);
    draw_rectangle(textbox_center, textbox_radius, white);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(color_texture_program.program); //set color_texture_program as current program:
    glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip)); //upload OBJECT_TO_CLIP to the proper uniform location:
    glBindVertexArray(vertex_buffer_for_color_texture_program); //use the mapping vertex_buffer_for_color_texture_program to fetch vertex data
    glActiveTexture(GL_TEXTURE0); //bind the solid white texture to location zero:
    glBindTexture(GL_TEXTURE_2D, white_tex);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size())); //run the OpenGL pipeline
    glBindTexture(GL_TEXTURE_2D, 0); //unbind the solid white texture
    glBindVertexArray(0); //reset vertex array to none
    glUseProgram(0); //reset current program to none

    draw_sprites.draw_text(text, textbox_center+text_offset, text_scale, black);
  } else if (current->shift.progress == 1.0f) {
    GameLevel::Standpoint *stpt = current->shift.sc->stpt;
    // Shift is complete: draw color wheel UI
    // TODO: Move constants to somewhere more reasonable, pull position from movable target
    // Scene::Transform *movable_transform = current->shift.sc->stpt->movable->transform;
    glm::vec3 wheel_center_clip = glm::vec3(current->shift.sc->stpt->movable_center_to_screen(), 1.0f);
    glm::vec2 wheel_center = clip_to_court * wheel_center_clip;
    float wheel_radius = 15.0f; //NOTE: view_max = (320,200)
    float border = 0.5f;
    glm::vec2 spoke_radius = glm::vec2(2, 2); // TODO: make the wheel size invariant to window size

    draw_circle_sector(wheel_center, wheel_radius+border, 0.0f, 360.0f, gray); // Border
    float sector_angle = 360.0f / (float) stpt->move_pos.size();
    for (size_t i = 0; i < stpt->move_pos.size(); i++) {
      draw_circle_sector(
        wheel_center, wheel_radius,
        i * sector_angle, sector_angle,
        glm::u8vec4(255.0f * stpt->move_pos[i].color)
      );
    }
    //draw_circle_sector(wheel_center, wheel_radius, 0.0f, 180.0f, cyan); // Bottom color
    //draw_circle_sector(wheel_center, wheel_radius, 180.0f, 180.0f, yellow); // Top color
    draw_rectangle(wheel_center, spoke_radius, black); // Center spoke

  	//build matrix that scales and translates appropriately:
  	// glm::mat4 court_to_clip = glm::mat4(
  	// 	glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
  	// 	glm::vec4(0.0f, scale, 0.0f, 0.0f),
  	// 	glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
  	// 	glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
  	// );
  	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
  	// clip_to_court = glm::mat3x2(
  	// 	glm::vec2(aspect / scale, 0.0f),
  	// 	glm::vec2(0.0f, 1.0f / scale),
  	// 	glm::vec2(center.x, center.y)
  	// );
  	//upload vertices to vertex_buffer:
  	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
  	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
  	glBindBuffer(GL_ARRAY_BUFFER, 0);
  	glUseProgram(color_texture_program.program); //set color_texture_program as current program:
  	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip)); //upload OBJECT_TO_CLIP to the proper uniform location:
  	glBindVertexArray(vertex_buffer_for_color_texture_program); //use the mapping vertex_buffer_for_color_texture_program to fetch vertex data
  	glActiveTexture(GL_TEXTURE0); //bind the solid white texture to location zero:
  	glBindTexture(GL_TEXTURE_2D, white_tex);
  	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size())); //run the OpenGL pipeline
  	glBindTexture(GL_TEXTURE_2D, 0); //unbind the solid white texture
  	glBindVertexArray(0); //reset vertex array to none
  	glUseProgram(0); //reset current program to none
  }

  if (current->won || current->lost || current->we_want_reset || current->they_want_reset) {
    glm::vec2 textbox_center;
    if (current->won || current->lost) { textbox_center = 0.5f * (view_min + view_max); }
    else { textbox_center = glm::vec2(0.5f*(view_min.x+view_max.x), 0.2f*(view_min.y+view_max.y)); }
    std::string text; float text_scale;
    if (current->won)                { text = "You Won!"; text_scale = 0.9f; }
    else if (current->lost)          { text = "Game Over. Press R to reset"; text_scale = 0.7f; }
    else if (current->we_want_reset) { text = "Waiting for other player to reset..."; text_scale = 0.7f; }
    else                             { text = "Reset request received"; text_scale = 0.7f; }
    glm::vec2 text_min, text_max;
    draw_sprites.get_text_extents(text, textbox_center, text_scale, &text_min, &text_max);
    glm::vec2 textbox_radius  = glm::vec2(0.5*(text_max.x-text_min.x)+textbox_padding.x, 0.5*(text_max.y-text_min.y)+textbox_padding.y);
    glm::vec2 text_offset = glm::vec2(0.5f*(text_min.x-text_max.x), 0.5*(text_min.y-text_max.y));

    draw_rectangle(textbox_center, textbox_radius+textbox_border, black);
    draw_rectangle(textbox_center, textbox_radius, white);
    // glm::mat4 court_to_clip = glm::mat4(
    //   glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
    //   glm::vec4(0.0f, scale, 0.0f, 0.0f),
    //   glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
    //   glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
    // );
    // clip_to_court = glm::mat3x2(
    //   glm::vec2(aspect / scale, 0.0f),
    //   glm::vec2(0.0f, 1.0f / scale),
    //   glm::vec2(center.x, center.y)
    // );
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(color_texture_program.program); //set color_texture_program as current program:
    glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip)); //upload OBJECT_TO_CLIP to the proper uniform location:
    glBindVertexArray(vertex_buffer_for_color_texture_program); //use the mapping vertex_buffer_for_color_texture_program to fetch vertex data
    glActiveTexture(GL_TEXTURE0); //bind the solid white texture to location zero:
    glBindTexture(GL_TEXTURE_2D, white_tex);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size())); //run the OpenGL pipeline
    glBindTexture(GL_TEXTURE_2D, 0); //unbind the solid white texture
    glBindVertexArray(0); //reset vertex array to none
    glUseProgram(0); //reset current program to none

    // if (current->won) { draw_sprites.draw_text("You Won!", textbox_center+text_offset, 0.9f, black); }
    // else if (current->lost) { draw_sprites.draw_text("Game Over", textbox_center+text_offset, 0.9f, black); }
    // else if (current->we_want_reset) { draw_sprites.draw_text("Waiting for other player to reset...", textbox_center+text_offset, 0.7f, black); }
    // else if (current->they_want_reset) { draw_sprites.draw_text("Reset request received", textbox_center+text_offset, 0.7f, black); }
    draw_sprites.draw_text(text, textbox_center+text_offset, text_scale, black);
  }
}

void MenuMode::draw(glm::uvec2 const &drawable_size) {
	if (current) {
		std::shared_ptr< Mode > hold_me = shared_from_this();
    if (current->pause) {
      // draw menu
      draw_menu(drawable_size, curr_stage());
    } else {
      // draw level
      current->draw(drawable_size);
      draw_ui(drawable_size);
    }
		//it is an error to remove the last reference to this object in current->draw():
		assert(hold_me.use_count() > 1);
	} else {
    draw_menu(drawable_size, curr_stage());
  }

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
}

void MenuMode::layout_items(float gap) {
	//DrawSprites temp(*atlas, view_min, view_max, view_max - view_min, DrawSprites::AlignPixelPerfect); //<-- doesn't actually draw
  
  for (uint32_t stg = 0; stg < MENU_COUNT; stg++) {
    std::vector< Item > &items = stages[stg].items;
    float y = view_max.y - 50;
  	for (auto &item : items) {
      y -= 15;
      item.at.y = y;
      item.at.x = 50;
      /*
  		glm::vec2 min, max;
  		if (item.sprite) {
  			min = item.scale * (item.sprite->min_px - item.sprite->anchor_px);
  			max = item.scale * (item.sprite->max_px - item.sprite->anchor_px);
  		} else {
  			temp.get_text_extents(item.name, glm::vec2(0.0f), item.scale, &min, &max);
  		}
  		item.at.y = y - max.y;
  		item.at.x = 0.5f * (view_max.x + view_min.x) - 0.5f * (max.x + min.x);
  		y = y - (max.y - min.y) - gap;
  	}
  	//float ofs = -0.5f * y;
  	//for (auto &item : items) {
  		//item.at.y += ofs;
  	//} */
    }
  }

}

void MenuMode::Stage::set_items() {
  for (uint32_t last_item = (uint32_t) items.size() - 1; last_item < items.size(); last_item--) {
    if (items[last_item].on_select) {
      set_items(last_item);
      break;
    }
  }
}

void MenuMode::Stage::set_items(uint32_t esc_item) {

  for (uint32_t first_item = 0; first_item < items.size(); first_item++) {
    if (items[first_item].on_select || items[first_item].event_handler) {
      selected = first_item;
      break;
    }
  }

  if (items[esc_item].on_select) escape = esc_item;
  else set_items();

}

MenuMode::Item MenuMode::Item::TextItem(std::string const& name_) {
  return MenuMode::Item(name_, nullptr, nullptr, nullptr);
}

MenuMode::Item MenuMode::Item::TextButtonItem(
  std::string const& name_,
  std::function< void(Item const&) > on_select_
) {
  return MenuMode::Item(name_, nullptr, nullptr, on_select_);
}

MenuMode::Item MenuMode::Item::TextEntryItem(
  std::string const& name_,
  std::function< bool(SDL_Keycode) > validate_input
) {

  auto event_handler = [validate_input](Item &item, SDL_Event const &evt) {

    if (validate_input(evt.key.keysym.sym)) {
      item.name.push_back((char)evt.key.keysym.sym);
      return true;
    } else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
      if (!item.name.empty()) item.name.pop_back();
      return true;
    }
    return false;

  };

  return MenuMode::Item(name_, nullptr, event_handler, nullptr);
}

MenuMode::Item MenuMode::Item::NavigationButtonItem(
  std::string const& name_,
  MenuMode *menu,
  MenuStage stage_to,
  std::function< void(Item &) > on_select
) {

  std::function< void(Item &) > select_fn = [menu, stage_to, on_select](Item &item) {
    if (on_select) on_select(item);
    menu->stages[stage_to].stage_from = menu->stage_name;
    menu->stage_name = stage_to;
  };

  return MenuMode::Item(name_, nullptr, nullptr, select_fn);

}

MenuMode::Item MenuMode::Item::BackButtonItem(MenuMode *menu) {

  auto on_select = [menu](Item &item) {
    std::cout << menu->curr_stage().stage_from << std::endl;
    menu->stage_name = menu->curr_stage().stage_from;
  };

  return MenuMode::Item("Back", nullptr, nullptr, on_select);

}

MenuMode::Item MenuMode::Item::ExitButtonItem() {

  auto on_select = [](Item &item) {
    Mode::set_current(nullptr);
  };

  return MenuMode::Item("Exit Game", nullptr, nullptr, on_select);

}