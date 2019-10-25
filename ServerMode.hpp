#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include "Connection.hpp"

#include <memory>

//based on PoolMode.hpp in base5
struct ServerMode : Mode {
	ServerMode(std::string const &server_port = "");;
	virtual ~ServerMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//remote connection:
	std::unique_ptr< Server > server;

	//std::unordered_map< Connection const *, PlayerInfo > connection_infos;
};
