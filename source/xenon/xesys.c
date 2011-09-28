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

#include "osd.h"

// FPS
#define TIMEBASE 100000

double draw_fps = 0;

unsigned long time_get_time() {
    return mftb() / (PPC_TIMEBASE_FREQ / TIMEBASE);
}

void frame_cap(int fps) {
    static unsigned int last_time = 0;
    unsigned int c = time_get_time();

    if (!fps) {
        last_time = c;
        return;
    }

    if (last_time + (100000/fps) < c) {
        last_time = c;
        return;
    }

    while (last_time + (100000/fps) - 20 > c) {
        udelay(((100000/fps) - 20 - (c - last_time)) * 10 );
        c = time_get_time();
    }

    last_time = c;

}

void compute_fps() {
    static int count = 0;
    static unsigned int last_time = 0;
    unsigned int c = time_get_time();
    ++count;
    if((c - last_time) > 100000) {
        draw_fps = ((double)count) / ((double)(c - last_time)) * 100000;
        count = 0;
        last_time = c;
    }
}


void SYSInit(){
     xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);
    
    
    console_init();

    usb_init();
    usb_do_poll();
}

void SYSClose(){
    console_close();
}

void SYSUpdate(){
    frame_cap(vdp_pal ? 60 : 50);
}