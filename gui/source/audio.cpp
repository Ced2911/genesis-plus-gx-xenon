/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 * Modified by Ced2911, 2011
 * 
 * audio.cpp
 * Audio support
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#include <xenon_sound/sound.h>

/****************************************************************************
 * InitAudio
 *
 * Initializes the Wii's audio subsystem
 ***************************************************************************/
void InitAudio() {
    xenon_sound_init();
}

/****************************************************************************
 * ShutdownAudio
 *
 * Shuts down audio subsystem. Useful to avoid unpleasant sounds if a
 * crash occurs during shutdown.
 ***************************************************************************/
void ShutdownAudio() {

}
