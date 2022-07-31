#include <ultra64.h>

#include "sm64.h"
#include "game/game_init.h"
#include "game/geo_misc.h"
#include "engine/math_util.h"

#include "camera.h"
#include "debug_rect.h"

/**
 * @file debug_rect.c
 * Draws 3D rects for debugging purposes.
 *
 * How to use:
 *
 * In render_game() in area.c, add a call to render_debug_rects():
 *
 * void render_game(void) {
 *    if (gCurrentArea != NULL && !gWarpTransition.pauseRendering) {
 *        geo_process_root(...);
 *
 *        render_debug_rects(); // add here
 *
 *        gSPViewport(...);
 *        gDPSetScissor(...);
 *    //...
 *
 * Now just call debug_rect() whenever you want to draw one!
 *
 * debug_rect by default takes two arguments: a start and end vec3f.
 * This will draw a rect starting from start to end.
 */

/**
 * Internal struct containing rect info
 */
struct DebugRect {
    u32 color;
    Vec3s start;
    Vec3s end;
    s16 yaw;
    u32 width;
};

struct DebugRect sRects[MAX_DEBUG_RECTS];
s16 sNumRects = 0;

extern Mat4 gMatStack[32]; //XXX: Hack

/**
 * The allocated size of a rect's dl
 */
#define DBG_RECT_DLSIZE     ((s32)(2 * sizeof(Gfx) + 8 * sizeof(Vtx)))

/**
 * Sets up the RCP for drawing the rects
 */
static const Gfx dl_debug_rect_begin[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_ZB_OPA_SURF, G_RM_NOOP2),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

static const Gfx dl_debug_rect_end[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
    gsSPSetGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPTexture(0, 0, 0, 0, G_ON),
    gsSPEndDisplayList(),
};

/**
 * Actually draws the rect
 */
static const Gfx dl_debug_draw_rect[] = {
    gsSP2Triangles(5,  4,  6, 0x0,  5,  6,  7, 0x0), // front
    gsSP2Triangles(0,  1,  2, 0x0,  2,  1,  3, 0x0), // back

    gsSP2Triangles(4,  0,  2, 0x0,  2,  6,  4, 0x0), // left
    gsSP2Triangles(1,  5,  3, 0x0,  3,  5,  7, 0x0), // right

    gsSP2Triangles(1,  0,  4, 0x0,  1,  4,  5, 0x0), // top
    gsSP2Triangles(2,  3,  6, 0x0,  6,  3,  7, 0x0), // bottom

    gsSPEndDisplayList(),
};

/**
 * Adds a rect to the list to be rendered this frame.
 *
 * If there are already MAX_DEBUG_RECTS rects, does nothing.
 */
static void append_debug_rect(Vec3f start, Vec3f end, u32 width, u32 color)
{
    if (sNumRects >= MAX_DEBUG_RECTS) {
        return;
    }

    vec3f_to_vec3s(sRects[sNumRects].start, start);
    vec3f_to_vec3s(sRects[sNumRects].end, end);

    //sRects[sNumRects].color = sCurRectColor;
    sRects[sNumRects].color = color;
    sRects[sNumRects].width = width;

    ++sNumRects;
}

/**
 * Draws a debug rect from (center - bounds) to (center + bounds)
 * To draw a rotated rect, use debug_rect_rot()
 *
 * @see debug_rect_rot()
 */
void debug_rect(Vec3f start, Vec3f end, u32 width, u32 color)
{
    append_debug_rect(start, end, width, color);
}

struct Vector3 angle_to_direction(float pitch, float yaw)
{
    struct Vector3 result;
    result.x = (float)(cos(pitch) * sin(yaw));
    result.y = (float)sin(pitch);
    result.z = (float)(cos(pitch) * cos(yaw));
    return result;
}

static void render_rect(int index)
{
    Vtx *verts;
    /*Mtx *translate;
    Mtx *rotate;
    Mtx *translateback;*/
    struct DebugRect *rect = &sRects[index];
    u32 color = rect->color;

    s32 xa = rect->start[0],
        ya = rect->start[1],
        za = rect->start[2];

    s32 xb = rect->end[0],
        yb = rect->end[1],
        zb = rect->end[2];

    if ((Gfx*)gGfxPoolEnd - gDisplayListHead < DBG_RECT_DLSIZE)
        return;

    verts = alloc_display_list(8 * sizeof(Vtx));

    /*Vec3f up;
    vec3f_set(up, 0, 1, 0);

    Vec3f right;
    vec3f_set(right, 0, 0, 0);

    Vec3f camDir;
    vec3f_set(camDir, gLakituState.pos[0] - gLakituState.focus[0], gLakituState.pos[1] - gLakituState.focus[1], gLakituState.pos[2] - gLakituState.focus[2] );
    vec3f_normalize(camDir);
    vec3f_cross(right, camDir, up);
    vec3f_cross(up, camDir, right);*/

    Vec3f up;
    vec3f_set(up, 0, 1, 0);

    Vec3f right;
    vec3f_set(right, 0, 0, 0);

    Vec3f dir;
    vec3f_set(dir, rect->start[0] - rect->end[0], rect->start[1] - rect->end[1], rect->start[2] - rect->end[2] );
    vec3f_normalize(dir);

    vec3f_cross(right, up, dir);
    vec3f_normalize(right);

    vec3f_cross(up, right, dir);
    vec3f_normalize(up);

    if (verts != NULL) {
        /*translate =     alloc_display_list(sizeof(Mtx));
        rotate =        alloc_display_list(sizeof(Mtx));
        translateback = alloc_display_list(sizeof(Mtx));

        guTranslate(translate, xb - xa,  yb - ya,  zb - za);
        guRotate(rotate, rect->yaw / (float)0x10000 * 360.0f, 0, 1.0f, 0);
        guTranslate(translateback, -(xb - xa), -(yb - ya), -(zb - za));

        gSPMatrix(gDisplayListHead++, translate,     G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPMatrix(gDisplayListHead++, rotate,        G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gSPMatrix(gDisplayListHead++, translateback, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);*/

#define DBG_RECT_COL /**/ (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff, (color >> 24) & 0xff
#define DBG_RECT_VTX(i, x, y, z) make_vertex(verts, i, x, y, z, 0, 0, DBG_RECT_COL)
        /*DBG_RECT_VTX(0, xa + up[0] * 25, ya + up[1] * 25, za + up[2] * 25);
        DBG_RECT_VTX(1, xb + up[0] * 25, yb + up[1] * 25, zb + up[2] * 25);
        DBG_RECT_VTX(2, xa - up[0] * 25, ya - up[1] * 25, za - up[2] * 25);
        DBG_RECT_VTX(3, xb - up[0] * 25, yb - up[1] * 25, zb - up[2] * 25);*/

        //Back
        DBG_RECT_VTX(0, xa + (up[0] * rect->width) - (right[0] * rect->width), ya + (up[1] * rect->width) - (right[1] * rect->width), za + (up[2] * rect->width) - (right[2] * rect->width)); // Up_Left
        DBG_RECT_VTX(1, xa + (up[0] * rect->width) + (right[0] * rect->width), ya + (up[1] * rect->width) + (right[1] * rect->width), za + (up[2] * rect->width) + (right[2] * rect->width)); // Up_Right
        DBG_RECT_VTX(2, xa - (up[0] * rect->width) - (right[0] * rect->width), ya - (up[1] * rect->width) - (right[1] * rect->width), za - (up[2] * rect->width) - (right[2] * rect->width)); // Down_Left
        DBG_RECT_VTX(3, xa - (up[0] * rect->width) + (right[0] * rect->width), ya - (up[1] * rect->width) + (right[1] * rect->width), za - (up[2] * rect->width) + (right[2] * rect->width)); // Down_Right

        //Front
        DBG_RECT_VTX(4, xb + (up[0] * rect->width) - (right[0] * rect->width), yb + (up[1] * rect->width) - (right[1] * rect->width), zb + (up[2] * rect->width) - (right[2] * rect->width)); // Up_Left
        DBG_RECT_VTX(5, xb + (up[0] * rect->width) + (right[0] * rect->width), yb + (up[1] * rect->width) + (right[1] * rect->width), zb + (up[2] * rect->width) + (right[2] * rect->width)); // Up_Right
        DBG_RECT_VTX(6, xb - (up[0] * rect->width) - (right[0] * rect->width), yb - (up[1] * rect->width) - (right[1] * rect->width), zb - (up[2] * rect->width) - (right[2] * rect->width)); // Down_Left
        DBG_RECT_VTX(7, xb - (up[0] * rect->width) + (right[0] * rect->width), yb - (up[1] * rect->width) + (right[1] * rect->width), zb - (up[2] * rect->width) + (right[2] * rect->width)); // Down_Right

#undef DBG_RECT_VTX

        gSPVertex(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(verts), 8, 0);

        gSPDisplayList(gDisplayListHead++, dl_debug_draw_rect);
    }
}

void render_debug_rects(void)
{
    s32 i;
    Mtx *mtx;

    if (sNumRects == 0) {
        return;
    }

    mtx = alloc_display_list(sizeof(Mtx));
    // Don't draw if there isn't space for the configuration + at least one rect
    if (mtx == NULL || ((Gfx*)gGfxPoolEnd - gDisplayListHead) <= (s32)(2 * sizeof(Gfx)) + DBG_RECT_DLSIZE) {
        sNumRects = 0;
        return;
    }

    //XXX: This is hacky. Ths camera's look-at matrix is stored in gMatStack[1], so this is a simple way
    //     of using it without reconstructing the matrix.
    mtxf_to_mtx(mtx, gMatStack[1]);
    gSPMatrix(gDisplayListHead++, mtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPDisplayList(gDisplayListHead++, dl_debug_rect_begin);

    for (i = 0; i < sNumRects; ++i) {
        render_rect(i);
    }

    gSPDisplayList(gDisplayListHead++, dl_debug_rect_end);
    sNumRects = 0;

    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}