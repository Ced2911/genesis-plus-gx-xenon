#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>
#include <ppc/timebase.h>

#include "osd.h"

#define OSD_TEXT_DISPLAY_TIMEOUT 120

static int menuEnabled = 0;
static uint64_t lastDisplayTextTime = 0;
static char displayedText[512];
static int bDisplayText = 0;

void SYSMenuEnable(int enable){
    menuEnabled = enable;
}

void SYSMenuInit(){
    menuEnabled = 0;
}

void SYSMenuUpdate(){
    if(menuEnabled){
        
    }
    
    if(bDisplayText){
        uint64_t now = mftb();
        if(tb_diff_msec(now, lastDisplayTextTime)){
            bDisplayText = 0;
        }
    }
}

void SYSMenuDisplayString( char * format, ... ){
    va_list args;
    va_start (args,format);
    vsprintf (displayedText,format, args);
    va_end (args);
    
    // display string
    lastDisplayTextTime = mftb();
    bDisplayText = 1;
    
    printf("%s\r\n",displayedText);
}