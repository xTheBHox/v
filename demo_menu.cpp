#include "demo_menu.hpp"
#include "Load.hpp"
#include "Sprite.hpp"
#include "data_path.hpp"

#include "GameLevel.hpp"
#include "PlayerTwoMode.hpp"
#include "PlayerOneMode.hpp"

#include <string>

Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const * {
	return new SpriteAtlas(data_path("trade-font"));
});

std::shared_ptr< MenuMode > demo_menu;
std::string connect_ip;

Load< void > load_demo_menu(LoadTagDefault, [](){

	std::vector< MenuMode::Item > main_items;
	main_items.emplace_back("[[ V ]]");
	main_items.emplace_back("Client");
	main_items.back().on_select = [](MenuMode::Item const &){
		MenuMode::set_current(std::make_shared< PlayerTwoMode >(connect_ip, "12345"));
	};
	main_items.emplace_back("Server");
	main_items.back().on_select = [](MenuMode::Item const &){
		MenuMode::set_current(std::make_shared< PlayerOneMode >("12345"));
	};

	demo_menu = std::make_shared< MenuMode >(main_items);
	demo_menu->atlas = trade_font_atlas;
	demo_menu->selected = 1;
	demo_menu->view_min = glm::vec2(0.0f, 0.0f);
	demo_menu->view_max = glm::vec2(320.0f, 200.0f);

	// demo_menu->layout_main_items(2.0f);
	// demo_menu->layout_pause_items(2.0f);
  demo_menu->layout_items(2.0f);

	demo_menu->left_select = &trade_font_atlas->lookup(">");
	demo_menu->left_select_offset = glm::vec2(-5.0f - 3.0f, 0.0f);
	demo_menu->right_select = &trade_font_atlas->lookup("<");
	demo_menu->right_select_offset = glm::vec2(0.0f + 3.0f, 0.0f);
	demo_menu->select_bounce_amount = 5.0f;
});
