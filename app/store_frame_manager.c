/*
 * STORE FRAME MANAGER
 */

#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include "stdio.h"
#include "interface.h"
#include "ops_buttons.h"
#include "general.h"
#include "actionarea.h"
#include "fileops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "rect_selectP.h"

#include "tools/forward.xpm"
#include "tools/forward_is.xpm"
#include "tools/backwards.xpm"
#include "tools/backwards_is.xpm"

typedef gint (*CallbackAction) (GtkWidget *, gpointer);
#define STORE_LIST_WIDTH        200
#define STORE_LIST_HEIGHT       150
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

static gint TYPE_TO_ADD;

static gint sfm_onionskin (GtkWidget *, gpointer);
static gint sfm_onionskin_on (GtkWidget *, gpointer);
static gint sfm_onionskin_fgbg (GtkWidget *, gpointer);

static gint sfm_auto_save (GtkWidget *, gpointer);
static gint sfm_set_aofi (GtkWidget *, gpointer);
static gint sfm_aofi (GtkWidget *, gpointer);

static void sfm_unset_aofi (GDisplay *);

static void sfm_play_backwards (GtkWidget *, gpointer);
static void sfm_play_forward (GtkWidget *, gpointer);
static void sfm_flip_backwards (GtkWidget *, gpointer);
static void sfm_flip_forward (GtkWidget *, gpointer);
static void sfm_stop (GtkWidget *, gpointer);

static void sfm_store_clear (GDisplay*);
static gint sfm_store_add (GtkWidget *, gpointer);
static gint sfm_store_delete (GtkWidget *, gpointer); 
static gint sfm_store_raise (GtkWidget *, gpointer); 
static gint sfm_store_lower (GtkWidget *, gpointer); 
static gint sfm_store_save (GtkWidget *, gpointer); 
static gint sfm_store_save_all (GtkWidget *, gpointer); 
static gint sfm_store_revert (GtkWidget *, gpointer); 
static gint sfm_store_load (GtkWidget *, gpointer); 
static gint sfm_store_change_frame (GtkWidget *, gpointer); 

static gint sfm_store_set_src (GtkWidget *, gpointer);
static gint sfm_store_recent_src (GtkWidget *, gpointer);
static gint sfm_store_set_dest (GtkWidget *, gpointer);
static gint sfm_store_recent_dest (GtkWidget *, gpointer);
static gint sfm_store_load_smart (GtkWidget *, gpointer);

static store* sfm_store_create (GDisplay*, GImage*, int, int, int);

static void sfm_store_select (GtkCList *, gint, gint, GdkEventButton *, gpointer);
static void sfm_store_unselect (GtkCList *, gint, gint, GdkEventButton *, gpointer);
static void sfm_store_set_option (GtkCList *, gint, gpointer);

static void sfm_store_make_cur (GDisplay *, int);

static void sfm_store_add_create_dialog (GDisplay *disp);

static gint
sfm_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  store_frame_manager *sfm = ((GDisplay*)data)->bfm->sfm;
  bfm_onionskin_rm ((GDisplay*)data);
  sfm_store_clear ((GDisplay*)data);

  if (GTK_WIDGET_VISIBLE (sfm->shell))
    gtk_widget_hide (sfm->shell);
  return TRUE; 
}

void
sfm_create_gui (GDisplay *disp)
{

  GtkWidget *check_button, *button, *hbox, *flip_box, *scrolled_window,
  *utilbox, *slider, *menu, *menu_item, *menu_bar;
  int i;
  char tmp[256];
  char *check_names[3] =
    {
      "Auto Save",
      "Set AofI",
      "AofI"
    };
  CallbackAction check_callbacks[3] =
    {
      sfm_auto_save,
      sfm_set_aofi,
      sfm_aofi
    };
  char *menu_names[9] =
    {
      "Add",
      "Delete",
      "Raise",
      "Lower",
      "Save",
      "Save All",
      "Revert to saved",
      "Load",
      "Change frame" 
	
    };
  char *menu2_names[5] =
    {
      "Set source",
      "Recent source",
      "Set dest",
      "Recent dest",
      "Load smart"
    };
  CallbackAction menu_callbacks[9] =
    {
      sfm_store_add,
      sfm_store_delete,
      sfm_store_raise,
      sfm_store_lower,
      sfm_store_save,
      sfm_store_save_all,
      sfm_store_revert,
      sfm_store_load,
      sfm_store_change_frame
    };
  CallbackAction menu2_callbacks[5] =
    {
      sfm_store_set_src,
      sfm_store_recent_src,
      sfm_store_set_dest,
      sfm_store_recent_dest,
      sfm_store_load_smart
    };
  OpsButton flip_button[] =
    {
	{ backwards_xpm, backwards_is_xpm, sfm_play_backwards, "Play Backwards", NULL, NULL, NULL, NULL, NULL, NULL },
	{ backwards_xpm, backwards_is_xpm, sfm_flip_backwards, "Flip Backward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ backwards_xpm, backwards_is_xpm, sfm_stop, "Stop", NULL, NULL, NULL, NULL, NULL, NULL },
	{ forward_xpm, forward_is_xpm, sfm_flip_forward, "Flip Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ forward_xpm, forward_is_xpm, sfm_play_forward, "Play Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, 0, 0, NULL, NULL, NULL, NULL }
    };

  gchar *store_col_name[6] = {"Read only", "Advance", "Flip", "Bg", "Dirty", "Filename"};
  
  /* the shell */
  disp->bfm->sfm->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (disp->bfm->sfm->shell), "sfm", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (disp->bfm->sfm->shell), FALSE, TRUE, FALSE);
  sprintf (tmp, "Store Frame Manager for %s\0", disp->gimage->filename);
  gtk_window_set_title (GTK_WINDOW (disp->bfm->sfm->shell), tmp);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->shell), "delete_event",
      GTK_SIGNAL_FUNC (sfm_delete_callback),
      disp);

  /* pull down menu */
  menu = gtk_menu_new ();
  for (i=0; i<9; i++)
    {
      menu_item = gtk_menu_item_new_with_label (menu_names[i]);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	  GTK_SIGNAL_FUNC (menu_callbacks[i]),
	  disp);
      gtk_widget_show (menu_item);
    }

  menu_bar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), menu_bar, FALSE, FALSE, 0);
  gtk_widget_show (menu_bar);
  menu_item = gtk_menu_item_new_with_label ("Store");
  gtk_widget_show (menu_item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
  gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
  
  menu = gtk_menu_new ();
  for (i=0; i<5; i++)
    {
      menu_item = gtk_menu_item_new_with_label (menu2_names[i]);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	  GTK_SIGNAL_FUNC (menu2_callbacks[i]),
	  disp);
      gtk_widget_show (menu_item);
    }

  menu_item = gtk_menu_item_new_with_label ("Dir");
  gtk_widget_show (menu_item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
  gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
  
  
  /* check buttons */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  for (i = 0; i < 3; i++)
    {
      disp->bfm->sfm->aofi = check_button = gtk_check_button_new_with_label (check_names[i]);
      gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
	  (GtkSignalFunc) check_callbacks[i],
	  disp);
      gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
      gtk_widget_show (check_button);
    }
  
  /* store window */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), scrolled_window, 
	TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  disp->bfm->sfm->store_list = gtk_clist_new_with_titles( 6, store_col_name);

  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 0);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 1);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 2);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 3);
  
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "select_row",
      GTK_SIGNAL_FUNC(sfm_store_select),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "unselect_row",
      GTK_SIGNAL_FUNC(sfm_store_unselect),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "click-column",
      GTK_SIGNAL_FUNC(sfm_store_set_option),
      disp);


  /* It isn't necessary to shadow the border, but it looks nice :) */
  gtk_clist_set_shadow_type (GTK_CLIST(disp->bfm->sfm->store_list), GTK_SHADOW_OUT);

  /* What however is important, is that we set the column widths as
   *                         * they will never be right otherwise. Note that the columns are
   *                                                 * numbered from 0 and up (to 1 in this case).
   *                                                                         */
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 0, 150);

  /* Add the CList widget to the vertical box and show it. */
  gtk_container_add(GTK_CONTAINER(scrolled_window), disp->bfm->sfm->store_list);
  gtk_widget_show(disp->bfm->sfm->store_list);

  /* flip buttons */
  flip_box = ops_button_box_new2 (disp->bfm->sfm->shell, tool_tips, flip_button, disp);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), flip_box, FALSE, FALSE, 0);
  gtk_widget_show (flip_box);

  /* onion skinning */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);


  disp->bfm->sfm->onionskin = gtk_check_button_new_with_label ("onionskin");
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->onionskin), "clicked",
      (GtkSignalFunc) sfm_onionskin_on,
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->onionskin, FALSE, FALSE, 0);
  gtk_widget_show (disp->bfm->sfm->onionskin);

  utilbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), utilbox, TRUE, TRUE, 0);
  gtk_widget_show (utilbox);

  disp->bfm->sfm->onionskin_val = (GtkWidget*) gtk_adjustment_new (1.0, 0.0, 1.0, .01, .01, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (disp->bfm->sfm->onionskin_val));
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (utilbox), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->onionskin_val), "value_changed",
      (GtkSignalFunc) sfm_onionskin,
      disp);
  gtk_widget_show (slider);

  button = gtk_button_new_with_label ("Fg/Bg");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_onionskin_fgbg,
      disp);
  gtk_widget_show (button);


  /* advance */

  /* add store */
  sfm_store_add_image (disp->gimage, disp, 0, 1, 0);
  sfm_unset_aofi (disp);

  gtk_widget_show (disp->bfm->sfm->shell); 
}


/*
 * FLIP BOOK CONTROLS
 */
static void 
sfm_play_backwards (GtkWidget *w, gpointer data)
{
}

static void 
sfm_play_forward (GtkWidget *w, gpointer data)
{
}

static void 
sfm_flip_backwards (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  row = fm->fg - 1 < 0 ? g_slist_length (fm->stores)-1 : fm->fg - 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row - 1 < 0 ? g_slist_length (fm->stores) - 1: row - 1; 
    }

  gtk_clist_select_row (fm->store_list, row, 0);
}

static void 
sfm_flip_forward (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  row = fm->fg + 1 == g_slist_length (fm->stores) ? 0 : fm->fg + 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row + 1 == g_slist_length (fm->stores)  ? 0 : row + 1; 
    }

  gtk_clist_select_row (fm->store_list, row, 0);
}

static void 
sfm_stop (GtkWidget *w, gpointer data)
{
}

/*
 * ONION SKIN
 */
static gint 
sfm_onionskin (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;

 if (fm->bg == -1 || fm->bg == fm->fg)
   {
     printf ("ERROR: you must select a bg\n");
     if (((GtkAdjustment*)fm->onionskin_val)->value != 1)
       gtk_adjustment_set_value (((GDisplay*)data)->bfm->sfm->onionskin_val, 1);
     return 1;
   } 
  gtk_toggle_button_set_active (((GDisplay*)data)->bfm->sfm->onionskin, TRUE); 
 
  bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_val)->value,
      fm->s_x, fm->s_y, fm->e_x, fm->e_y); 
  
  return 1;
}

static gint 
sfm_onionskin_on (GtkWidget *w, gpointer data)
{

  static flag = 0;
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;

  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      printf ("ERROR: you must select a bg\n");
      return 1;
    }

  if (!gtk_toggle_button_get_active (((GDisplay*)data)->bfm->sfm->onionskin))
    {
      /* rm onionskin */
      flag = 1;
      gtk_adjustment_set_value (((GDisplay*)data)->bfm->sfm->onionskin_val, 1);
      bfm_onionskin_rm ((GDisplay*)data); 
    }
  else
    {
      /* set onionskin */
      if (flag)
	gtk_toggle_button_set_active (((GDisplay*)data)->bfm->sfm->onionskin, FALSE); 
    }
  return 1;
}

static gint 
sfm_onionskin_fgbg (GtkWidget *w, gpointer data)
{
  if (!gtk_toggle_button_get_active (((GDisplay*)data)->bfm->sfm->onionskin))
    return 1;

  if (((GtkAdjustment*)((GDisplay*)data)->bfm->sfm->onionskin_val)->value)
    gtk_adjustment_set_value (((GDisplay*)data)->bfm->sfm->onionskin_val, 0);
  else
    gtk_adjustment_set_value (((GDisplay*)data)->bfm->sfm->onionskin_val, 1);

  return 1;
}

void 
sfm_onionskin_set_offset (GDisplay* disp, int x, int y)
{
}

void 
sfm_onionskin_rm (GDisplay* disp)
{
}

/*
 * STORE
 */
static void 
sfm_store_clear (GDisplay* disp)
{
}

static gint sfm_store_add (GtkWidget *w, gpointer data)
{
 
 sfm_store_add_create_dialog ((GDisplay *)data); 
  return 1; 
}

static void
sfm_store_remove (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg));
  gint new_fg, i;

  gint l = g_slist_length (fm->stores);

  if (item->gimage->dirty)
    {
    }

  new_fg = fm->fg < l-1 ? fm->fg : fm->fg - 1;

  if (fm->fg == fm->bg)
    {
      fm->bg = new_fg;
      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->bg, 3, "Bg"); 
    }
    
  
  gtk_clist_remove (fm->store_list, fm->fg);
  g_slist_remove (fm->stores, (g_slist_nth (fm->stores, fm->fg)));
  gtk_clist_select_row (fm->store_list, new_fg, 0);

  for (i=0; i<l-1; i++)
    {
      if (((store*) (g_slist_nth (fm->stores, i)))->bg)
	fm->bg = i;
    } 
  
}

static gint sfm_store_delete (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  gint l = g_slist_length (fm->stores);

  if (l == 1)
    {
      printf ("ERROR: cannot rm last store\n");
      return 1;
    }
  
   sfm_store_remove ((GDisplay *)data); 
 
  
  return 1; 
}

static gint sfm_store_raise (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  store *item;

  if (!fm->fg)
    return 1;

  gtk_clist_swap_rows (fm->store_list, fm->fg-1, fm->fg);

  item = (store*) g_slist_nth (fm->stores, fm->fg)->data;
  fm->stores = g_slist_remove (fm->stores, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg-1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg - 1 : fm->bg == fm->fg - 1 ? fm->fg : fm->bg;
  fm->fg --;
  
  return 1; 
}

static gint sfm_store_lower (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  store *item;

  gint l = g_slist_length (fm->stores);
  
  if (fm->fg == l-1)
    return 1;

  
  gtk_clist_swap_rows (fm->store_list, fm->fg+1, fm->fg);

  item = (store*) g_slist_nth (fm->stores, fm->fg)->data;
  fm->stores = g_slist_remove (fm->stores, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg+1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg + 1 : fm->bg == fm->fg + 1 ? fm->fg : fm->bg;
  fm->fg ++;
  return 1; 
}

static gint sfm_store_save (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 

  store *item = (store*) g_slist_nth (fm->stores, fm->fg)->data;

  if (item->readonly)
    {
      printf ("ERROR: trying to save a readonly file \n");
      return 1;
    }

  file_save (item->gimage, prune_filename (item->gimage->filename),
      item->gimage->filename);
  return 1; 
}

static gint sfm_store_save_all (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  store *item;
  gint i, l = g_slist_length (fm->stores);

  for (i=0; i<l; i++)
    {
      item = (store*) (g_slist_nth (fm->stores, i))->data;
      if (!item->readonly)
	file_save (item->gimage, prune_filename (item->gimage->filename),
	    item->gimage->filename);
    }
  return 1; 
}

static gint sfm_store_revert (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg))->data;


  return 1; 
}

static gint sfm_store_load (GtkWidget *w, gpointer data)
{

  return 1; 
}

static gint sfm_store_change_frame (GtkWidget *w, gpointer data)
{
  return 1; 
}

static gint 
sfm_store_set_src (GtkWidget *w, gpointer data)
{
  return 1; 
}

static gint 
sfm_store_recent_src (GtkWidget *w, gpointer data)
{
  return 1; 
}

static gint 
sfm_store_set_dest (GtkWidget *w, gpointer data)
{
  return 1; 
}

static gint 
sfm_store_recent_dest (GtkWidget *w, gpointer data)
{
  return 1; 
}

static gint 
sfm_store_load_smart (GtkWidget *w, gpointer data)
{
  return 1; 
}

static store* 
sfm_store_create (GDisplay* disp, GImage* img, int active, int num, int readonly)
{
  store *new_store;
  store_frame_manager *fm = disp->bfm->sfm;

  new_store = (store*) g_malloc (sizeof (store));
  new_store->readonly = readonly ? 1 : 0;
  new_store->advance = 1;
  new_store->flip = 1;
  new_store->bg = 0;
  new_store->gimage = img;


  if (readonly)
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, "RO");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, ""); 
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 1, "A");
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 2, "F");
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 3, "");
  if (img && img->dirty)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "");
  if (!new_store->gimage)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 5, "Empty");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 5, prune_filename (img->filename));

  if (active)
    gtk_clist_select_row (GTK_CLIST (fm->store_list), num, 0);

  return new_store;
}

static void
sfm_store_make_cur (GDisplay *gdisplay, int row)
{
  store *item;
  store_frame_manager *fm;

  if (row == -1)
    return;

  fm = gdisplay->bfm->sfm;
  fm->fg = row;

  if (g_slist_length (fm->stores) <= row)
    return;

  item = (store*)(g_slist_nth (fm->stores, row)->data);

  /* display the new store */
  gdisplay->gimage = item->gimage;
  gdisplay->ID = item->gimage->ID;
  gdisplays_update_title (gdisplay->gimage->ID);
  gdisplay_add_update_area (gdisplay, fm->s_x, fm->s_y, fm->e_x, fm->e_y);
#if 0
  if (active_tool->type == CLONE)
    clone_flip_image ();
#endif
  bfm_onionskin_set_fg (gdisplay, 
      ((store*)(g_slist_nth (fm->stores, fm->fg)->data))->gimage); 
  gdisplays_flush ();
}

static void 
sfm_store_select (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  
  if (!gdisplay || row == -1)
    return;

  sfm_store_make_cur (gdisplay, row);

}

static void 
sfm_store_unselect (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  store_frame_manager *fm;
 
  if (!gdisplay)
    return;
  
  fm = gdisplay->bfm->sfm;

  fm->fg = -1;

}

void
sfm_dirty (GDisplay* disp)
{

  if (disp->gimage->dirty)
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 4, "");

}
static void 
sfm_store_set_option (GtkCList *w, gint col, gpointer client_pointer)
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  store_frame_manager *fm;
  store *item;
  gint row;
  
  if (!gdisplay)
    return;
  
  fm = gdisplay->bfm->sfm;

  if(-1 == (row = gdisplay->bfm->sfm->fg))
    return;
  
  item = (store*) g_slist_nth (fm->stores, row)->data;
  
  switch (col)
    {
    case 0:
     item->readonly = item->readonly ? 0 : 1;
     if (item->readonly)
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 0, "RO"); 
     else
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 0, "");  
      break;
    case 1:
     item->advance = item->advance ? 0 : 1; 
     if (item->advance)
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 1, "A"); 
     else
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 1, "");  
      break;
    case 2:
     item->flip = item->flip ? 0 : 1; 
     if (item->flip)
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 2, "F"); 
     else
      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 2, "");  
      break;
    case 3:
      if (fm->bg != -1 && fm->bg != fm->fg)
	{
	  ((store*) g_slist_nth (fm->stores, fm->bg)->data)->bg = 0;
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->bg, 3, "");
	  fm->bg = -1;
	  bfm_onionskin_set_bg (((GDisplay*)client_pointer), 
	      0); 
	}
      item->bg = item->bg ? 0 : 1; 
      if (item->bg)
	{
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 3, "Bg"); 
	  fm->bg = row;
      
	  bfm_onionskin_set_bg (((GDisplay*)client_pointer), 
	      ((store*)(g_slist_nth (fm->stores, fm->bg)->data))->gimage); 
	  
	}
      else
	{
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 3, "");  
	  fm->bg = -1;
	  bfm_onionskin_set_bg (((GDisplay*)client_pointer), 
	      0); 
	}
      break;
    default:
      break;
      
    }
}

void
sfm_store_add_image (GImage *gimage, GDisplay *disp, int row, int fg, int readonly)
{
  store_frame_manager *fm = disp->bfm->sfm; 
  char *text[6]={NULL, NULL, NULL, NULL, NULL, NULL}; 
  store *new_store;

  if (row<0)
    return;

  gtk_clist_insert (GTK_CLIST (fm->store_list), row, text);
  new_store = sfm_store_create (disp, gimage, fg, row, readonly);
  fm->stores = g_slist_insert (fm->stores, new_store, row);
}


/*
 *
 */
static gint 
sfm_auto_save (GtkWidget *w, gpointer data)
{
  return 1;
}

static ToolType tool=-1;

static gint 
sfm_set_aofi (GtkWidget *w, gpointer data)
{

  GDisplay *disp;
      
  disp = (GDisplay*)data;

  if (tool == -1)
    {
     if (disp->select) 
      disp->select->hidden = FALSE;
      tool = active_tool->type;
      tools_select (RECT_SELECT);
      gtk_toggle_button_set_active (((GDisplay*)data)->bfm->sfm->aofi, TRUE);
    }
  else
    {
      tools_select (tool);

      tool = -1; 

      selection_invis (disp->select);
      selection_layer_invis (disp->select);
      disp->select->hidden = disp->select->hidden ? FALSE : TRUE;
      
      selection_start (disp->select, TRUE);

      gdisplays_flush ();
    }
  return 1; 
}

void
sfm_setting_aofi(GDisplay *disp)
{
  RectSelect * rect_sel;
  double scale;
  gint s_x, s_y, e_x, e_y;
  store_frame_manager *fm;

  if (tool == -1)
    return;

  if (active_tool->type == RECT_SELECT)
    {
      /* get position */
      rect_sel = (RectSelect *) active_tool->private;
      s_x = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
      s_y = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
      e_x = MAX (rect_sel->x, rect_sel->x + rect_sel->w) - s_x;
      e_y = MAX (rect_sel->y, rect_sel->y + rect_sel->h) - s_y;

      if ((!s_x && !s_y && !e_x && !e_y) ||
	  (!rect_sel->w && !rect_sel->h))
	{
	  scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));
	  s_x = 0;
	  s_y = 0;
	  e_x = disp->disp_width*scale;
	  e_y = disp->disp_height*scale;
	}
      fm = disp->bfm->sfm;

      fm->s_x = fm->sx = s_x;
      fm->s_y = fm->sy = s_y;
      fm->e_x = fm->ex = e_x;
      fm->e_y = fm->ey = e_y;
    }

}

static gint 
sfm_aofi (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*)data)->bfm->sfm;

  if (gtk_toggle_button_get_active (fm->aofi))
    {
      fm->s_x = fm->sx;
      fm->s_y = fm->sy;
      fm->e_x = fm->ex;
      fm->e_y = fm->ey;
    }
  else
    {
    sfm_unset_aofi ((GDisplay*)data);
    }
  return 1;
}

static void
sfm_unset_aofi (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  double scale;

  scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));
  fm->s_x = 0;
  fm->s_y = 0;
  fm->e_x = disp->disp_width*scale;
  fm->e_y = disp->disp_height*scale;
}

/*
 * ADD STORE
 */

static void
sfm_store_add_stores (GDisplay* disp)
{
  gint row, l, f, i;
  GImage *gimage; 
  char *whole, *raw, tmp[256], tmp2[256], *frame;
  store_frame_manager *fm = disp->bfm->sfm;
  gint num_to_add = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_add));
  store *item;

  row = fm->fg;
  item = (store*) g_slist_nth (fm->stores, row)->data;

  switch (TYPE_TO_ADD)
    {
    case 0:
      frame = strdup (bfm_get_frame (item->gimage));
      l = strlen (frame);
      f = atoi (frame);
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);
		      
      for (i=0; i<num_to_add; i++)
	{
	  sprintf (tmp, "%d\0", f-(i+1));
	  while (strlen (tmp) != l)
	    {
	      sprintf (tmp2, "0%s\0", tmp);
	      strcpy (tmp, tmp2);
	    }
	  bfm_this_filename (item->gimage, &whole, &raw, tmp);

	  /* find the current */
	  if ((gimage=file_load_without_display (whole, raw, disp)))
	    {
	      sfm_store_add_image (gimage, disp, row+(i+1), 0, fm->readonly);
	    }

	}
      break;
    case 1:
      frame = strdup (bfm_get_frame (item->gimage));
      l = strlen (frame);
      f = atoi (frame);
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);

      for (i=0; i<num_to_add; i++)
	{
	  sprintf (tmp, "%d\0", f+(i+1));
	  while (strlen (tmp) != l)
	    {
	      sprintf (tmp2, "0%s\0", tmp);
	      strcpy (tmp, tmp2);
	    }
	  bfm_this_filename (item->gimage, &whole, &raw, tmp);

	  /* find the current */
	  if ((gimage=file_load_without_display (whole, raw, disp)))
	    {
	      sfm_store_add_image (gimage, disp, row+i+1, 0, fm->readonly);
	    }

	}
      break;
    case 2:
      gimage = item->gimage;

      for (i=0; i<num_to_add; i++)
	{
	  sfm_store_add_image (gimage_copy (gimage), disp, row+i+1, 0, fm->readonly);
	}
      break;
    case 3:
      for (i=0; i<num_to_add; i++)
	{
	  sfm_store_add_image (NULL, disp, row+i+1, 0, 0);
	}

      break;
    }

}

static void
sfm_store_add_dialog_ok (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->add_dialog))
	{
	  gtk_widget_hide (fm->add_dialog);
	  sfm_store_add_stores ((GDisplay*) client_data);
	}
    }
}

static gint
sfm_store_add_dialog_delete (GtkWidget *w, GdkEvent *e, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;


  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->add_dialog))
	{
	  gtk_widget_hide (fm->add_dialog);
	}
    }
  return TRUE;
}

static void
sfm_store_add_readonly (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;
  fm->readonly = fm->readonly ? 0 : 1; 
}

static void
frame_manager_add_dialog_option (GtkWidget *w, gpointer client_data)
{
  TYPE_TO_ADD = (int) client_data;
}


static void
sfm_store_add_create_dialog (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  int i;
  GtkWidget *hbox, *radio_button, *label, *check_button;

  GSList *group = NULL;
  char *options[4] =
    {
      "Load prev frames",
      "Load next frames",
      "Load copies of cur frame",
      "Empty"
    };

  static ActionAreaItem offset_action_items[1] =
    {
	{ "Ok", sfm_store_add_dialog_ok, NULL, NULL },
    };

  if (!fm)
    return;

  if (!fm->add_dialog)
    {
      fm->add_dialog = gtk_dialog_new ();
      gtk_object_ref(GTK_OBJECT(fm->add_dialog));
      gtk_window_set_wmclass (GTK_WINDOW (fm->add_dialog), "Store Option", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (fm->add_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->add_dialog), "New Store Option");

      gtk_signal_connect (GTK_OBJECT (fm->add_dialog), "delete_event",
	  GTK_SIGNAL_FUNC (sfm_store_add_dialog_delete),
	  disp);

#if 0
      gtk_widget_set_uposition(fm->add_dialog, generic_window_x, generic_window_y);
      layout_connect_window_position(fm->add_dialog, &generic_window_x, &generic_window_y);
      minimize_register(fm->add_dialog);
#endif

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox),
	  hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
  
      /* readonly */
      check_button = gtk_check_button_new_with_label ("readonly");
      gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
	  (GtkSignalFunc) sfm_store_add_readonly,
	  disp);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox), check_button, FALSE, FALSE, 0);
      gtk_widget_show (check_button);

      /* num of store */
      label = gtk_label_new ("Add");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      disp->bfm->sfm->num_to_add = gtk_spin_button_new (gtk_adjustment_new (1, 1, 100, 1, 1, 0), 1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->num_to_add, FALSE, FALSE, 2);

      gtk_widget_show (disp->bfm->sfm->num_to_add);
      
      label = gtk_label_new ("stores");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);
      
      /* radio buttons */
      for (i = 0; i < 4; i++)
	{
	  radio_button = gtk_radio_button_new_with_label (group, options[i]);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
	  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
	      (GtkSignalFunc) frame_manager_add_dialog_option,
	      (void *)((long) i));
	  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox), radio_button, FALSE, FALSE, 0);
	  gtk_widget_show (radio_button);
	}

      offset_action_items[0].user_data = disp;
      build_action_area (GTK_DIALOG (fm->add_dialog), offset_action_items, 1, 0);

      gtk_widget_show (fm->add_dialog);
    }
  else
    {
    if (!GTK_WIDGET_VISIBLE (fm->add_dialog))
      {
	gtk_widget_show (fm->add_dialog);
      }
    }
}

