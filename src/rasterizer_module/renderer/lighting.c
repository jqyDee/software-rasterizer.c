#include "lighting.h"

#include <stddef.h>

#include "../math/rotation.h"
#include "../world.h"

void compute_light_dirs_cam(world *world) {
  for (size_t light_id = 0; light_id < world->light_count; light_id++) {
    world->light_dirs_cam[light_id] =
        rotate_x(rotate_y(world->lights[light_id].light_dir, -world->cam->yaw),
                 -world->cam->pitch);
  }
}
