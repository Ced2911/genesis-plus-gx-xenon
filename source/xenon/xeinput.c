#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "osd.h"
#include "xesys.h"
#include "xeinput.h"

static struct controller_data_s ctrl[MAX_INPUTS];
static struct controller_data_s old_ctrl[MAX_INPUTS];

void SYSInputReset() {
    memset(ctrl, 0, MAX_INPUTS * sizeof (struct controller_data_s));
    memset(old_ctrl, 0, MAX_INPUTS * sizeof (struct controller_data_s));
}

void SYSInputUpdate() {

    usb_do_poll();
    int joynum = 0;

    for (joynum = 0; joynum < MAX_INPUTS; joynum++) {

        get_controller_data(&ctrl[joynum], joynum);

        /* reset input */
        input.pad[joynum] = 0;

        /*
                if(ctrl[joynum].logo){
                    if(old_ctrl[joynum].select != ctrl[joynum].select){
                        SYSSaveState();
                    }
                    if(old_ctrl[joynum].start != ctrl[joynum].start){
                        SYSLoadState();
                    }
                    if(old_ctrl[joynum].up != ctrl[joynum].up){
                        SYSSelectStates(SYSGetStatesNumber()+1);
                    }
                    if(old_ctrl[joynum].down != ctrl[joynum].down){
                        SYSSelectStates(SYSGetStatesNumber()-1);
                    }
                    if(ctrl[joynum].lb && ctrl[joynum].rb ){
                        system_init();
                        system_reset();
                    }
                }
                else
         */
        // ask to leave
        if ((old_ctrl[joynum].logo == 1) && (ctrl[joynum].logo == 0)) {
            osd_call(-1);
        }

        {

            switch (input.dev[joynum]) {
                case DEVICE_PAD3B:
                case DEVICE_PAD6B:
                case DEVICE_PAD2B:

                    if (ctrl[joynum].a) input.pad[joynum] |= INPUT_A;
                    if (ctrl[joynum].b) input.pad[joynum] |= INPUT_B;
                    if (ctrl[joynum].rb) input.pad[joynum] |= INPUT_C;
                    if (ctrl[joynum].start) input.pad[joynum] |= INPUT_START;
                    if (ctrl[joynum].x) input.pad[joynum] |= INPUT_X;
                    if (ctrl[joynum].y) input.pad[joynum] |= INPUT_Y;
                    if (ctrl[joynum].lb) input.pad[joynum] |= INPUT_Z;
                    if (ctrl[joynum].select) input.pad[joynum] |= INPUT_MODE;

                    if (ctrl[joynum].up) input.pad[joynum] |= INPUT_UP;
                    else
                        if (ctrl[joynum].down) input.pad[joynum] |= INPUT_DOWN;
                    if (ctrl[joynum].left) input.pad[joynum] |= INPUT_LEFT;
                    else
                        if (ctrl[joynum].right) input.pad[joynum] |= INPUT_RIGHT;
                default:
                    break;
            }
        }
        old_ctrl[joynum] = ctrl[joynum];
    }
}
