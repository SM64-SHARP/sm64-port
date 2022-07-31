#ifndef DEBUG_BOX_H
#define DEBUG_BOX_H

/**
 * @file debug_box.h
 * Draws debug boxes, see debug_box.c for details
 */

#include "types.h"

/**
 * The max amount of debug boxes before debug_box() just returns.
 * You can set this to something higher, but you might run out of space in the gfx pool.
 */
#define MAX_DEBUG_BOXES 512

void debug_box_color(u32 color);
void debug_box(Vec3f center, Vec3f bounds);
void debug_box_rot(Vec3f center, Vec3f bounds, s16 yaw);

void debug_box_pos(Vec3f pMin, Vec3f pMax);
void debug_box_pos_rot(Vec3f pMin, Vec3f pMax, s16 yaw);

void render_debug_boxes(void);

#endif /* DEBUG_BOX_H */