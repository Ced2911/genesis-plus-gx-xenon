/* 
 * File:   genesis_settings.cpp
 * Author: cc
 * 
 * Created on 22 décembre 2011, 12:56
 */
#include <stdio.h>
#include <stdlib.h>
#include "genesis_settings.h"
extern "C" {
#include "iniparser.h"
}
#define INI_FILE "uda:/genesisplus.ini"

GenesisPlusSettings gensettings;

int LoadSettings(GenesisPlusSettings * settings) {
    SetDefaultSettings(settings);

    dictionary * ini;
    ini = iniparser_load(INI_FILE);
    if (ini == NULL) {
        fprintf(stderr, "cannot parse file: %s\n", INI_FILE);
        return -1;
    }

    iniparser_dump(ini, stderr);

    int i;

    if ((i = iniparser_getint(ini, "core:input", -1)) != -1)
        settings->input_type = i;
    if ((i = iniparser_getint(ini, "core:device", -1)) != -1)
        settings->device_type = i;
    if ((i = iniparser_getint(ini, "core:video", -1)) != -1)
        settings->video_filter = i;
    if ((i = iniparser_getint(ini, "core:overscan", -1)) != -1)
        settings->overscan = i;
    if ((i = iniparser_getint(ini, "core:region", -1)) != -1)
        settings->region = i;
    if ((i = iniparser_getint(ini, "core:system", -1)) != -1)
        settings->system = i;
    if ((i = iniparser_getint(ini, "core:lockon", -1)) != -1)
        settings->lock_on = i;
    if ((i = iniparser_getint(ini, "core:ym2413", -1)) != -1)
        settings->ym2413 = i;
    if ((i = iniparser_getint(ini, "core:saves", -1)) != -1)
        settings->saves = i;
    if ((i = iniparser_getint(ini, "core:aspect", -1)) != -1)
        settings->aspect_ratio = i;
    iniparser_freedict(ini);
    return 0;
}

void SaveSettings(GenesisPlusSettings * settings) {
    char comment[256];

    FILE * ini;

    ini = fopen(INI_FILE, "w");
    if(ini == NULL)
        return; // fail
    fprintf(ini,
            "#\r\n"
            "# Genesis plus xenon\r\n"
            "#\r\n"
            "\r\n"
            "[Core]\r\n"
            "\r\n"
            "input      = %d ;\r\n"
            "device     = %d ;\r\n"
            "video      = %d ;\r\n"
            "overscan   = %d ;\r\n"
            "region     = %d ;\r\n"
            "system     = %d ;\r\n"
            "lockon     = %d ;\r\n"
            "ym2413     = %d ;\r\n"
            "saves      = %d ;\r\n"
            "aspect     = %d ;\r\n"
            "\r\n"
            "\r\n"
            , settings->input_type, settings->device_type, settings->video_filter,
            settings->overscan, settings->region, settings->system, settings->lock_on,
            settings->ym2413, settings->saves,settings->aspect_ratio
            );
    fclose(ini);

}

void SetDefaultSettings(GenesisPlusSettings * settings) {
    settings->input_type = INPUT_MD_GAMEPAD;
    settings->device_type = DEVICE_PAD6B;
    settings->video_filter = VF_2XSAI;
    settings->overscan = OVERSCAN_NO_BORDERS;
    settings->region = REGIONS_AUTO;
    settings->system = SYSTEM_AUTO;
    settings->lock_on = LOCKON_OFF;
    settings->ym2413 = YM2413_AUTO;
    settings->saves = SAVES_BOTH;
    settings->aspect_ratio = ASPECT_RATIO_SCREECH;
}

void getAspectTypeString(int type, char* dest){
   switch (type) {
        case ASPECT_RATIO_4_3:
            sprintf(dest, "4/3");
            break;
        case ASPECT_RATIO_SCREECH:
            sprintf(dest, "Screech");
            break;
    } 
}

void getSaveTypeString(int type, char * dest) {
    switch (type) {
        case SAVES_BOTH:
            sprintf(dest, "Sram + states");
            break;
        case SAVES_SRAM:
            sprintf(dest, "Sram");
            break;
        case SAVES_STATES:
            sprintf(dest, "States");
            break;
        case SAVES_OFF:
            sprintf(dest, "Off");
            break;
    }
}

void getInputTypeString(int type, char * dest) {
    switch (type) {
        case INPUT_NO_SYSTEM:
            sprintf(dest, "Unconnected port");
            break;
        case INPUT_MD_GAMEPAD:
            sprintf(dest, "Single 3-buttons or 6-buttons Control Pad");
            break;
        case INPUT_MOUSE:
            sprintf(dest, "Sega Mouse");
            break;
        case INPUT_MENACER:
            sprintf(dest, "Sega Menacer (port B only)");
            break;
        case INPUT_JUSTIFIER:
            sprintf(dest, "Konami Justifiers (port B only)");
            break;
        case INPUT_XE_A1P:
            sprintf(dest, "XE-A1P analog controller (port A only)");
            break;
        case INPUT_ACTIVATOR:
            sprintf(dest, "Sega Activator");
            break;
        case INPUT_MS_GAMEPAD:
            sprintf(dest, "single 2-buttons Control Pad (Master System)");
            break;
        case INPUT_LIGHTPHASER:
            sprintf(dest, "Sega Light Phaser (Master System)");
            break;
        case INPUT_PADDLE:
            sprintf(dest, "Sega Paddle Control (Master System)");
            break;
        case INPUT_SPORTSPAD:
            sprintf(dest, "Sega Sports Pad (Master System)");
            break;
        case INPUT_TEAMPLAYER:
            sprintf(dest, "Multi Tap -- Sega TeamPlayer");
            break;
        case INPUT_WAYPLAY:
            sprintf(dest, "Multi Tap -- EA 4-Way Play (use both ports)");
            break;
    }
}

void getDeviceTypeString(int type, char * dest) {
    switch (type) {
        case NO_DEVICE:
            sprintf(dest, "Unconnected device (fixed ID for Team Player)");
            break;
        case DEVICE_PAD3B:
            sprintf(dest, "3-buttons Control Pad (fixed ID for Team Player)");
            break;
        case DEVICE_PAD6B:
            sprintf(dest, "6-buttons Control Pad (fixed ID for Team Player)");
            break;
        case DEVICE_PAD2B:
            sprintf(dest, "2-buttons Control Pad");
            break;
        case DEVICE_MOUSE:
            sprintf(dest, "Sega Mouse");
            break;
        case DEVICE_LIGHTGUN:
            sprintf(dest, "Sega Light Phaser, Menacer or Konami Justifiers");
            break;
        case DEVICE_PADDLE:
            sprintf(dest, "Sega Paddle Control");
            break;
        case DEVICE_SPORTSPAD:
            sprintf(dest, "Sega Sports Pad");
            break;
        case DEVICE_PICO:
            sprintf(dest, "PICO tablet");
            break;
        case DEVICE_TEREBI:
            sprintf(dest, "Terebi Oekaki tablet");
            break;
        case DEVICE_XE_A1P:
            sprintf(dest, "XE-A1P analog controller");
            break;
        case DEVICE_ACTIVATOR:
            sprintf(dest, "Activator");
            break;
    }
}

void getSystemTypeString(int type, char * dest) {
    switch (type) {
        case SYSTEM_AUTO:
            sprintf(dest, "Auto");
            break;
        case SYSTEM_SG:
            sprintf(dest, "SG-1000");
            break;
        case SYSTEM_MARKIII:
            sprintf(dest, "Mark III");
            break;
        case SYSTEM_SMS:
            sprintf(dest, "Sega Master System");
            break;
        case SYSTEM_SMS2:
            sprintf(dest, "Sega Master System 2");
            break;
        case SYSTEM_GG:
            sprintf(dest, "Game Gear");
            break;
        case SYSTEM_MD:
            sprintf(dest, "Mega Drive");
            break;
    }
}

void getVFTypeString(int type, char * dest) {
    
    switch (type) {
        case VF_NONE:
            sprintf(dest, "None");
            break;
        case VF_BLINEAR:
            sprintf(dest, "Blinear");
            break;
        case VF_2XSAI:
            sprintf(dest, "2xSai");
            break;
    }
}

void getOverscanTypeString(int type, char * dest) {
    switch (type) {
        case OVERSCAN_NO_BORDERS:
            sprintf(dest, "No borders");
            break;
        case OVERSCAN_VERTICAL_BORDERS:
            sprintf(dest, "Vertical borders only");
            break;
        case OVERSCAN_HORIZONTAL_BORDERS:
            sprintf(dest, "Horizontal borders only");
            break;
        case OVERSCAN_BOTH:
            sprintf(dest, "Both");
            break;
    }
}

void getRegionTypeString(int type, char * dest) {
    switch (type) {
        case REGIONS_AUTO:
            sprintf(dest, "Auto");
            break;
        case REGIONS_USA:
            sprintf(dest, "USA");
            break;
        case REGIONS_EUROPE:
            sprintf(dest, "Europe");
            break;
        case REGIONS_JAPAN_NTSC:
            sprintf(dest, "Japan Ntsc");
            break;
        case REGIONS_JAPAN_PAL:
            sprintf(dest, "Japan Pal");
            break;
    }
}

void getLockOnTypeString(int type, char * dest) {
    switch (type) {
        case LOCKON_OFF:
            sprintf(dest, "Off");
            break;
        case LOCKON_SONIC_KNUCLES:
            sprintf(dest, "Sonic & Knucles");
            break;
        case LOCKON_GG:
            sprintf(dest, "Game Genie");
            break;
        case LOCKON_AR:
            sprintf(dest, "Action Replay");
            break;
    }
}

void getYM2413TypeString(int type, char * dest) {
    switch (type) {
        case YM2413_OFF:
            sprintf(dest, "Off");
            break;
        case YM2413_ON:
            sprintf(dest, "On");
            break;
        case YM2413_AUTO:
            sprintf(dest, "Auto");
            break;
    }
}
