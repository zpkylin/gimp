#include <stdio.h>
#include "zoombookmark.h"
#include "scale.h"
#include "scroll.h"

ZoomBookmark zoom_bookmarks[ZOOM_BOOKMARK_NUM];
static int zoom_bookmark_is_init = FALSE;

static void zoom_bookmark_check_init();

void zoom_bookmark_check_init()
{
   int i=0;

   if (!zoom_bookmark_is_init) {
      for (i = 0; i < ZOOM_BOOKMARK_NUM; i++) {
         zoom_bookmarks[i].image_offset_x = 0;
         zoom_bookmarks[i].image_offset_y = 0;
         zoom_bookmarks[i].zoom = 101;
      }

      zoom_bookmark_is_init = TRUE;
   }

}

void zoom_bookmark_jump_to(const ZoomBookmark* mark, GDisplay *gdisp)
{
   zoom_bookmark_check_init();

   change_scale(gdisp, mark->zoom);
   scroll_display(gdisp, mark->image_offset_x - gdisp->offset_x, 
		         mark->image_offset_y - gdisp->offset_y);
}

void zoom_bookmark_set(ZoomBookmark* mark, const GDisplay *gdisp)
{
   zoom_bookmark_check_init();

   mark->image_offset_x = gdisp->offset_x;
   mark->image_offset_y = gdisp->offset_y;
   mark->zoom = SCALEDEST(gdisp) * 100 + SCALESRC(gdisp);
}

int zoom_bookmark_save(const char *filename)
{
   int i=0;
   FILE *file = 0;
   zoom_bookmark_check_init();

   file = fopen(filename, "wt");
   if (!file) {
      printf("Could not open file %s\n", filename);
      return 0;
   }

   fprintf(file, "%d\n", ZOOM_BOOKMARK_NUM);
   for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
      fprintf(file, "%d %d %d\n", zoom_bookmarks[i].zoom, zoom_bookmarks[i].image_offset_x,
		      zoom_bookmarks[i].image_offset_y);
   }

   fclose(file);
   return 1;
}

int zoom_bookmark_load(const char *filename)
{
   int i=0,num=0;
   FILE *file =0;
   zoom_bookmark_check_init();

   file = fopen(filename, "rt");
   if (!file) {
      printf("Could not open file %s\n", filename);
      return 0;
   }

   fscanf(file, "%d\n", &num);
   if (ZOOM_BOOKMARK_NUM < num)
      num = ZOOM_BOOKMARK_NUM;

   for (i=0; i < num; i++) {
      fscanf(file, "%d %d %d\n", &zoom_bookmarks[i].zoom, &zoom_bookmarks[i].image_offset_x,
		      &zoom_bookmarks[i].image_offset_y);
   }

   fclose(file);
   return 1;
}

