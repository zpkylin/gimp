/*
 * This is the base class for the frame managers
 */

#ifndef	BASE_FRAME_MANAGER
#define BASE_FRAME_MANAGER

#include "gimage.h"
#include "gdisplay.h"

typedef struct _store_frame_manager store_frame_manager;
typedef struct _clip_frame_manager clip_frame_manager;


struct _base_frame_manager
{
  GImage *fg;
  GImage *bg;
  char onionskin;
  clip_frame_manager *cfm;
  store_frame_manager *sfm; 
};

void bfm_create_sfm (GDisplay *);
void bfm_create_cfm (GDisplay *);
void bfm_delete_sfm (GDisplay *);
void bfm_delete_cfm (GDisplay *);


GImage* bfm_get_fg (GDisplay *);
GImage* bfm_get_bg (GDisplay *);
void bfm_set_dirty_flag (GDisplay *, int);

char bfm_onionskin (GDisplay *);
void bfm_onionskin_set_offset (GDisplay *, int, int);
void bfm_onionskin_rm (GDisplay *);

void bfm_next_filename (GImage *, char **, char **, char, GDisplay *);
char* bfm_get_name (GImage *image);
char* bfm_get_frame (GImage *image);
char* bfm_get_ext (GImage *image);
void bfm_this_filename (GImage *, char **, char **, char *);

/*
 * This is the store frame manager
 */
typedef struct
{
  GImage *gimage;
  char readonly;
  char advance;
  char flip;
  char fg;
  char bg;
  char selected;
  char active;
  /* GUI */
  GtkWidget *iadvance;
  GtkWidget *iflip;
  GtkWidget *ibg;
  GtkWidget *idirty;
  GtkWidget *ireadonly;
  GtkWidget *label;
  GtkWidget *list_item;
}store;

struct _store_frame_manager
{
  GSList *stores;
  gint selected;
  gint readonly;
  /* GUI */
  GtkWidget *shell;  
  GtkWidget *store_list;  
  GtkWidget *trans_data;  
  GtkWidget *add_dialog;  
  GtkWidget *num_to_add;  
};

/*
 * This is the clip frame manager
 */
typedef struct
{
  GImage *gimage;
}frame;

typedef struct
{
  GSList *frames;
}clip;

struct _clip_frame_manager
{
  GSList *fg_clip;
  GSList *bg_clip;
  /* GUI */
};

#endif

