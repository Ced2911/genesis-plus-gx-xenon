/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libwiigui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <xetypes.h>
#include <xenos/xe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ogcsys.h>
#include <unistd.h>
//#include <wiiuse/wpad.h>
//#include <fat.h>
#include <usb/usbmain.h>
#include <xenon_soc/xenon_power.h>
#include <debug.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "demo.h"

struct SSettings Settings;
int ExitRequested = 0;

void ExitApp() {
    //    ShutoffRumble();
    //    StopGX();
    TR;
    printf("Exit .......\r\n");
    exit(0);
}

void
DefaultSettings() {
    Settings.LoadMethod = METHOD_AUTO;
    Settings.SaveMethod = METHOD_AUTO;
    sprintf(Settings.Folder1, "libwiigui/first folder");
    sprintf(Settings.Folder2, "libwiigui/second folder");
    sprintf(Settings.Folder3, "libwiigui/third folder");
    Settings.AutoLoad = 1;
    Settings.AutoSave = 1;
}

int main() {
    xenon_make_it_faster(XENON_SPEED_FULL);

    usb_init();
    usb_do_poll();


    // uart speed patch 115200 - jtag/freeboot
    //     *(volatile uint32_t*)(0xea001000+0x1c) = 0xe6010000;

    InitVideo(); // Initialize video
    SetupPads(); // Initialize input
    InitAudio(); // Initialize audio

    //    fatInitDefault(); // Initialize file system
    InitFreeType((u8*) font_ttf, font_ttf_size); // Initialize font system

    InitGUIThreads(); // Initialize GUI

    DefaultSettings();

    MainMenu(MENU_SETTINGS);
}

extern XenosSurface * g_pTexture;

int o_main(int argc, char *argv[]) {
    xenon_make_it_faster(XENON_SPEED_FULL);

    usb_init();
    usb_do_poll();

    // uart speed patch 115200 - jtag/freeboot
    //     *(volatile uint32_t*)(0xea001000+0x1c) = 0xe6010000;

    InitVideo(); // Initialize video


    while (1) {
        Menu_DrawImg(0, 40, 640, 360, g_pTexture, 0, 1, 1, 127);
        Menu_DrawImg(1000, 500, 200, 200, g_pTexture, 0, 1, 1, 255);
        Menu_Render();
    }
    return 1;
}