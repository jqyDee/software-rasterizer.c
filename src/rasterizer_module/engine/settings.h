#pragma once

typedef enum {
  LIGHTING_CPU_LAMBERTIAN,
  LIGHTING_GPU_PHONG,
} lighting_mode_t;

typedef struct settings_s {
  bool show_debug_gui;
  bool track_editor_active;
  bool mouse_over_gui;
  bool show_normals;
  bool text_input_active;
  bool dragging_gui;
  bool collision_enabled;
  bool show_collision_boxes;
  bool show_kart_debug;
  bool show_fps;
  bool debug_skip_texture;
  bool debug_skip_normalbuffer;
  bool debug_skip_albedobuffer;
  bool debug_skip_framebuffer;
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
  float ambient_light;
  lighting_mode_t lighting_mode;
} settings;
