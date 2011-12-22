#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>

#include "osd.h"

#include "xeaudio.h"

static int sound_initialised = 0;

uint16_t * pAudioBuffer = NULL;
uint16_t * pAudioStart = NULL;

void SYSAudioFree() {
    free(pAudioBuffer);
}

void SYSAudioInit() {
    if (sound_initialised == 0) {
        xenon_sound_init();

        pAudioStart = pAudioBuffer = (uint16_t*) malloc(48000 * sizeof (uint16_t));
        memset(pAudioBuffer, 0, 48000 * sizeof (uint16_t));

        sound_initialised = 1;
    }
}

void SYSAudioUpdate() {
    int size = audio_update() * 4;

    int i;

    uint32_t * p = (uint32_t *) pAudioBuffer;

    for (i = 0; i < size; ++i) {
        *pAudioBuffer = snd.buffer[0][i];
        ++pAudioBuffer;
        *pAudioBuffer = snd.buffer[1][i];
        ++pAudioBuffer;
    }


    for (i = 0; i < size; i++) {
        p[i] = bswap_32(p[i]);
    }


    if (pAudioBuffer - pAudioStart > 10000)
        pAudioBuffer = pAudioStart;

    while (xenon_sound_get_unplayed()>(4 * size)) udelay(50);

    xenon_sound_submit(p, size);
}