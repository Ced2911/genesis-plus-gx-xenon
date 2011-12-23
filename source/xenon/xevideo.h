#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>

void SYSVideoInit();
void SYSVideoUpdate();
void update_texture_viewport(int width, int height);

extern struct XenosSurface * g_pTexture;
extern unsigned char* screen;