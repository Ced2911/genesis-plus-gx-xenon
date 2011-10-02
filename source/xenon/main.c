#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"


#include "xesys.h"
#include "xeaudio.h"
#include "xevideo.h"
#include "xeinput.h"


void osd_input_Update() {
    //usb_do_poll();
}


int main() {
   
    SYSInit();
    SYSVideoInit();
    SYSAudioInit();


    /* set default config */
    error_init();
    set_config_defaults();

    /* Load ROM file */
    cart.rom = malloc(10 * 1024 * 1024);
    memset(cart.rom, 0, 10 * 1024 * 1024);
    //
    //if (!load_rom("uda:/sonic.smd")) {
   // if (!load_rom("uda:/roms/Sonic the Hedgehog 3 (E).zip")) {
   if (!load_rom("uda:/roms/Sonic and Knuckles & Sonic 3 (JUE) [!].zip")) {
        printf("Error loading file `%s'.", "uda:/Sonic the Hedgehog 3 (E).zip");
        return 1;
    }

    SYSClose();

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

    config.overscan = 0;// disabled

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
    
    /* run 1 frame */
    system_frame(0);
    
    int running = 1;
    /* emulation loop */
    while (running) {
        system_frame(0);
        SYSUpdate();
        SYSVideoUpdate();
        SYSAudioUpdate();
        SYSInputUpdate();
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

    return 0;
}