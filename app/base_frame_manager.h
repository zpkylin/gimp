/*
 * This is the base class for the frame managers
 */

#ifndef	BASE_FRAME_MANAGER
#define BASE_FRAME_MANAGER

#include "gimage.h"
#include "gdisplay.h"
#include "procedural_db.h"

typedef struct _store_frame_manager store_frame_manager;
typedef struct _clip_frame_manager clip_frame_manager;


struct _base_frame_manager
{
  GImage *fg;
  GImage *bg;
  char onionskin;
  clip_frame_manager *cfm;
  store_frame_manager *sfm;
  char *src_dir;
  char *dest_dir;  
  char *src_name;
  char *dest_name;  
};

void bfm_create_sfm (GDisplay *);
void bfm_create_cfm (GDisplay *);
void bfm_delete_sfm (GDisplay *);
void bfm_delete_cfm (GDisplay *);

int bfm (GDisplay *);

GImage* bfm_get_fg (GDisplay *);
GImage* bfm_get_bg (GDisplay *);
void bfm_dirty (GDisplay *);

char bfm_onionskin (GDisplay *);
void bfm_onionskin_set_offset (GDisplay *, int, int);
void bfm_onionskin_rm (GDisplay *);
void bfm_set_fg (GDisplay *, GImage *);
void bfm_set_bg (GDisplay *, GImage *);
GimpDrawable* bfm_get_fg_drawable (GDisplay *);
GimpDrawable* bfm_get_bg_drawable (GDisplay *);
int bfm_get_fg_display (GDisplay *);
int bfm_get_bg_display (GDisplay *);
void bfm_onionskin_display (GDisplay *, double, int, int, int, int); 

void bfm_next_filename (GImage *, char *, char *, char, GDisplay *);
char* bfm_get_name (char *);
char* bfm_get_frame (char *);
char* bfm_get_ext (char *);
void bfm_this_filename (char *, char *, char *, char *, int);

GDisplay* bfm_load_image_into_fm (GDisplay *, GImage *);
extern ProcRecord bfm_set_dir_src_proc; 
extern ProcRecord bfm_set_dir_dest_proc; 
void bfm_set_dir_src (GDisplay *, char*); 
void bfm_set_dir_dest (GDisplay *, char*); 

/*
 * This is the store frame manager
 */
typedef struct
{
  GImage *gimage;
  GImage *new_gimage;
  char readonly;
  char advance;
  char flip;
  char fg;
  char bg;
  char selected;
  char active;
  char special;
  char remove; 
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
  gint fg;
  gint bg;
  gint readonly;
  gint s_x, s_y, e_x, e_y;
  gint sx, sy, ex, ey;
  gint play;
  gint load_smart;
  /* GUI */
  GtkWidget *aofi;
  GtkWidget *autosave;
  GtkWidget *onionskin;
  GtkWidget *shell;  
  GtkWidget *store_list;  
  GtkWidget *onionskin_val;  
  GtkWidget *add_dialog;  
  GtkWidget *chg_frame_dialog;  
  GtkWidget *change_to;   
  GtkWidget *num_to_add;  
  GtkWidget *num_to_adv;  
  GtkWidget *src_dir;  
  GtkWidget *dest_dir;  
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

