#include "MenuMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for easy sprite drawing:
#include "DrawSprites.hpp"

//for playing movement sounds:
#include "Sound.hpp"

//for loading:
#include "Load.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <random>

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

std::shared_ptr< PlayerMode > MenuMode::current;

void MenuMode::set_current(std::shared_ptr< PlayerMode > const &new_current) {
	current = new_current;
}
std::shared_ptr< PlayerMode > MenuMode::get_current() {
	return current;
}

MenuMode::MenuMode(std::vector< Item > const &main_items_) : main_items(main_items_){
	/*//select first item which can be selected:
	for (uint32_t i = 0; i < items.size(); ++i) {
		if (items[i].on_select) {
			selected = i; break;
		}
	} */
  current = nullptr;

	pause_items.emplace_back("[[ PAUSED ]]");
	pause_items.emplace_back("Resume");
	pause_items.back().on_select = [&](Item const &){
    // current = MenuMode::get_current();
    if (current) { current->pause = false; SDL_SetRelativeMouseMode(SDL_TRUE); }
	};
	pause_items.emplace_back("Reset");
	pause_items.back().on_select = [&](Item const &){
    if (current) {
      current->we_want_reset = true;
      current->reset_countdown = 0.01f;
      current->connect->send('R');
      std::cout << "Requested reset" << std::endl;
      // TODO just call resume at the end
      current->pause = false;
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
	};
	pause_items.emplace_back("Main Menu");
	pause_items.back().on_select = [](Item const &){
    MenuMode::set_current(nullptr);
	};
	pause_items.emplace_back("Quit");
	pause_items.back().on_select = [](Item const &){
    Mode::set_current(nullptr);
	};


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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	if (current) {
    if (current->pause) {
      // we're on the pause menu
    	if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_ESCAPE){
          current->pause = false;
          SDL_SetRelativeMouseMode(SDL_TRUE);
        }
    		if (evt.key.keysym.sym == SDLK_UP || evt.key.keysym.scancode == SDL_SCANCODE_W) {
    			//skip non-selectable items:
    			for (uint32_t i = selected - 1; i < pause_items.size(); --i) {
    				if (pause_items[i].on_select) {
    					selected = i;
    					Sound::play(*sound_click);
    					break;
    				}
    			}
    			return true;
    		} else if (evt.key.keysym.sym == SDLK_DOWN || evt.key.keysym.scancode == SDL_SCANCODE_S) {
    			//note: skips non-selectable items:
    			for (uint32_t i = selected + 1; i < pause_items.size(); ++i) {
    				if (pause_items[i].on_select) {
    					selected = i;
    					Sound::play(*sound_click);
    					break;
    				}
    			}
    			return true;
    		} else if (evt.key.keysym.sym == SDLK_RETURN || evt.key.keysym.sym == SDLK_SPACE) {
    			if (selected < pause_items.size() && pause_items[selected].on_select) {
    				Sound::play(*sound_clonk);
    				pause_items[selected].on_select(pause_items[selected]);
            selected = 1;
    				return true;
    			}
    		}
    	}
  		return false;
    } else {
      current->handle_event(evt, window_size);
      // Check UI elements:
      // TODO: find a way to not keep calling screen_get? Same issue in draw_ui.
      if (current->shift.progress == 1.0f) {
        // Player is completed shift mode: check color wheel interaction
        if (evt.type == SDL_MOUSEBUTTONUP && evt.button.button == SDL_BUTTON_LEFT) {
          GameLevel::Standpoint *stpt = current->shift.sc->stpt;
          glm::vec2 mpos = glm::vec2(evt.button.x, evt.button.y);
          glm::vec3 center_clip = glm::vec3(current->shift.sc->stpt->movable_center_to_screen(), 1.0f);
          glm::vec2 center = view_to_drawable * (clip_to_court * center_clip);
          // TODO: Make this a function of wheel_radius
          float threshold = window_size.y / 15.0f;
          glm::vec2 dist = mpos - center;
          if (glm::dot(dist, dist) < threshold * threshold) {
            // TODO: Make this work for multiple colors
            float angle = glm::atan(dist.y, dist.x);
            if (angle < 0.0f) angle += 2.0f * 3.1415926f;
            float sector_angle = 2.0f * 3.1415926f / (float) stpt->move_pos.size();
            size_t index = (size_t) (angle / sector_angle);
            stpt->move_to(index);
            current->currently_moving.emplace_back(stpt->movable->index);
          }
        }
      }
      return false;
    }
	} else { // we're on the main menu
  	if (evt.type == SDL_KEYDOWN) {
      if (evt.key.keysym.sym == SDLK_ESCAPE){
        Mode::set_current(nullptr);
      }
  		if (evt.key.keysym.sym == SDLK_UP || evt.key.keysym.scancode == SDL_SCANCODE_W) {
  			//skip non-selectable items:
  			for (uint32_t i = selected - 1; i < main_items.size(); --i) {
  				if (main_items[i].on_select) {
  					selected = i;
  					Sound::play(*sound_click);
  					break;
  				}
  			}
  			return true;
  		} else if (evt.key.keysym.sym == SDLK_DOWN || evt.key.keysym.scancode == SDL_SCANCODE_S) {
  			//note: skips non-selectable items:
  			for (uint32_t i = selected + 1; i < main_items.size(); ++i) {
  				if (main_items[i].on_select) {
  					selected = i;
  					Sound::play(*sound_click);
  					break;
  				}
  			}
  			return true;
  		} else if (evt.key.keysym.sym == SDLK_RETURN || evt.key.keysym.sym == SDLK_SPACE) {
  			if (selected < main_items.size() && main_items[selected].on_select) {
  				Sound::play(*sound_clonk);
  				main_items[selected].on_select(main_items[selected]);
          selected = 1;
  				return true;
  			}
  		}
  	}
		return false;
	}
}

void MenuMode::update(float elapsed) {

	select_bounce_acc = select_bounce_acc + elapsed / 0.7f;
	select_bounce_acc -= std::floor(select_bounce_acc);

	if (current) {
    current->update(elapsed);
    if (current->we_want_reset && current->they_want_reset) {
      current->level->reset();
      current->pause = false;
      current->won = false;
      current->lost = false;
      current->we_want_reset = false;
      current->they_want_reset = false;
      std::cout << "Reset!" << std::endl;
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    //}
	}
}

void MenuMode::draw_menu(glm::uvec2 const &drawable_size, std::vector<Item> items) {

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
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

		for (auto const &item : items) {
			bool is_selected = (&item == &items[0] + selected);
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
      {-1.0f, -0.98480775301f, -0.93969262078f, -0.86602540378f, -0.76604444311f,
        -0.64278760968f, -0.5f, -0.34202014332f, -0.17364817766f,
        0.0f, 0.17364817766f, 0.34202014332f, 0.5f, 0.64278760968f,
        0.76604444311f, 0.86602540378f, 0.93969262078f, 0.98480775301f,
        1.0f,  0.98480775301f,  0.93969262078f,  0.86602540378f, 0.76604444311f,
        0.64278760968f, 0.5f, 0.34202014332f, 0.17364817766f,
        0.0f, -0.17364817766f, -0.34202014332f, -0.5f, -0.64278760968f,
        -0.76604444311f, -0.86602540378f, -0.93969262078f, -0.98480775301f, -1.0f
      };
    static const float y_offsets[sides+1] =
      {0.0f, -0.17364817766f, -0.34202014332f, -0.5f, -0.64278760968f,
        -0.76604444311f, -0.86602540378f, -0.93969262078f, -0.98480775301f,
        -1.0f, -0.98480775301f, -0.93969262078f, -0.86602540378f, -0.76604444311f,
        -0.64278760968f, -0.5f, -0.34202014332f, -0.17364817766f,
        0.0f, 0.17364817766f, 0.34202014332f, 0.5f, 0.64278760968f,
        0.76604444311f, 0.86602540378f, 0.93969262078f, 0.98480775301f,
        1.0f,  0.98480775301f,  0.93969262078f,  0.86602540378f, 0.76604444311f,
        0.64278760968f, 0.5f, 0.34202014332f, 0.17364817766f, 0.0f
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
  view_to_drawable = glm::vec2(
    drawable_size.x / (view_max.x - view_min.x),
    drawable_size.y / (view_max.y - view_min.y)
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
    else if (current->lost)          { text = "Game Over"; text_scale = 0.9f; }
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
      // draw pause menu
      draw_menu(drawable_size, pause_items);
    } else {
      // draw level
      current->draw(drawable_size);
      draw_ui(drawable_size);
    }
		//it is an error to remove the last reference to this object in current->draw():
		assert(hold_me.use_count() > 1);
	} else {
    draw_menu(drawable_size, main_items);
  }

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
}


void MenuMode::layout_items(float gap) {
	DrawSprites temp(*atlas, view_min, view_max, view_max - view_min, DrawSprites::AlignPixelPerfect); //<-- doesn't actually draw
  // layout for main menu items
  float y = view_max.y;
	for (auto &item : main_items) {
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
	float ofs = -0.5f * y;
	for (auto &item : main_items) {
		item.at.y += ofs;
	}

  // layout for pause menu items
	y = view_max.y;
	for (auto &item : pause_items) {
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
	ofs = -0.5f * y;
	for (auto &item : pause_items) {
		item.at.y += ofs;
	}
}
