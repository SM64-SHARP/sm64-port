#ifndef DEBUG_RECT_H
#define DEBUG_RECT_H

/**
 * @file debug_rect.h
 * Draws debug rects, see debug_rect.c for details
 */

#include "types.h"

/**
 * The max amount of debug rects before debug_rect() just returns.
 * You can set this to something higher, but you might run out of space in the gfx pool.
 */
#define MAX_DEBUG_RECTS 512

struct Vector3
{
    u32 x;
    u32 y;
    u32 z;
};

void debug_rect(Vec3f start, Vec3f end, u32 width, u32 color);

void render_debug_rects(void);

struct Vector3 angle_to_direction(float pitch, float yaw);

#endif /* DEBUG_RECT_H */