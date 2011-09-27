#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <xenos/xe.h>
#include <xenon_sound/sound.h>
#include <diskio/ata.h>
#include <ppc/cache.h>
#include <ppc/timebase.h>
#include <pci/io.h>
#include <input/input.h>
#include <xenon_smc/xenon_smc.h>
#include <console/console.h>
#include <xenon_soc/xenon_power.h>
#include <usb/usbmain.h>
#include <ppc/timebase.h>
#include <console/console.h>

#include <ppc/timebase.h>
#include <time/time.h>
#include <time.h>
typedef unsigned int DWORD;
#include "ps.h"
#include "vs.h"

#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE  2048

#define VIDEO_WIDTH  320
#define VIDEO_HEIGHT 240


static struct XenosVertexBuffer *vb = NULL;
static struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;
static struct XenosSurface * g_pTexture = NULL;
static struct XenosDevice _xe;

static unsigned char* screen = NULL;
static uint32_t pitch = 0;

typedef struct DrawVerticeFormats {
    float x, y, z, w;
    unsigned int color;
    float u, v;
} DrawVerticeFormats;

DrawVerticeFormats Rect[6];

enum {
    UvBottom = 0,
    UvTop,
    UvLeft,
    UvRight
};
float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

void init_video() {
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

    g_pTexture = Xe_CreateTexture(g_pVideoDevice, 720, 576, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);
    screen = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    pitch = g_pTexture->wpitch;
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);


    // move it to ini file
    float x = -1.0f;
    float y = -1.0f;
    float w = 2.0f;
    float h = 2.0f;

    // top left
    Rect[0].x = x;
    Rect[0].y = y + h;
    Rect[0].u = ScreenUv[UvBottom];
    Rect[0].v = ScreenUv[UvRight];
    Rect[0].color = 0;

    // bottom left
    Rect[1].x = x;
    Rect[1].y = y;
    Rect[1].u = ScreenUv[UvBottom];
    Rect[1].v = ScreenUv[UvLeft];
    Rect[1].color = 0;

    // top right
    Rect[2].x = x + w;
    Rect[2].y = y + h;
    Rect[2].u = ScreenUv[UvTop];
    Rect[2].v = ScreenUv[UvRight];
    Rect[2].color = 0;

    // top right
    Rect[3].x = x + w;
    Rect[3].y = y + h;
    Rect[3].u = ScreenUv[UvTop];
    ;
    Rect[3].v = ScreenUv[UvRight];
    Rect[3].color = 0;

    // bottom left
    Rect[4].x = x;
    Rect[4].y = y;
    Rect[4].u = ScreenUv[UvBottom];
    Rect[4].v = ScreenUv[UvLeft];
    Rect[4].color = 0;

    // bottom right
    Rect[5].x = x + w;
    Rect[5].y = y;
    Rect[5].u = ScreenUv[UvTop];
    Rect[5].v = ScreenUv[UvLeft];
    Rect[5].color = 0;

    int i = 0;
    for (i = 0; i < 6; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 6 * sizeof (DrawVerticeFormats));
    void *v = Xe_VB_Lock(g_pVideoDevice, vb, 0, 6 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    memcpy(v, Rect, 6 * sizeof (DrawVerticeFormats));
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetClearColor(g_pVideoDevice, 0);
}

void osd_input_Update() {
    usb_do_poll();
}

void video_update() {
    // refresh texture cash
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);
    
    // reset states
    Xe_InvalidateState(g_pVideoDevice);
    Xe_SetClearColor(g_pVideoDevice, 0);

    // select stream and shaders
    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, 0, sizeof (DrawVerticeFormats));
    Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    // draw and sync
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_TRIANGLELIST, 0, 2);
    Xe_Resolve(g_pVideoDevice);
    Xe_Sync(g_pVideoDevice);
}

int main() {
    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    init_video();
    usb_init();
    usb_do_poll();
    /* set default config */
    error_init();
    set_config_defaults();

    /* Load ROM file */
    cart.rom = malloc(10 * 1024 * 1024);
    memset(cart.rom, 0, 10 * 1024 * 1024);
    //Sonic the Hedgehog 3 (E).zip
    //if (!load_rom("uda:/sonic.smd")) {
    if (!load_rom("uda:/sonic.smd")) {
        char caption[256];
        printf("Error loading file `%s'.", "uda:/sonic.smd");
        exit(1);
    }

    /* load BIOS */
    memset(bios_rom, 0, sizeof (bios_rom));
    FILE *f = fopen(OS_ROM, "rb");
    if (f != NULL) {
        fread(&bios_rom, 0x800, 1, f);
        fclose(f);
        int i;
        for (i = 0; i < 0x800; i += 2) {
            uint8 temp = bios_rom[i];
            bios_rom[i] = bios_rom[i + 1];
            bios_rom[i + 1] = temp;
        }
        config.tmss |= 2;
    }

    /* initialize Genesis virtual system */
    memset(&bitmap, 0, sizeof (t_bitmap));
    bitmap.width = 720;
    bitmap.height = 576;

    //bitmap.pitch = (bitmap.width * 4);
    bitmap.pitch = pitch;

    bitmap.data = screen;
    bitmap.viewport.changed = 3;

    /* initialize emulation */
    audio_init(SOUND_FREQUENCY, vdp_pal ? 50.0 : 60.0);
    system_init();

    /* load SRAM */
    f = fopen("uda:/game.srm", "rb");
    if (f != NULL) {
        fread(sram.sram, 0x10000, 1, f);
        fclose(f);
    }

    /* reset emulation */
    system_reset();

    int running = 1;
    /* emulation loop */
    while (running) {
        system_frame(0);
        video_update();
    }

    /* save SRAM */
    f = fopen("./game.srm", "wb");
    if (f != NULL) {
        fwrite(sram.sram, 0x10000, 1, f);
        fclose(f);
    }

    system_shutdown();
    audio_shutdown();
    error_shutdown();
    free(cart.rom);
}