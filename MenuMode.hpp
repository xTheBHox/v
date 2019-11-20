#pragma once

/*
 * MenuMode is a game mode that implements a multiple-choice system.
 *
 */

#include "ColorTextureProgram.hpp"
#include "Sprite.hpp"
#include "Mode.hpp"
#include "GameLevel.hpp"
#include "PlayerMode.hpp"
#include "PlayerOneMode.hpp"
#include "PlayerTwoMode.hpp"

#include <vector>
#include <functional>

struct MenuMode : Mode {
	struct Item;
  MenuMode(std::vector< Item > const &main_items);
	virtual ~MenuMode();

	static std::shared_ptr< PlayerMode > current;
  static void set_current(std::shared_ptr< PlayerMode > const &);
  static std::shared_ptr< PlayerMode > get_current();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
  virtual void draw_menu(glm::uvec2 const &drawable_size, std::vector<Item> items);
	virtual void draw_ui(glm::uvec2 const &drawable_size);
	virtual void draw(glm::uvec2 const &drawable_size) override;

  //for drawing manually using vertex buffer
  struct Vertex {
    Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
      Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
    glm::vec3 Position;
    glm::u8vec4 Color;
    glm::vec2 TexCoord;
  };
  ColorTextureProgram color_texture_program; //Shader program that draws transformed, vertices tinted with vertex colors:
  GLuint vertex_buffer = 0; //Buffer used to hold vertex data during drawing
  GLuint vertex_buffer_for_color_texture_program = 0; //VAO that maps buffer locations to color_texture_program attribute locations:
  GLuint white_tex = 0; //Solid white texture:
  glm::mat3x2 clip_to_court = glm::mat3x2(1.0f); //matrix that maps from clip coordinates to court-space coordinates:
  glm::vec2 view_to_drawable = glm::vec2(1.0f, 1.0f);
  // computed in draw() as the inverse of OBJECT_TO_CLIP

	//----- menu state -----

	//Each menu item is an "Item":
	struct Item {
		Item(
			std::string const &name_,
			Sprite const *sprite_ = nullptr,
			float scale_ = 1.0f,
			glm::u8vec4 const &tint_ = glm::u8vec4(0xff),
			std::function< void(Item const &) > const &on_select_ = nullptr,
			glm::vec2 const &at_ = glm::vec2(0.0f)
			) : name(name_), sprite(sprite_), scale(scale_), tint(tint_), selected_tint(tint_), on_select(on_select_), at(at_) {
		}
		std::string name;
		Sprite const *sprite; //sprite drawn for item
		float scale; //scale for sprite
		glm::u8vec4 tint; //tint for sprite (unselected)
		glm::u8vec4 selected_tint; //tint for sprite (selected)
		std::function< void(Item const &) > on_select; //if set, item is selectable
		glm::vec2 at; //location to draw item
	};
	std::vector< Item > main_items;
  std::vector< Item > pause_items;
  std::vector< Item > *current_items;

	//call to arrange items in a centered list:
	void layout_items(float gap = 0.0f);

	//if set, used to highlight the current selection:
	Sprite const *left_select = nullptr;
	Sprite const *right_select = nullptr;

	glm::vec2 left_select_offset = glm::vec2(0.0f);
	glm::vec2 right_select_offset = glm::vec2(0.0f);

	glm::u8vec4 left_select_tint = glm::u8vec4(0xff);
	glm::u8vec4 right_select_tint = glm::u8vec4(0xff);

	float select_bounce_amount = 0.0f;
	float select_bounce_acc = 0.0f;

	//must be set to the atlas from which all the sprites used herein are taken:
	SpriteAtlas const *atlas = nullptr;

	//currently selected item:
	uint32_t selected = 0;

	//area to display; by default, menu lays items out in the [-1,1]^2 box:
	glm::vec2 view_min = glm::vec2(-1.0f, -1.0f);
	glm::vec2 view_max = glm::vec2( 1.0f,  1.0f);

	//if not nullptr, background's functions are called as follows:
	// background->handle_event() is called at the end of handle_event() [if this doesn't handle the event]
	// background->update() is called at the end of update()
	// background->draw() is called at the start of draw()
	//IMPORTANT NOTE: this means that if background->draw() ends up deleting this (e.g., by removing
	//  the last shared_ptr that references it), then it will crash. Don't do that!
	// std::shared_ptr< Mode > background;

};
