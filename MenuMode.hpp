#pragma once

/*
 * MenuMode is a game mode that implements a multiple-choice system.
 *
 */

#include "ColorTextureProgram.hpp"
#include "Sprite.hpp"
#include "Mode.hpp"
#include "GameLevel.hpp"
#include "SinglePlayerMode.hpp"
#include "ServerMode.hpp"
#include "ClientMode.hpp"
#include "Sound.hpp"

#include <vector>
#include <functional>


struct MenuMode : Mode {
	struct Stage;
	struct Item;
  MenuMode();
	virtual ~MenuMode();

	static std::unique_ptr< PlayerMode > current;

	void start_game(uint32_t level_num);

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
  virtual void draw_menu(glm::uvec2 const &drawable_size, Stage const &stage);
	virtual void draw_ui(glm::uvec2 const &drawable_size);
	virtual void draw(glm::uvec2 const &drawable_size) override;
	
	inline Stage& curr_stage() {
		return stages[stage_name];
	}
	inline std::vector< Item >& curr_items() {
		return stages[stage_name].items;
	}
	inline Item& curr_item() {
		return stages[stage_name].items[stages[stage_name].selected];
	};
	inline uint32_t& curr_select() {
		return stages[stage_name].selected;
	};

	Item *active_item = nullptr;

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

	enum MenuStage : uint32_t {
		MENU_MAIN,
		MENU_CONNECT,
		MENU_IP,
		MENU_START,
		MENU_LEVEL,
		MENU_PLAYER,
		MENU_PAUSE,
		MENU_HELP,
		MENU_COUNT // the number of menu items. Not a menu!
	};

	enum MenuPlayer { MENU_PLAYER_SOLO, MENU_PLAYER_SERVER, MENU_PLAYER_CLIENT };

	//Each menu item is an "Item":
	struct Item {

		Item(
			std::string const &name_,
			Sprite const *sprite_ = nullptr,
			std::function< bool(Item &, SDL_Event const &) > event_handler_ = nullptr,
			std::function< void(Item &) > on_select_ = nullptr
		) : name(name_), sprite(sprite_), event_handler(event_handler_), on_select(on_select_) { }

		std::string name;
		Sprite const *sprite; //sprite drawn for item
		
		std::function< bool(Item &, SDL_Event const &) > event_handler; // when focused, handle events
		std::function< void(Item &) > on_select; //if set, item is selectable

		float scale = 1.0f; //scale for sprite

		glm::u8vec4 tint = glm::u8vec4(0xff); //tint for sprite (unselected)
		glm::u8vec4 selected_tint = glm::u8vec4(0xff); //tint for sprite (selected)

		glm::vec2 at = glm::vec2(0.0f); //location to draw item

		static Item TextItem(std::string const &name_);
		static Item TextButtonItem(std::string const &name_, std::function< void(Item const &) > on_select_);
		static Item TextEntryItem(
			std::string const &name_,
			std::function< bool(SDL_Keycode) > validate_input
		);

		static Item NavigationButtonItem(
			std::string const &name_,
			MenuMode *menu, 
			MenuStage stage_to,
			std::function< void(Item &) > on_select = nullptr
		); // add more params
		static Item BackButtonItem(MenuMode *menu);
		static Item ExitButtonItem();

	};

	struct Stage {

		// All the items in the menu
		std::vector< Item > items;

		// The menu stage we came from
		MenuStage stage_from = MENU_COUNT;
		
		// currently selected menu item
		uint32_t selected = 0;

		// the item to invoke when escape is pressed
		// defaults to last selectable item
		uint32_t escape = 0;

		void set_items();
		void set_items(uint32_t esc_item);

	};

	std::vector< Stage > stages = std::vector<Stage> (MENU_COUNT);

	MenuStage stage_name = MENU_MAIN;
	MenuPlayer menu_player = MENU_PLAYER_SOLO;
	uint32_t selected_player = 1;
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
  std::shared_ptr< Sound::PlayingSample > background_music;
  std::shared_ptr< Sound::PlayingSample > moving_sound;
	std::shared_ptr< Sound::PlayingSample > win_sound;

	bool we_just_reached_goal = false;


};
