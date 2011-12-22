#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <usb/usbmain.h>
#include <console/console.h>
#include <xenon_smc/xenon_smc.h>
#include <xenon_soc/xenon_power.h>
#include <ppc/timebase.h>
#include <time/time.h>
#include <diskio/ata.h>
#include "xeaudio.h"
#include "xeinput.h"
#include "xesys.h"
#include "xevideo.h"
#include "xemenu.h"
#include "osd.h"

static uint8_t state_buf[STATE_SIZE];
static char state_filename[256];
static int states_nbr = 0;

void SYSSelectStates(int n) {
    states_nbr = n;

    if (n < 0)
        states_nbr = 0;
    if (n > 10)
        states_nbr = 10;

    sprintf(state_filename, "uda:/states/%08x-%02d.gpz", rominfo.realchecksum, states_nbr);

    //printf("%s\r\n",state_filename);
    SYSMenuDisplayString("Switched to states n°%02d", states_nbr);
}

int SYSGetStatesNumber() {
    return states_nbr;
}

void SYSLoadState() {
    FILE *f = fopen(state_filename, "r+b");
    if (f) {
        fread(&state_buf, STATE_SIZE, 1, f);
        state_load(state_buf);
        fclose(f);

        SYSMenuDisplayString("Loaded state from %s", state_filename);
    }
}

void SYSSaveState() {
    FILE *f = fopen(state_filename, "w+b");
    if (f) {
        state_save(state_buf);
        fwrite(&state_buf, STATE_SIZE, 1, f);
        fclose(f);

        SYSMenuDisplayString("Saved state to %s", state_filename);
    }
}

void SYSInit() {

}

void SYSClose() {
}

void SYSUpdate() {
    SYSVideoUpdate();
    SYSAudioUpdate();
    SYSInputUpdate();
    SYSMenuUpdate();
}
