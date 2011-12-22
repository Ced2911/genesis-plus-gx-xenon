/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 * Modified by Ced2911, 2011
 *
 * input.h
 * Wii/GameCube controller management
 ***************************************************************************/

#ifndef _INPUT_H_
#define _INPUT_H_
#include "w_input.h"
#include <input/input.h>

#define PI 				3.14159265f
#define PADCAL			50

extern int rumbleRequest[4];

void SetupPads();
void UpdatePads();
void ShutoffRumble();
void DoRumble(int i);

#endif
