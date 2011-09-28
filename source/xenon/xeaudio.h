#include <xenon_sound/sound.h>

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE  2048

void SYSAudioInit();
void SYSAudioFree();
void SYSAudioUpdate();
