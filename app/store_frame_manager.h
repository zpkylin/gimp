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
GDisplay* sfm_load_image_into_fm (GDisplay *, GImage *);
void sfm_set_dir_src (GDisplay*, char*);

void sfm_store_add (GtkWidget *, gpointer);
void sfm_store_delete (GtkWidget *, gpointer);
void sfm_store_raise (GtkWidget *, gpointer);
void sfm_store_lower (GtkWidget *, gpointer);
void sfm_store_save (GtkWidget *, gpointer);
void sfm_store_save_all (GtkWidget *, gpointer);
void sfm_store_revert (GtkWidget *, gpointer);
void sfm_store_load (GtkWidget *, gpointer);
void sfm_store_change_frame (GtkWidget *, gpointer);

void sfm_set_dir_src (GDisplay *, char *);
void sfm_set_dir_dest (GDisplay *, char *);

void sfm_store_recent_src (GtkWidget *, gpointer);
void sfm_store_recent_dest (GtkWidget *, gpointer);
void sfm_store_load_smart (GtkWidget *, gpointer);

#endif

