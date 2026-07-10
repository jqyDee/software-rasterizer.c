#pragma once

typedef struct settings_s {
  bool show_debug_gui;
  bool mouse_over_gui;
  bool show_normals;
  bool text_input_active;
  bool dragging_gui;
  bool collision_enabled;
  bool show_collision_boxes;
  bool show_fps;
  int render_width;
  int parallel_cutoff_rows;
  float near_plane;
  float mouse_sensitivity;
  float move_speed;
  float debug_cam_speed_factor;
  float gravity;
  float jump_speed;
  float ground_y;
  float camera_radius;
  float seam_bias;
} settings;
