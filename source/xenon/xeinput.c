#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "osd.h"
#include "xeinput.h"

static struct controller_data_s ctrl[MAX_INPUTS];
static struct controller_data_s old_ctrl[MAX_INPUTS];
static uint8_t state_buf[STATE_SIZE];
static char state_filename[256];
static int states_nbr = 0;

static void SelectStates(int n){
    states_nbr = n;
    
    if(n<0)
        states_nbr=0;
    if(n>10)
        states_nbr=10;
        
    sprintf(state_filename,"uda:/states/%08x-%02d.gpz",rominfo.realchecksum,states_nbr);
    
    printf("%s\r\n",state_filename);
}

static void LoadState(){
    FILE *f = fopen(state_filename,"r+b");
    if (f)
    {
      fread(&state_buf, STATE_SIZE, 1, f);
      state_load(state_buf);
      fclose(f);
    }
}


static void SaveState(){
    FILE *f = fopen(state_filename,"w+b");
    if (f)
    {
      state_save(state_buf);
      fwrite(&state_buf, STATE_SIZE, 1, f);
      fclose(f);
    }
}

void SYSInputUpdate() {
   
    usb_do_poll();
    int joynum = 0;

    for (joynum = 0; joynum < MAX_INPUTS; joynum++) {

        get_controller_data(&ctrl[joynum],joynum);
        
        /* reset input */
        input.pad[joynum] = 0;
        
        if(ctrl[joynum].logo){
            if(old_ctrl[joynum].select != ctrl[joynum].select){
                SaveState();
            }
            if(old_ctrl[joynum].start != ctrl[joynum].start){
                LoadState();
            }
            if(old_ctrl[joynum].up != ctrl[joynum].up){
                SelectStates(states_nbr+1);
            }
            if(old_ctrl[joynum].down != ctrl[joynum].down){
                SelectStates(states_nbr-1);
            }
            if(ctrl[joynum].lb && ctrl[joynum].rb ){
                system_init();
                system_reset();
            }
        }
        else{

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
        old_ctrl[joynum]=ctrl[joynum];
    }
}
