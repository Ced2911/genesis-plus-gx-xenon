#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>

#include "osd.h"

typedef unsigned int DWORD;
#include "ps.h"
#include "vs.h"

#define XE_W 1024
#define XE_H 1024

static struct XenosVertexBuffer *vb = NULL;
static struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;

static struct XenosDevice _xe;

unsigned char* screen = NULL;
struct XenosSurface * g_pTexture = NULL;

static uint32_t pitch = 0;

typedef struct DrawVerticeFormats {
    float x, y, z, w;
    unsigned int color;
    float u, v;
} DrawVerticeFormats;

enum {
    UvBottom = 0,
    UvTop,
    UvLeft,
    UvRight
};
float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

void update_texture_viewport(int width, int height) {
    // Create display
    static int old_width = 0;
    static int old_height = 0;

    //if ((width != old_width) || (old_height != height)) {

    ScreenUv[UvTop] = ((float) (width) / (float) XE_W)*2;
    ScreenUv[UvLeft] = ((float) (height) / (float) XE_H)*2;

    DrawVerticeFormats *Rect = Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // bottom left

        Rect[1].v = ScreenUv[UvLeft];
        Rect[2].u = ScreenUv[UvTop];

    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    old_width = width;
    old_height = height;
    //}
    //printf("%f - %f \r\n",Rect[1].v,Rect[2].u);
}

void SYSVideoInit() {
    g_pVideoDevice = &_xe;
    Xe_Init(g_pVideoDevice);

    Xe_SetRenderTarget(g_pVideoDevice, Xe_GetFramebufferSurface(g_pVideoDevice));

    static const struct XenosVBFFormat vbf = {
        3,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
        }
    };

    g_pPixelTexturedShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_PS);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelTexturedShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xvs_VS);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);
    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);

    edram_init(g_pVideoDevice);

    g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);
    //g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_5551 | XE_FMT_ARGB, 0);
    screen = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    pitch = g_pTexture->wpitch;
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    // disable filtering
    if (config.video_filter == VIDEO_FILTER_BLINEAR)
        g_pTexture->use_filtering = 1;
    else
        g_pTexture->use_filtering = 0;

    // move it to ini file
    float x = -1.0f;
    float y = 1.0f;
    float w = 4.0f;
    float h = 4.0f;
    /*
        float w = 2.0f;
        float h = 2.0f;
     */

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 3 * sizeof (DrawVerticeFormats));
    DrawVerticeFormats *Rect = Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        ScreenUv[UvTop] = ScreenUv[UvTop]*2;
        ScreenUv[UvLeft] = ScreenUv[UvLeft]*2;

        // top left
        Rect[0].x = x;
        Rect[0].y = y;
        Rect[0].u = ScreenUv[UvBottom];
        Rect[0].v = ScreenUv[UvRight];
        Rect[0].color = 0;

        // bottom left
        Rect[1].x = x;
        Rect[1].y = y - h;
        Rect[1].u = ScreenUv[UvBottom];
        Rect[1].v = ScreenUv[UvLeft];
        Rect[1].color = 0;

        // top right
        Rect[2].x = x + w;
        Rect[2].y = y;
        Rect[2].u = ScreenUv[UvTop];
        Rect[2].v = ScreenUv[UvRight];
        Rect[2].color = 0;

        // top right
        Rect[3].x = x + w;
        Rect[3].y = y;
        Rect[3].u = ScreenUv[UvTop];
        Rect[3].v = ScreenUv[UvRight];
        Rect[3].color = 0;

        int i = 0;
        for (i = 0; i < 3; i++) {
            Rect[i].z = 0.0;
            Rect[i].w = 1.0;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetClearColor(g_pVideoDevice, 0);
}

void SYSVideoUpdate() {
    /* resize uv to viewport */
    update_texture_viewport(bitmap.viewport.w, bitmap.viewport.h);

    // Refresh texture cash
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    // Reset states
    Xe_InvalidateState(g_pVideoDevice);
    Xe_SetClearColor(g_pVideoDevice, 0);

    // Select stream and shaders
    Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, 0, sizeof (DrawVerticeFormats));
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    // Draw
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_TRIANGLELIST, 0, 1);

    // Resolve
    Xe_Resolve(g_pVideoDevice);
    //while (!Xe_IsVBlank(g_pVideoDevice));
    Xe_Sync(g_pVideoDevice);
}