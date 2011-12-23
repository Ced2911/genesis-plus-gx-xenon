#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"

#include "xesys.h"
#include "xeaudio.h"
#include "xevideo.h"
#include "xeinput.h"
#include "xemenu.h"
}
#include <ppc/timebase.h>

#include "xenon/config.h"
#undef SYSTEM_SG
#undef    SYSTEM_SG
#undef   SYSTEM_MARKIII
#undef   SYSTEM_SMS
#undef   SYSTEM_SMS2
#undef   SYSTEM_GG
#undef  SYSTEM_MD
#undef     NO_DEVICE
#undef   DEVICE_PAD3B
#undef  DEVICE_PAD6B
#undef  DEVICE_PAD2B
#undef  DEVICE_MOUSE
#undef  DEVICE_LIGHTGUN
#undef  DEVICE_PADDLE
#undef   DEVICE_SPORTSPAD
#undef   DEVICE_PICO
#undef   DEVICE_TEREBI
#undef  DEVICE_XE_A1P
#undef   DEVICE_ACTIVATOR
#include "genesis_settings.h"

int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);
void ErrorPrompt(const char *msg);

static int running = 1;
static int gen_exit = 0;

void osd_input_Update() {
    //usb_do_poll();
}

extern "C" void osd_call(int i) {
    SYSInputReset();
    if (i == -1) {
//        if (WindowPrompt("Leave", "Are you sure to continue to the menu ?", "Ok", "Cancel")) {
            running = 0;
//        }
    }
}

void FirstRun() {
    printf("FirstRun\r\n");
    SYSInit();
    SYSVideoInit();
    SYSAudioInit();
    SYSMenuInit();
}

static void SetGenConfig() {
    config.filter = gensettings.video_filter;
    config.lock_on = gensettings.lock_on;


    input.system[0] = gensettings.input_type;
    config.input[0].padtype = gensettings.device_type;

    input.system[1] = gensettings.input_type;
    config.input[1].padtype = gensettings.device_type;

    config.overscan = gensettings.overscan;

    config.region_detect = gensettings.region;
    config.system = gensettings.system;

    config.ym2413 = gensettings.ym2413;
}

uint8_t state_buf[STATE_SIZE];

void save_sram(const char *dest) {
    FILE *f = fopen(dest, "w+b");
    if (f != NULL) {
        fwrite(sram.sram, 0x10000, 1, f);
        fclose(f);
    }
}

void load_sram(const char *dest) {
    FILE *f = fopen(dest, "rb");
    if (f) {
        fread(sram.sram, 0x10000, 1, f);
        fclose(f);
    }
}

void save_state(const char *dest) {
    FILE *f = fopen(dest, "w+b");
    if (f) {
        state_save(state_buf);
        fwrite(&state_buf, STATE_SIZE, 1, f);
        fclose(f);
        SYSMenuDisplayString("Saved state to %s", dest);
    }
}

void load_state(const char *dest) {
    FILE *f = fopen(dest, "rb");
    if (f) {
        fread(&state_buf, STATE_SIZE, 1, f);
        state_load(state_buf);
        fclose(f);
        SYSMenuDisplayString("Loaded state from %s", dest);
    }
}

static char romname[1024];
static char sramname[1024];
static char statename[1024];

int genesis_init() {
    static int bfirstRun = true;

    if (bfirstRun) {
        FirstRun();
        bfirstRun = false;
    }

    /* set default config */
    error_init();
    set_config_defaults();

    /* user cfg */
    SetGenConfig();

    /* Load ROM file */
    cart.rom = (unsigned char*) malloc(10 * 1024 * 1024);
    memset(cart.rom, 0, 10 * 1024 * 1024);
    //
    //if (!load_rom("uda:/sonic.smd")) {
    if (!load_rom(romname)) {
        char buf[1024];
        sprintf(buf, "Error loading file `%s'.", romname);
        ErrorPrompt(buf);
        free(cart.rom);
        return 1;
    }

    // now running
    running = 1;
    // not exit
    gen_exit = 0;

    SYSInputReset();

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
    bitmap.width = g_pTexture->width;
    bitmap.height = g_pTexture->height;
    bitmap.pitch = g_pTexture->wpitch;

    bitmap.data = screen;
    bitmap.viewport.changed = 3;

    //config.overscan = 0; // disabled


    /* initialize emulation */
    printf("audio_init\r\n");
    audio_init(SOUND_FREQUENCY, vdp_pal ? 50.0 : 60.0);
    printf("system_init\r\n");
    system_init();

    printf("sramname\r\n");
    /* load SRAM */
    if (gensettings.saves & SAVES_SRAM) {
        load_sram(sramname);
    }


    /* reset emulation */
    system_reset();

    /* run 1 frame */
    system_frame(0);

    if (gensettings.saves & SAVES_STATES) {
        load_state(statename);
    }

    return 0;
}

static void genesis_leave() {
    if (gensettings.saves & SAVES_SRAM) {
        /* save SRAM */
        save_sram(sramname);
    }

    if (gensettings.saves & SAVES_STATES) {
        save_state(statename);
    }

    system_shutdown();
    //    audio_shutdown();
    error_shutdown();
    free(cart.rom);
}

static void genesis_loop() {
    // set genesis config
    SetGenConfig();
    // restore viewport
//    update_texture_viewport(bitmap.viewport.w, bitmap.viewport.h);
    
    /* emulation loop */
    while (running) {
        system_frame(0);
        SYSUpdate();
    }

    if (gen_exit == 1)
        genesis_leave();
}

char ROMFilename[256];
char foldername[1024];

int genesis_main(const char *root, const char * dir, const char *filename) {
    sprintf(romname, "%s/%s/%s", root, dir, filename);
    sprintf(sramname, "%s/%s/%s.srm", root, dir, filename);
    sprintf(statename, "%s/%s/%s.gpz", root, dir, filename);

    strcpy(ROMFilename,filename);
    strcpy(foldername,dir);
    
    return 0;
}

void genesis_reset() {
    /* restart emulation */
    system_init();
    system_reset();
}

void genesis_resume() {
    running = 1;
    gen_exit = 0;
        
    genesis_loop();
}

void genesis_exit() {
    gen_exit = 1;
    running = 0;
    genesis_loop();
}