#ifndef __ZOOM_BOOKMARK_H__
#define __ZOOM_BOOKMARK_H__


#define ZOOM_BOOKMARK_NUM 4

#include "gdisplay.h"

typedef struct 
{
   gint zoom;
   gint image_offset_x;
   gint image_offset_y;
} ZoomBookmark;

extern ZoomBookmark zoom_bookmarks[ZOOM_BOOKMARK_NUM];

void zoom_bookmark_jump_to(const ZoomBookmark* mark, GDisplay *gdisp);
void zoom_bookmark_set(ZoomBookmark* mark, const GDisplay *gdisp);
int zoom_bookmark_save(const char *filename);
int zoom_bookmark_load(const char *filename);

#endif
