#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "osd.h"
#include "xeinput.h"


static struct controller_data_s ctrl[MAX_INPUTS];

void SYSInputUpdate() {
   
    usb_do_poll();
    int joynum = 0;

    for (joynum = 0; joynum < MAX_INPUTS; joynum++) {

        get_controller_data(&ctrl[joynum],joynum);
        
        /* reset input */
        input.pad[joynum] = 0;

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
}
