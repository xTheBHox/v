#pragma once

#include "PlayerMode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"

struct ConnectionHelper {

	ConnectionHelper(bool isServer_, GameLevel *level_, std::string const &host, std::string const &port);
	bool isServer;
	void send_movable(std::list<size_t> currently_moving);
	void send_player_pos();
	void send_reset(float reset_countdown, bool they_want_reset, bool we_want_reset);
	void receive_data();
	std::unique_ptr< Server > server = nullptr;
	std::unique_ptr< Client > client = nullptr;
	GameLevel *level;


};
