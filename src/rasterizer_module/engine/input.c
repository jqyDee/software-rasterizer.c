#include "input.h"
#include "../testing.h"

#include <stdio.h>

#include "../math/collision.h"
#include "../math/vec.h"

// ========== DECL ==========
INTERNAL_INLINE void ctrl_click_pick(world *world);
INTERNAL_INLINE user_input_response mouse_look(world *world);
INTERNAL_INLINE void cam_movement(world *world, const float delta_time);
INTERNAL_INLINE void undo_redo(world *world);
INTERNAL_INLINE user_input_response function_keys(world *world);

// ========== EXPORT ==========
user_input_response handle_user_input(world *world, const float delta_time) {
  ctrl_click_pick(world);

  user_input_response r = mouse_look(world);
  if (r != UIR_NONE)
    return r;

  if (world->settings.text_input_active)
    return UIR_NONE;

  cam_movement(world, delta_time);
  undo_redo(world);
  return function_keys(world);
}

// ========== HELPERS ==========
INTERNAL_INLINE vec3f get_forward(const cam *cam) {
  return (vec3f){sinf(cam->yaw), 0, cosf(cam->yaw)};
}

INTERNAL_INLINE vec3f get_right(const cam *cam) {
  return (vec3f){sinf(cam->yaw - M_PI_2), 0, cosf(cam->yaw - M_PI_2)};
}

INTERNAL_INLINE void apply_wasd(const world *world, const vec3f forward,
                                const vec3f right, const float factor,
                                const float delta_time, vec3f *new_pos) {
  if (IsKeyDown(KEY_W)) {
    *new_pos = vec_add(*new_pos, vec_scale(forward, world->settings.move_speed *
                                                        factor * delta_time));
  }
  if (IsKeyDown(KEY_S)) {
    *new_pos = vec_sub(*new_pos, vec_scale(forward, world->settings.move_speed *
                                                        factor * delta_time));
  }
  if (IsKeyDown(KEY_A)) {
    *new_pos = vec_add(*new_pos, vec_scale(right, world->settings.move_speed *
                                                      factor * delta_time));
  }
  if (IsKeyDown(KEY_D)) {
    *new_pos = vec_sub(*new_pos, vec_scale(right, world->settings.move_speed *
                                                      factor * delta_time));
  }
}

INTERNAL_INLINE void apply_vertical(const world *world, const float factor,
                                    const float delta_time, vec3f *new_pos) {
  if (IsKeyDown(KEY_SPACE)) {
    new_pos->y += world->settings.move_speed * factor * delta_time;
  }
  if (IsKeyDown(KEY_LEFT_SHIFT)) {
    new_pos->y -= world->settings.move_speed * factor * delta_time;
  }
}

INTERNAL_INLINE void apply_jump(world *world, const float delta_time,
                                vec3f *new_pos) {
  cam *cam = world->cam;

  // second check is for if jump is already in process
  bool on_flat_ground = cam->pos.y <= world->settings.ground_y + Y_SLOP &&
                        world->player_vy <= 0.0f;
  bool on_mesh =
      world->settings.collision_enabled && world->player_vy <= 0.0f &&
      camera_collides(world,
                      (vec3f){cam->pos.x, cam->pos.y - Y_SLOP_X2, cam->pos.z});

  // only jump when ON ground or mesh.
  if (IsKeyPressed(KEY_SPACE) && (on_flat_ground || on_mesh)) {
    world->player_vy = world->settings.jump_speed;
  }

  // update
  world->player_vy -= world->settings.gravity * delta_time;
  new_pos->y = cam->pos.y + world->player_vy * delta_time;

  // clamp to ground
  if (new_pos->y < world->settings.ground_y) {
    new_pos->y = world->settings.ground_y;
    world->player_vy = 0.0f;
  }
}

// FREE FLY CAM; NO COLLISSION;
INTERNAL_INLINE void move_cam_free_fly(world *world, const vec3f forward,
                                       const vec3f right,
                                       const float delta_time) {
  cam *cam = world->cam;
  vec3f new_pos = cam->pos;

  apply_wasd(world, forward, right, world->settings.debug_cam_speed_factor,
             delta_time, &new_pos);
  apply_vertical(world, world->settings.debug_cam_speed_factor, delta_time,
                 &new_pos);

  cam->pos = new_pos;
}

// GAME CAM
INTERNAL_INLINE void move_cam_game(world *world, const vec3f forward,
                                   const vec3f right, const float delta_time) {
  cam *cam = world->cam;
  vec3f old_pos = cam->pos;
  vec3f new_pos = old_pos;

  apply_wasd(world, forward, right, 1.0f, // fixed factor of 1, no speed boost
             delta_time, &new_pos);
  apply_jump(world, delta_time, &new_pos);

  // if collision enabled, resolve the position before applying
  if (world->settings.collision_enabled) {
    /* resolve Y first so x/z tests use the correct height */
    float resolved_y = old_pos.y;
    vec3f try_y = {old_pos.x, new_pos.y, old_pos.z};
    if (!camera_collides(world, try_y))
      resolved_y = new_pos.y;
    else
      world->player_vy = 0.0f;

    vec3f resolved = {old_pos.x, resolved_y, old_pos.z};
    vec3f try_x = {new_pos.x, resolved_y, old_pos.z};
    if (!camera_collides(world, try_x))
      resolved.x = new_pos.x;
    vec3f try_z = {resolved.x, resolved_y, new_pos.z};
    if (!camera_collides(world, try_z))
      resolved.z = new_pos.z;

    cam->pos = resolved;
  } else {
    cam->pos = new_pos;
  }
}

INTERNAL_INLINE void cam_movement(world *world, const float delta_time) {
  cam *cam = world->cam;
  vec3f forward = get_forward(cam);
  vec3f right = get_right(cam);

  if (world->settings.show_debug_gui) {
    move_cam_free_fly(world, forward, right, delta_time);
  } else {
    move_cam_game(world, forward, right, delta_time);
  }
}

INTERNAL_INLINE user_input_response function_keys(world *world) {
  if (IsKeyPressed(KEY_F2)) {
    world->settings.show_fps = !world->settings.show_fps;
  }
  if (IsKeyPressed(KEY_F3)) {
    world->settings.show_debug_gui = !world->settings.show_debug_gui;
    world->cam =
        world->settings.show_debug_gui ? &world->debug_cam : &world->game_cam;
  }
  if (IsKeyPressed(KEY_F5)) {
    return UIR_RELOAD_PLUGIN;
  }
  if (IsKeyPressed(KEY_F6)) {
    return UIR_RESET_CAM_AND_POSITION;
  }
  return UIR_NONE;
}

INTERNAL_INLINE void undo_redo(world *world) {
  if (!world->settings.dragging_gui &&
      (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER))) {
    if (IsKeyPressed(KEY_Y)) { // this is swapped, as I am on german keyboard
      if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        scene_redo(world);
        printf("INFO: redoing.\n");
      } else {
        scene_undo(world);
        printf("INFO: undoing.\n");
      }
    }
    if (IsKeyPressed(KEY_Z)) { // this is swapped, as I am on german keyboard
      scene_redo(world);
      printf("INFO: redoing.\n");
    }
  }
}

INTERNAL_INLINE void ctrl_click_pick(world *world) {
  settings *settings = &world->settings;
  if (!settings->show_debug_gui || settings->mouse_over_gui ||
      !IsKeyDown(KEY_LEFT_CONTROL) || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return;

  Vector2 mouse = GetMousePosition();
  float sx = (float)world->renderer->screen_width /
             (float)world->renderer->display_width;
  float sy = (float)world->renderer->screen_height /
             (float)world->renderer->display_height;
  int fb_x = (int)(mouse.x * sx);
  int fb_y = (int)(mouse.y * sy);

  if (fb_x >= 0 && fb_x < world->renderer->screen_width && fb_y >= 0 &&
      fb_y < world->renderer->screen_height) {
    int id =
        world->renderer->idbuffer[fb_y * world->renderer->screen_width + fb_x];
    if (id >= 0)
      world->selected_instance = id;
  }
}

// Returns UIR_NONE on the first frame of capture — cursor just disabled,
// delta is invalid so skip movement that frame.
INTERNAL_INLINE user_input_response mouse_look(world *world) {
  cam *cam = world->cam;
  settings *settings = &world->settings;

  // Only call Disable/EnableCursor on transition — both internally call
  // SetMousePosition which resets the delta accumulator if called every frame.
  static bool cursor_disabled = false;
  bool capture =
      !settings->show_debug_gui ||
      (!IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonDown(MOUSE_LEFT_BUTTON) &&
       !settings->mouse_over_gui);

  if (capture) {
    if (!cursor_disabled) {
      DisableCursor();
      cursor_disabled = true;
      return UIR_NONE;
    }
    Vector2 delta = GetMouseDelta();
    cam->yaw += delta.x * settings->mouse_sensitivity;
    cam->pitch += delta.y * settings->mouse_sensitivity;
    const float max_pitch = M_PI_2 - 0.01f;
    if (cam->pitch > max_pitch)
      cam->pitch = max_pitch;
    if (cam->pitch < -max_pitch)
      cam->pitch = -max_pitch;
  } else {
    if (cursor_disabled) {
      EnableCursor();
      cursor_disabled = false;
    }
  }
  return UIR_NONE;
}
