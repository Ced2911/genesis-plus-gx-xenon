/* 
 * File:   genesis_settings.h
 * Author: cc
 *
 * Created on 22 décembre 2011, 12:56
 */

#ifndef GENESIS_SETTINGS_H
#define	GENESIS_SETTINGS_H

// menu options
enum MENU_OPTION_ORDER{
    MO_SAVE,
    MO_INPUT,
    MO_DEVICE,
    MO_VIDEO,
    MO_OVERSCAN,
    MO_SYSTEM,
    MO_REGION,
    MO_LOCKON,
    MO_YM2413
};

enum SAVES_VALUES{
    SAVES_OFF=0,
    SAVES_SRAM=1,
    SAVES_STATES=2,
    SAVES_BOTH=3,
    SAVES_MAX
};

// genesis option
//config.ym2413         = 2; /* = AUTO (0 = always OFF, 1 = always ON) */

enum YM2413_VALUES {
    YM2413_OFF,
    YM2413_ON,
    YM2413_AUTO,
    YM2413_MAX
};

//config.system         = 0; /* = AUTO (or SYSTEM_SG, SYSTEM_MARKIII, SYSTEM_SMS, SYSTEM_SMS2, SYSTEM_GG, SYSTEM_MD) */

enum SYSTEM_VALUES {
    SYSTEM_AUTO = 0,
    SYSTEM_SG,
    SYSTEM_MARKIII,
    SYSTEM_SMS,
    SYSTEM_SMS2,
    SYSTEM_GG,
    SYSTEM_MD,
    SYSTEM_MAX
};

//  config.region_detect  = 0; /* = AUTO (1 = USA, 2 = EUROPE, 3 = JAPAN/NTSC, 4 = JAPAN/PAL) */

enum REGIONS_VALUES {
    REGIONS_AUTO = 0,
    REGIONS_USA,
    REGIONS_EUROPE,
    REGIONS_JAPAN_NTSC,
    REGIONS_JAPAN_PAL,
    REGIONS_MAX
};

//    config.lock_on        = 0; /* = OFF (can be TYPE_SK, TYPE_GG & TYPE_AR) */

enum LOCKON_VALUES {
    LOCKON_OFF = 0,
    LOCKON_SONIC_KNUCLES,
    LOCKON_GG,
    LOCKON_AR,
    LOCKON_MAX
};

//  config.overscan = 3;       /* = both ON (0 = no borders , 1 = vertical borders only, 2 = horizontal borders only) */

enum OVERSCAN_VALUES {
    OVERSCAN_NO_BORDERS,
    OVERSCAN_VERTICAL_BORDERS,
    OVERSCAN_HORIZONTAL_BORDERS,
    OVERSCAN_BOTH,
    OVERSCAN_MAX
};

// config video filter

enum VF_VALUES {
    VF_NONE,
    VF_BLINEAR,
    VF_MAX
};

// CONFIG PAD

enum INPUT_VALUES {
    INPUT_NO_SYSTEM,
    INPUT_MD_GAMEPAD,
    INPUT_MOUSE,
    INPUT_MENACER,
    INPUT_JUSTIFIER,
    INPUT_XE_A1P,
    INPUT_ACTIVATOR,
    INPUT_MS_GAMEPAD,
    INPUT_LIGHTPHASER,
    INPUT_PADDLE,
    INPUT_SPORTSPAD,
    INPUT_TEAMPLAYER,
    INPUT_WAYPLAY,
    INPUT_MAX
};

// config device

enum DEVICE_VALUES {
    /* Device type */
    NO_DEVICE,
    DEVICE_PAD3B,
    DEVICE_PAD6B,
    DEVICE_PAD2B,
    DEVICE_MOUSE,
    DEVICE_LIGHTGUN,
    DEVICE_PADDLE,
    DEVICE_SPORTSPAD,
    DEVICE_PICO,
    DEVICE_TEREBI,
    DEVICE_XE_A1P,
    DEVICE_ACTIVATOR,
    DEVICE_MAX
};

struct GenesisPlusSettings {
    int input_type;
    int device_type;
    int video_filter;
    int overscan;
    int system;
    int region;
    int lock_on;
    int ym2413;
    int saves;
};

extern GenesisPlusSettings gensettings;

void SetDefaultSettings(GenesisPlusSettings * settings);
int LoadSettings(GenesisPlusSettings * settings);
void SaveSettings(GenesisPlusSettings * settings);

void getSaveTypeString(int type, char * dest);

void getInputTypeString(int type, char * dest);
void getDeviceTypeString(int type, char * dest);
void getSystemTypeString(int type, char * dest);
void getVFTypeString(int i, char * dest);
void getOverscanTypeString(int type, char * dest);
void getRegionTypeString(int type, char * dest);
void getLockOnTypeString(int type, char * dest);
void getYM2413TypeString(int type, char * dest);


#endif	/* GENESIS_SETTINGS_H */

