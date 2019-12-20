#pragma once

#include "Mode.hpp"
#include "GameLevel.hpp"
#include "Connection.hpp"
#include "Sound.hpp"

#include <functional>

struct PlayerMode : Mode {
  PlayerMode(uint32_t level_num_, uint32_t player_num_);
  virtual ~PlayerMode();

  virtual void level_change(uint32_t level_num_);
  virtual void level_reset();
  virtual void player_set();

  virtual void handle_reset();
  bool handle_ui(SDL_Event const &evt, glm::uvec2 const &window_size);

  virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

  void update_shift(float elapsed);
  void update_reset_timer(float elapsed);
  void update_movables_move(float elapsed);
  void update_me_move(float elapsed);

  virtual int update_recv_msg(char msg_type, char *buf, size_t buf_len, size_t *used_len);
  virtual void update_recv(std::vector< char >& data);
  virtual void update_send();
  virtual void update_network();

  virtual void update(float elapsed) override;

  virtual void draw(glm::uvec2 const &drawable_size) override;

  //Current control signals:
  struct {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;
    bool jump = false;
    bool mouse_down = false;
    float mouse_sensitivity = 4.0f;
  } controls;

  bool DEBUG_fly = false;  //fly around for collsion debug
  bool pause = false;
  bool won = false;
  float to_next_level = 0.0f;
  bool lost = false;
  bool we_want_reset = false;
  bool they_want_reset = false;
  bool we_reached_goal = false;
  float reset_countdown = 0.0f;
  uint32_t player_num;
  uint32_t level_num;

  // Player camera tracked using this structure:
  struct PlayerData {
    Scene::Camera *camera = nullptr;
    Scene::Transform *body = nullptr;
    glm::vec3 vel = glm::vec3(0.0f, 0.0f, 0.0f);
    float azimuth = 0.0f;
    float elevation = 0.0f / 180.0f * 3.1415926f;
    bool in_air = false;
    GameLevel::Movable *on_movable = nullptr;
  };

  PlayerData pov;

  struct {
    bool flat = false;
  } controls_shift;

  struct {
    GameLevel::Screen *sc = nullptr;
    // How much the camera has shifted
    float progress = 0.0f;
    float speed = 4.0f; // 1/t, t is the time taken to shift
  } shift;

  std::list< size_t > currently_moving;
  uint32_t play_moving_sound = 0;

  Scene::Transform *other_player;

  Connection *connect = nullptr;
  std::function<void(Connection *, Connection::Event)> callback_fn;

  GameLevel *level = nullptr;

  struct Physics {
    
    // max speed for ground movement / walking
    static constexpr float player_move_max_speed = 15.0f;
    // interpolation half-life for ground movement
    static constexpr float player_move_halflife = 0.02f;

    // sprinting ground movement multiplier
    static constexpr float player_sprint_multiplier = 2.0f;

    // max speed for air movement
    static constexpr float player_air_move_max_speed = 5.0f;
    // interpolation half-life for air movement
    static constexpr float player_air_move_halflife = 0.1f;

    // Cosine of the max angle of the surface with respect to the horizontal
    // plane that will reset jumping for the player.
    static constexpr float player_jumpable_reset_angle_cos = 0.1f;

    // gravitational acceleration vector
    static constexpr glm::vec3 gravity = glm::vec3(0.0f, 0.0f, -100.0f);

    // jumping force, in seconds (time from jumping start to peak height)
    static constexpr float player_jump_sec = 0.3f;
  };

};
