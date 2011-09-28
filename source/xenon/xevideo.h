#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>

void SYSVideoInit();
void SYSVideoUpdate();

extern struct XenosSurface * g_pTexture;
extern unsigned char* screen;