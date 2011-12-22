#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdio.h>
#include <math.h>
#include <zlib.h>

#include "types.h"
#include "macros.h"
#include "m68k/m68k.h"
#include "z80/z80.h"
#include "system.h"
#include "genesis.h"
#include "vdp_ctrl.h"
#include "vdp_render.h"
#include "mem68k.h"
#include "memz80.h"
#include "membnk.h"
#include "io_ctrl.h"
#include "input_hw/input.h"
#include "state.h"
#include "sound/sound.h"
#include "sound/sn76489.h"
#include "sound/ym2413.h"
#include "sound/ym2612.h"
#include "loadrom.h"
#include "cart_hw/sms_cart.h"
#include "cart_hw/md_cart.h"
#include "cart_hw/md_eeprom.h"
#include "cart_hw/gg_eeprom.h"
#include "cart_hw/sram.h"
#include "cart_hw/ggenie.h"
#include "cart_hw/areplay.h"
#include "svp.h"
#include "osd.h"

#endif /* _SHARED_H_ */

