#include "MenuMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for easy sprite drawing:
#include "DrawSprites.hpp"

//for playing movement sounds:
#include "Sound.hpp"

//for loading:
#include "Load.hpp"

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
    if (current) { current->level->reset(); }
	};
	pause_items.emplace_back("Main Menu");
	pause_items.back().on_select = [](Item const &){
    MenuMode::set_current(nullptr);
	};
	pause_items.emplace_back("Quit");
	pause_items.back().on_select = [](Item const &){
    Mode::set_current(nullptr);
	};

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
      return current->handle_event(evt, window_size);
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
	}
}

void MenuMode::draw_menu(glm::uvec2 const &drawable_size, std::vector<Item> items) {
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

void MenuMode::draw(glm::uvec2 const &drawable_size) {
	if (current) {
		std::shared_ptr< Mode > hold_me = shared_from_this();
    if (current->pause) {
      // draw pause menu
      draw_menu(drawable_size, pause_items);
    } else {
      // draw level
      current->draw(drawable_size);
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
