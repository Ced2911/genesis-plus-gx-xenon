#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef unsigned int DWORD;
#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include "tex2Sai.h"
#include "filter/vfilter.h"

#include "osd.h"
#include "ps.h"
#include "vs.h"

#define XE_W 2048
#define XE_H 2048

static struct XenosVertexBuffer *vb = NULL;
static struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;

unsigned char* screen = NULL;
struct XenosSurface * g_pTexture = NULL;

uint32_t pitch = 0;

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

//wiigui-xenon video.cpp
struct XenosDevice * GetVideoDevice();

float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

static unsigned char * hq_buffer = NULL;
static unsigned char * texture_buffer = NULL;
static int texture_pitch = NULL;

void update_texture_viewport() {
    struct XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
    
    // center
    float x, y;
    x = 0;
    y = 0;
    
    float w =0;
    float h =0;

    // fullscreen
    if(useFullScreen()){
        w = 1;
        h = 1;
    }else{
        // 4/3
        w = 3.f/4.f;
        h = 1;
    }
    
    DrawVerticeFormats *Rect = Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // bottom left
        Rect[0].x = x - w;
        Rect[0].y = y - h;
        Rect[0].u = 0;
        Rect[0].v = 1;

        // bottom right
        Rect[1].x = x + w;
        Rect[1].y = y - h;
        Rect[1].u = 1;
        Rect[1].v = 1;

        // top right
        Rect[2].x = x + w;
        Rect[2].y = y + h;
        Rect[2].u = 1;
        Rect[2].v = 0;
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);
}

static int video_initialised = 0;

void SYSVideoInit() {


    g_pVideoDevice = GetVideoDevice();

    Xe_SetRenderTarget(g_pVideoDevice, Xe_GetFramebufferSurface(g_pVideoDevice));

    if (video_initialised == 0) {
        // init 2xsai buffer
        hq_buffer = (unsigned char*) malloc(1024 * 1024 * 4);


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

        if (g_pTexture == NULL)
            g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);

        texture_buffer = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
        pitch = g_pTexture->wpitch;
        Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    }

    memset(hq_buffer, 0, 1024 * 1024 * 4);

    // disable filtering
/*
    if (config.video_filter)
        g_pTexture->use_filtering = 1;
    else
        g_pTexture->use_filtering = 0;
*/

    
    /*
        screen = texture_buffer;
     * pitch = g_pTexture->wpitch;
     */
    screen = hq_buffer;

    /*
        float w = 2.0f;
        float h = 2.0f;
     */
    if (video_initialised == 0) {

        // move it to ini file
        float x = -1.0f;
        float y = 1.0f;
        float w = 2.0f;
        float h = 2.0f;

        vb = Xe_CreateVertexBuffer(g_pVideoDevice, 3 * sizeof (DrawVerticeFormats));
        DrawVerticeFormats *Rect = Xe_VB_Lock(g_pVideoDevice, vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
        {
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

            int i = 0;
            for (i = 0; i < 3; i++) {
                Rect[i].z = 0.0;
                Rect[i].w = 1.0;
            }
        }
        Xe_VB_Unlock(g_pVideoDevice, vb);

    }
    if (video_initialised == 0)
        video_initialised = 1;
}

static void XeTexSubImage(struct XenosSurface * surf, int xoffset, int yoffset, int width, int height, const void * buffer) {
    int srcbpp = 4;
    int dstbpp = 4;
    if (surf) {
        uint8_t * surfbuf = (uint8_t*) Xe_Surface_LockRect(g_pVideoDevice, surf, 0, 0, 0, 0, XE_LOCK_WRITE);
        uint8_t * srcdata = (uint8_t*) buffer;
        uint8_t * dstdata = surfbuf;
        int srcbytes = srcbpp;
        int dstbytes = dstbpp;
        int y, x;

        int pitch_d = (surf->wpitch);
        int pitch_s = (bitmap.pitch);
        int offset_d = 0;
        int offset_s = 0;

        for (y = yoffset; y < (yoffset + height); y++) {
            offset_d = (y * pitch_d)+(xoffset * dstbytes);
            offset_s = (y * pitch_s)+(xoffset * srcbytes);

            dstdata = surfbuf + offset_d; // ok
            srcdata = buffer + offset_s; // ok

            for (x = xoffset; x < (xoffset + width); x++) {
                if (srcbpp == 4 && dstbytes == 4) {
                    dstdata[0] = srcdata[0];
                    dstdata[1] = srcdata[1];
                    dstdata[2] = srcdata[2];
                    dstdata[3] = srcdata[3];

                    srcdata += srcbytes;
                    dstdata += dstbytes;
                }
            }
        }

        Xe_Surface_Unlock(g_pVideoDevice, surf);
    }
}

// g_genplus.cpp
int getVideoFitler();

enum VF_VALUES {
    VF_NONE,
    VF_BLINEAR,
    VF_2XSAI,
    VF_MAX
};

void SYSVideoUpdate() {
    /* resize uv to viewport */
    int vwidth = bitmap.viewport.w + (2 * bitmap.viewport.x);
    int vheight = bitmap.viewport.h + (2 * bitmap.viewport.y);

    update_texture_viewport();
    
    int video_filter = getVideoFitler();

    // apply video filter
    switch (video_filter){
        case VF_2XSAI:
            filter_Std2xSaI_ex8(bitmap.data, bitmap.pitch, texture_buffer, vwidth, vheight);
            g_pTexture->width = vwidth<<1;
            g_pTexture->height = vheight<<1;
            break;
        case VF_BLINEAR:
            g_pTexture->use_filtering = 0;
            XeTexSubImage(g_pTexture, bitmap.viewport.x, bitmap.viewport.y, bitmap.viewport.w, bitmap.viewport.h, bitmap.data);
            g_pTexture->width = vwidth;
            g_pTexture->height = vheight;
            break;
        default:
            g_pTexture->use_filtering = 1;
            XeTexSubImage(g_pTexture, bitmap.viewport.x, bitmap.viewport.y, bitmap.viewport.w, bitmap.viewport.h, bitmap.data);
            g_pTexture->width = vwidth;
            g_pTexture->height = vheight;
            break;
    }

    // Refresh texture cash
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);
        

    // Select stream and shaders
    Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, 0, sizeof (DrawVerticeFormats));
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    Xe_SetClearColor(g_pVideoDevice, 0xFF000000);
    // Draw
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);

    // Resolve
    Xe_Resolve(g_pVideoDevice);
    //while (!Xe_IsVBlank(g_pVideoDevice));
    Xe_Sync(g_pVideoDevice);
    
    // Reset states
    Xe_InvalidateState(g_pVideoDevice);
}