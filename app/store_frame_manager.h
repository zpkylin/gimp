/*
 *
 */

#ifndef STORE_FRAME_MANAGER
#define STORE_FRAME_MANAGER

#include "gdisplay.h"

void sfm_create_gui (GDisplay *);
void sfm_onionskin_set_offset (GDisplay*, int, int);
void sfm_onionskin_rm (GDisplay*);
void sfm_store_add_image (GImage *, GDisplay *, int, int, int);
void sfm_setting_aofi(GDisplay *);
void sfm_dirty (GDisplay *);

#endif

