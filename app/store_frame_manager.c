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
#include "menus.h"

#include "tools/play_forward.xpm"
#include "tools/play_forward_is.xpm"
#include "tools/play_backwards.xpm"
#include "tools/play_backwards_is.xpm"
#include "tools/forward.xpm"
#include "tools/forward_is.xpm"
#include "tools/backwards.xpm"
#include "tools/backwards_is.xpm"
#include "tools/stop.xpm"
#include "tools/stop_is.xpm"

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

static store* sfm_store_create (GDisplay*, GImage*, int, int, int);

static void sfm_store_select (GtkCList *, gint, gint, GdkEventButton *, gpointer);
static void sfm_store_unselect (GtkCList *, gint, gint, GdkEventButton *, gpointer);
static void sfm_store_set_option (GtkCList *, gint, gpointer);

static void sfm_store_set_op (GDisplay *, gint, gint);

static void sfm_store_make_cur (GDisplay *, int);

static void sfm_store_add_create_dialog (GDisplay *disp);
static void sfm_store_change_frame_create_dialog (GDisplay *disp);

static gint sfm_adv_backwards (GtkWidget *, gpointer);
static gint sfm_adv_forward (GtkWidget *, gpointer);

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
  *utilbox, *slider, *menubar, *event_box, *label, *vbox;
  GtkAccelGroup *table;
  GtkTooltips *tooltip;

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
  OpsButton flip_button[] =
    {
	{ play_backwards_xpm, play_backwards_is_xpm, sfm_play_backwards, "Play Backwards", NULL, NULL, NULL, NULL, NULL, NULL },
	{ backwards_xpm, backwards_is_xpm, sfm_flip_backwards, "Flip Backward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ stop_xpm, stop_is_xpm, sfm_stop, "Stop", NULL, NULL, NULL, NULL, NULL, NULL },
	{ forward_xpm, forward_is_xpm, sfm_flip_forward, "Flip Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ play_forward_xpm, play_forward_is_xpm, sfm_play_forward, "Play Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, 0, 0, NULL, NULL, NULL, NULL }
    };

  gchar *store_col_name[6] = {"RO", "Adv", "F", "Bg", "*", "Filename"};
  
  /* the shell */
  disp->bfm->sfm->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (disp->bfm->sfm->shell), "sfm", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (disp->bfm->sfm->shell), FALSE, TRUE, FALSE);
  sprintf (tmp, "Store Frame Manager for %s", disp->gimage->filename);
  gtk_window_set_title (GTK_WINDOW (disp->bfm->sfm->shell), tmp);

  gtk_window_set_transient_for (GTK_WINDOW (disp->bfm->sfm->shell),
      GTK_WINDOW (disp->shell));

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->shell), "delete_event",
      GTK_SIGNAL_FUNC (sfm_delete_callback),
      disp);

  /* pull down menu */
  menus_get_sfm_store_menu (&menubar, &table, disp);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_reparent (menubar, GTK_WIDGET (GTK_DIALOG (disp->bfm->sfm->shell)->vbox));
  gtk_widget_show (menubar);

  gtk_window_add_accel_group (GTK_WINDOW (disp->shell), table);
 
  
  /* check buttons */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  disp->bfm->sfm->autosave = check_button = gtk_check_button_new_with_label (check_names[0]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[0],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      "Saves all stores not marked RO before removing them from list", NULL);

  check_button = gtk_check_button_new_with_label (check_names[1]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[1],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      "Sets the area of interest", NULL);

  disp->bfm->sfm->aofi = check_button = gtk_check_button_new_with_label (check_names[2]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[2],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      "Enable area of interest", NULL);
  
  /* advance */
  button = gtk_button_new_with_label ("<");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_adv_backwards,
      disp);
  gtk_widget_show (button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      "Loads a new frame (current frame number - step size) into each store marked A.", NULL);

  disp->bfm->sfm->num_to_adv = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 100, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->num_to_adv, FALSE, FALSE, 2);
  gtk_widget_show (disp->bfm->sfm->num_to_adv);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, disp->bfm->sfm->num_to_adv, 
      "Step size", NULL);
  
  button = gtk_button_new_with_label (">");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_adv_forward,
      disp);
  gtk_widget_show (button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      "Loads a new frame (current frame number + step size) into each store marked A.", NULL);
  
  /* store window */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);


  gtk_widget_set_usize (scrolled_window, 200, 150); 
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), scrolled_window, 
	TRUE, TRUE, 0);
  
  event_box = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_window), event_box);
  gtk_widget_show (event_box);

  disp->bfm->sfm->store_list = gtk_clist_new_with_titles( 6, store_col_name);

  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 0, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 1, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 2, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 3, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 4, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 5, GTK_JUSTIFY_LEFT);

  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 0);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 1);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 2);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 3);
  
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 0, 20);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 1, 30);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 2, 10);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 3, 20);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 4, 10);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 5, 
      strlen (prune_pathname (disp->gimage->filename))*10);
  
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "select_row",
      GTK_SIGNAL_FUNC(sfm_store_select),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "unselect_row",
      GTK_SIGNAL_FUNC(sfm_store_unselect),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "click-column",
      GTK_SIGNAL_FUNC(sfm_store_set_option),
      disp);

  gtk_clist_set_selection_mode (GTK_CLIST(disp->bfm->sfm->store_list), GTK_SELECTION_BROWSE); 

  /* It isn't necessary to shadow the border, but it looks nice :) */
  gtk_clist_set_shadow_type (GTK_CLIST(disp->bfm->sfm->store_list), GTK_SHADOW_OUT);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      "RO - read only. This store will not be saved\n
      A - advance. This store is affected by the advance controls\n
      F - flip. This store is affected by the flip book controls\n
      Bg - background. This is the src in a clone and the bg in the onionskin\n
      * - modified. This store has been modified", NULL);

  /* Add the CList widget to the vertical box and show it. */
  gtk_container_add(GTK_CONTAINER(event_box), disp->bfm->sfm->store_list);
  gtk_widget_show(disp->bfm->sfm->store_list);
  gtk_widget_show (scrolled_window);

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
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (disp->bfm->sfm->onionskin), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (disp->bfm->sfm->onionskin));
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, disp->bfm->sfm->onionskin, 
      "Checked when onionskining is on", NULL);
  
  
  utilbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), utilbox, TRUE, TRUE, 0);
  gtk_widget_show (utilbox);

  disp->bfm->sfm->onionskin_val = (GtkWidget*) gtk_adjustment_new (1.0, 0.0, 1.0, .1, .1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (disp->bfm->sfm->onionskin_val));
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (utilbox), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->onionskin_val), "value_changed",
      (GtkSignalFunc) sfm_onionskin,
      disp);
  gtk_widget_show (slider);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, slider, 
      "Sets the opacity for onion skinning.\n
      LMB to drag slider\n
      MMB to jump", NULL);

  button = gtk_button_new_with_label ("Fg/Bg");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_onionskin_fgbg,
      disp);
  gtk_widget_show (button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      "Flips between the fg and bg store", NULL);

  /* src & dest labels */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);
  
  label = gtk_label_new ("DEST DIR");
  gtk_box_pack_start(GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  label = gtk_label_new ("SRC DIR");
  gtk_box_pack_start(GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  scrolled_window =  gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
  gtk_box_pack_start(GTK_BOX (hbox), scrolled_window,
      TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);
  
  hbox = gtk_vbox_new (FALSE, 1);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_window), hbox);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_widget_show (hbox);
 
  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), event_box, FALSE, FALSE, 0);
  gtk_widget_show (event_box);
  
  
  disp->bfm->sfm->src_dir = gtk_label_new ("SRC DIR");
  gtk_label_set_justify (GTK_LABEL(disp->bfm->sfm->src_dir), GTK_JUSTIFY_LEFT);
  gtk_container_add (GTK_CONTAINER (event_box), 
      disp->bfm->sfm->src_dir);
  gtk_widget_show (disp->bfm->sfm->src_dir);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      "Src dir and file name", NULL);
  
  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), event_box, FALSE, FALSE, 0);
  gtk_widget_show (event_box);

  disp->bfm->sfm->dest_dir = gtk_label_new ("DEST DIR");
  gtk_label_set_justify (GTK_LABEL(disp->bfm->sfm->dest_dir), GTK_JUSTIFY_LEFT);
  gtk_container_add (GTK_CONTAINER(event_box), disp->bfm->sfm->dest_dir);
  gtk_widget_show (disp->bfm->sfm->dest_dir);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      "Dest dir and file name", NULL);


  /* add store */
  sfm_store_add_image (disp->gimage, disp, 0, 1, 0);
  sfm_unset_aofi (disp);

  
    gtk_widget_show (disp->bfm->sfm->shell); 
}


/*
 * FLIP BOOK CONTROLS
 */

static gint
sfm_backwards (store_frame_manager *fm)
{
  gint row = fm->fg - 1 < 0 ? g_slist_length (fm->stores)-1 : fm->fg - 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row - 1 < 0 ? g_slist_length (fm->stores) - 1: row - 1;
    }

  gtk_clist_select_row (GTK_CLIST(fm->store_list), row, 5);
  
  return TRUE;
}

static void 
sfm_play_backwards (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;

  bfm_onionskin_rm (gdisplay);

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 

  if (!fm->play)
    {
      fm->play = gtk_timeout_add (100, (GtkFunction) sfm_backwards, fm);
    }

}

static gint
sfm_forward (store_frame_manager *fm)
{
  gint row = fm->fg + 1 == g_slist_length (fm->stores) ? 0 : fm->fg + 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row + 1 == g_slist_length (fm->stores)  ? 0 : row + 1;
    }

  gtk_clist_select_row (GTK_CLIST(fm->store_list), row, 5);

  return TRUE;
}

static void 
sfm_play_forward (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;
  
  bfm_onionskin_rm (gdisplay);

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm;

  if (!fm->play)
    {
      fm->play = gtk_timeout_add (100, (GtkFunction) sfm_forward, fm);
    }
}

static void 
sfm_flip_backwards (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;
  
  bfm_onionskin_rm (gdisplay);

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  row = fm->fg - 1 < 0 ? g_slist_length (fm->stores)-1 : fm->fg - 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row - 1 < 0 ? g_slist_length (fm->stores) - 1: row - 1; 
    }

  gtk_clist_select_row (GTK_CLIST(fm->store_list), row, 5);
}

static void 
sfm_flip_forward (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm;
  gint row=0;
  
  bfm_onionskin_rm (gdisplay);

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  row = fm->fg + 1 == g_slist_length (fm->stores) ? 0 : fm->fg + 1;

  while (!((store*)g_slist_nth (fm->stores, row)->data)->flip && row != fm->fg)
    {
      row = row + 1 == g_slist_length (fm->stores)  ? 0 : row + 1; 
    }

  gtk_clist_select_row (GTK_CLIST(fm->store_list), row, 5);
}

static void 
sfm_stop (GtkWidget *w, gpointer data)
{
  GDisplay *gdisplay = (GDisplay*) data;
  store_frame_manager *fm = gdisplay->bfm->sfm;
  
  bfm_onionskin_rm (gdisplay);

  if (fm->play)
    {
      gtk_timeout_remove (fm->play);
      fm->play = 0;
    }

}

/*
 * ADVANCE
 */

static gint 
sfm_adv_backwards (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*)data)->bfm->sfm; 
  gint num_to_adv = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv));
  char whole[256], raw[256]; 
  gint row=0, cur_frame, new_frame, flag=0, frame;
  char filename[500], pathname[500];

  GImage *gimage=NULL;

  GSList *list=NULL, *l=NULL;
  store *item, *i;
  
  bfm_onionskin_rm ((GDisplay*)data);

  list = fm->stores;

  while (list)
    {
      flag = 0;
      item = (store*) list->data;

      cur_frame = atoi (bfm_get_frame (item->gimage->filename));
      new_frame = cur_frame - num_to_adv;
      if (item->advance && !item->special)
	{
	  l = fm->stores;
	  while (l && !flag)
	    {
	      i = (store*) l->data;
	      if (!i->special)
		{
		  frame = atoi (bfm_get_frame (i->gimage->filename));
		  if (frame == new_frame)
		    {
		      item->new_gimage = i->gimage;
		      i->remove = 0;
		      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, 
			  prune_filename (item->new_gimage->filename));
		      if (i->gimage->dirty)
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "*");
		      else
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		      flag = 1; 
		    }	 
		  l = g_slist_next (l);
		}
	    }
	}
      if (!flag && item->advance)
	{
	  /* load frame from disk */
	  if (fm->load_smart)
	    bfm_this_filename (((GDisplay*)data)->bfm->dest_dir,
		((GDisplay*)data)->bfm->dest_name,
		whole, raw, new_frame);
	  else
	    bfm_this_filename (((GDisplay*)data)->bfm->src_dir, 
		((GDisplay*)data)->bfm->src_name, 
		whole, raw, new_frame);
	  if (item->fg)
	    {
	      if (file_load (whole, raw, ((GDisplay*)data)))
		{
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		  ((GDisplay*)data)->ID = ((GDisplay*)data)->gimage->ID;
		  item->new_gimage = ((GDisplay*)data)->gimage;
		}
	      else
		if (fm->load_smart)
		  {
		    bfm_this_filename (((GDisplay*)data)->bfm->src_dir,
			((GDisplay*)data)->bfm->src_name,
			whole, raw, new_frame);
		    if (file_load (whole, raw, ((GDisplay*)data)))
		      {
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
			((GDisplay*)data)->ID = ((GDisplay*)data)->gimage->ID;
			item->new_gimage = ((GDisplay*)data)->gimage;
		      }
		    else
		      {
			item->new_gimage = item->gimage;
			item->remove = 0;
		      }		     
		  }
		else
		  {
		    item->new_gimage = item->gimage;
		    item->remove = 0;
		  }
	    }
	  else
	    {
	      if ((gimage = file_load_without_display (whole, raw, ((GDisplay*)data))))
		{
		  item->new_gimage = gimage;
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		}
	      else
		if (fm->load_smart)
		  {
		    bfm_this_filename (((GDisplay*)data)->bfm->src_dir,
			((GDisplay*)data)->bfm->src_name,
			whole, raw, new_frame);
		    if ((gimage = file_load_without_display (whole, raw, ((GDisplay*)data))))
		      {
			item->new_gimage = gimage;
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		      }
		    else
		      {
			item->new_gimage = item->gimage;
			item->remove = 0;
		      }
		  }
		else
		  {
		    item->new_gimage = item->gimage;
		    item->remove = 0;
		  }
	    }
	}
      list = g_slist_next (list);
      row ++;
    }

  /* save stores and delete images */
  list = fm->stores;
  while (list)
    {
      item = (store*) list->data;

      if (item->advance)
	{
	  if (!item->readonly && item->gimage->dirty && 
	      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->autosave)))
	    {
	      sprintf (filename, "%s.%s.%s", 
		  bfm_get_name (((GDisplay *)data)->bfm->dest_name),
		  bfm_get_frame (item->gimage->filename),
		  bfm_get_ext (((GDisplay *)data)->bfm->dest_name));
	      sprintf (pathname, "%s%s", ((GDisplay *)data)->bfm->dest_dir, filename);
	      file_save (item->gimage->ID, pathname, filename); 
	    }
	  if (item->remove)
	    {
	      gimage_delete (item->gimage);
	    }
	  item->gimage = item->new_gimage;
	  if (item->fg)
	    sfm_store_make_cur ((GDisplay*)data, fm->fg); 
	  item->new_gimage = NULL;
	  item->remove = 1; 
	}
      list = g_slist_next (list); 
    }

 
  return 1;
}


static gint 
sfm_adv_forward (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*)data)->bfm->sfm; 
  gint num_to_adv = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv));
  char whole[256], raw[256]; 
  gint row=0, cur_frame, new_frame, flag=0, frame;
  char filename[500], pathname[500];

  GImage *gimage=NULL;

  GSList *list=NULL, *l=NULL;
  store *item, *i;

  bfm_onionskin_rm ((GDisplay*)data);

  list = fm->stores;

  while (list)
    {
      flag = 0;
      item = (store*) list->data;

      cur_frame = atoi (bfm_get_frame (item->gimage->filename));
      new_frame = cur_frame + num_to_adv;
      if (item->advance && !item->special)
	{
	  l = fm->stores;
	  while (l && !flag)
	    {
	      i = (store*) l->data;
	      if (!i->special)
		{
		  frame = atoi (bfm_get_frame (i->gimage->filename));
		  if (frame == new_frame)
		    {
		      item->new_gimage = i->gimage;
		      i->remove = 0;
		      gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, 
			  prune_filename (item->new_gimage->filename));
		      if (i->gimage->dirty)
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "*");
		      else
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		      flag = 1; 
		    }	 
		  l = g_slist_next (l);
		}
	    }
	}
      if (!flag && item->advance)
	{
	  /* load frame from disk */
	  if (fm->load_smart)
	    bfm_this_filename (((GDisplay*)data)->bfm->dest_dir,
		((GDisplay*)data)->bfm->dest_name,
		whole, raw, new_frame);
	  else
	    bfm_this_filename (((GDisplay*)data)->bfm->src_dir, 
		((GDisplay*)data)->bfm->src_name, 
		whole, raw, new_frame);
	  if (item->fg)
	    {
	      if (file_load (whole, raw, ((GDisplay*)data)))
		{
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 5, raw);
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		  ((GDisplay*)data)->ID = ((GDisplay*)data)->gimage->ID;
		  item->new_gimage = ((GDisplay*)data)->gimage;
		}
	      else
		if (fm->load_smart)
		  {
		    bfm_this_filename (((GDisplay*)data)->bfm->src_dir,
			((GDisplay*)data)->bfm->src_name,
			whole, raw, new_frame);
		    if (file_load (whole, raw, ((GDisplay*)data)))
		      {
			gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 5, raw);
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
			((GDisplay*)data)->ID = ((GDisplay*)data)->gimage->ID;
			item->new_gimage = ((GDisplay*)data)->gimage;
		      }
		    else
		      {
			item->new_gimage = item->gimage;
			item->remove = 0;
		      }		     
		  }
		else
		  {
		    item->new_gimage = item->gimage;
		    item->remove = 0;
		  }
	    }
	  else
	    {
	      if ((gimage = file_load_without_display (whole, raw, ((GDisplay*)data))))
		{
		  item->new_gimage = gimage;
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
		  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		}
	      else
		if (fm->load_smart)
		  {
		    bfm_this_filename (((GDisplay*)data)->bfm->src_dir,
			((GDisplay*)data)->bfm->src_name,
			whole, raw, new_frame);
		    if ((gimage = file_load_without_display (whole, raw, ((GDisplay*)data))))
		      {
			item->new_gimage = gimage;
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 5, raw);
			gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 4, "");
		      }
		    else
		      {
			item->new_gimage = item->gimage;
			item->remove = 0;
		      }
		  }
		else
		  {
		    item->new_gimage = item->gimage;
		    item->remove = 0;
		  }
	    }
	}
      list = g_slist_next (list);
      row ++;
    }

  /* save stores and delete images */
  list = fm->stores;
  while (list)
    {
      item = (store*) list->data;

      if (item->advance)
	{
	  if (!item->readonly && item->gimage->dirty && 
	      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->autosave)))
	    {
	      sprintf (filename, "%s.%s.%s", 
		  bfm_get_name (((GDisplay *)data)->bfm->dest_name),
		  bfm_get_frame (item->gimage->filename),
		  bfm_get_ext (((GDisplay *)data)->bfm->dest_name));
	      sprintf (pathname, "%s%s", ((GDisplay *)data)->bfm->dest_dir, filename);
	      file_save (item->gimage->ID, pathname, filename); 
	    }
	  if (item->remove)
	    {
	      gimage_delete (item->gimage);
	    }
	  item->gimage = item->new_gimage;
	  if (item->fg)
	    sfm_store_make_cur ((GDisplay*)data, fm->fg); 
	  item->new_gimage = NULL;
	  item->remove = 1; 
	}
      list = g_slist_next (list); 
    }


  return 1;
}

/*
 * ONION SKIN
 */
      
static char onionskin_off = 1;
static gint 
sfm_onionskin (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;

  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      printf ("ERROR: you must select a bg\n");
      if (((GtkAdjustment*)fm->onionskin_val)->value != 1)
	gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_val), 1);
      return 1;
    }
  
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin)) &&
      onionskin_off) 
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin), TRUE); 

  bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_val)->value,
      fm->s_x, fm->s_y, fm->e_x, fm->e_y); 

  return 1;
}

static gint 
sfm_onionskin_on (GtkWidget *w, gpointer data)
{

  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;

  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      printf ("ERROR: you must select a bg\n");
      return 1;
    }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin)))
    {
      onionskin_off = 1;
      /* rm onionskin */
      if (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_val)->value != 1)
	{
	  gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_val), 1);
	}
	bfm_onionskin_rm ((GDisplay*)data); 
    }
  else
    {
      onionskin_off = 0;
      /* set onionskin */
	  bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_val)->value,
	      fm->s_x, fm->s_y, fm->e_x, fm->e_y);
    }
  return 1;
}

static gint 
sfm_onionskin_fgbg (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;
  
  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      printf ("ERROR: you must select a bg\n");
      return 1;
    }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin)))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin), TRUE); 

      bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_val)->value,
	  fm->s_x, fm->s_y, fm->e_x, fm->e_y); 
    }  

  if (((GtkAdjustment*)((GDisplay*)data)->bfm->sfm->onionskin_val)->value)
    gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_val), 0);
  else
    gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_val), 1);

  return 1;
}

void 
sfm_onionskin_set_offset (GDisplay* disp, int x, int y)
{
  layer_translate2 (disp->bfm->bg->active_layer, -x, -y,
                    disp->bfm->sfm->s_x, disp->bfm->sfm->s_y, 
		    disp->bfm->sfm->e_x, disp->bfm->sfm->e_y);
}

void 
sfm_onionskin_rm (GDisplay* disp)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin), FALSE); 
}

/*
 * STORE
 */
static void 
sfm_store_clear (GDisplay* disp)
{
}

void 
sfm_store_add (GtkWidget *w, gpointer data)
{
  bfm_onionskin_rm ((GDisplay*)data);
 sfm_store_add_create_dialog ((GDisplay *)data); 
}

static void
sfm_store_remove (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg));
  gint new_fg, i;

  gint l = g_slist_length (fm->stores);
  bfm_onionskin_rm (disp);

  if (item->gimage->dirty)
    {
    }

  new_fg = fm->fg < l-1 ? fm->fg : fm->fg - 1;

  if (fm->fg == fm->bg)
    {
      fm->bg = new_fg;
      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->bg, 3, "Bg"); 
    }
    
  
  gtk_clist_remove (GTK_CLIST(fm->store_list), fm->fg);
  fm->stores = g_slist_remove (fm->stores, (g_slist_nth (fm->stores, fm->fg))->data);
  gtk_clist_select_row (GTK_CLIST(fm->store_list), new_fg, 5);

  for (i=0; i<l-1; i++)
    {
      if (((store*) (g_slist_nth (fm->stores, i)))->bg)
	fm->bg = i;
    } 
  
}

void 
sfm_store_delete (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  gint l = g_slist_length (fm->stores);
  bfm_onionskin_rm ((GDisplay*)data);

  if (l == 1)
    {
      printf ("ERROR: cannot rm last store\n");
      return;
    }
  
   sfm_store_remove ((GDisplay *)data); 
 
}

void 
sfm_store_raise (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  store *item;
  bfm_onionskin_rm ((GDisplay*)data);

  if (!fm->fg)
    return;

  gtk_clist_swap_rows (GTK_CLIST(fm->store_list), fm->fg-1, fm->fg);

  item = (store*) g_slist_nth (fm->stores, fm->fg)->data;
  fm->stores = g_slist_remove (fm->stores, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg-1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg - 1 : fm->bg == fm->fg - 1 ? fm->fg : fm->bg;
  fm->fg --;
  
}

void 
sfm_store_lower (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  store *item;

  gint l = g_slist_length (fm->stores);
  bfm_onionskin_rm ((GDisplay*)data);
  
  if (fm->fg == l-1)
    return;

  
  gtk_clist_swap_rows (GTK_CLIST(fm->store_list), fm->fg+1, fm->fg);

  item = (store*) g_slist_nth (fm->stores, fm->fg)->data;
  fm->stores = g_slist_remove (fm->stores, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg+1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg + 1 : fm->bg == fm->fg + 1 ? fm->fg : fm->bg;
  fm->fg ++;
}

void 
sfm_store_save (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  char filename[500], pathname[500];

  store *item = (store*) g_slist_nth (fm->stores, fm->fg)->data;
  bfm_onionskin_rm ((GDisplay*)data);

  if (item->readonly)
    {
      printf ("ERROR: trying to save a readonly file \n");
      return;
    }

  sprintf (filename, "%s.%s.%s", 
      bfm_get_name (((GDisplay *)data)->bfm->dest_name),
      bfm_get_frame (item->gimage->filename),
      bfm_get_ext (((GDisplay *)data)->bfm->dest_name));
  sprintf (pathname, "%s%s", ((GDisplay *)data)->bfm->dest_dir, filename);
  file_save (item->gimage->ID, pathname, filename); 
}

void 
sfm_store_save_all (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  store *item;
  gint i, l = g_slist_length (fm->stores);
  char filename[500], pathname[500];
  bfm_onionskin_rm ((GDisplay*)data);

  for (i=0; i<l; i++)
    {
      item = (store*) (g_slist_nth (fm->stores, i))->data;
      if (!item->readonly)
	{
	  sprintf (filename, "%s.%s.%s", 
	      bfm_get_name (((GDisplay *)data)->bfm->dest_name),
	      bfm_get_frame (item->gimage->filename),
	      bfm_get_ext (((GDisplay *)data)->bfm->dest_name));
	  sprintf (pathname, "%s%s", ((GDisplay *)data)->bfm->dest_dir, filename);
	  file_save (item->gimage->ID, pathname, filename); 
	}
    }
}

void 
sfm_store_revert (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg))->data;

  if (file_load (item->gimage->filename,
	prune_filename (item->gimage->filename), (GDisplay *)data))
    {
      ((GDisplay *)data)->ID = ((GDisplay *)data)->gimage->ID;
      item->gimage = ((GDisplay *)data)->gimage;
      sfm_store_make_cur (((GDisplay *)data), fm->fg);

      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 5, prune_filename (item->gimage->filename));
      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 4, "");
    }
}

void
sfm_store_change_frame (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm;
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg))->data;
  char filename[500], pathname[500];
  bfm_onionskin_rm ((GDisplay*)data);


  if (item->gimage->dirty && !item->readonly)
    {
     
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->autosave)))
	{
	  sprintf (filename, "%s.%s.%s", 
	      bfm_get_name (((GDisplay *)data)->bfm->dest_name),
	      bfm_get_frame (item->gimage->filename),
	      bfm_get_ext (((GDisplay *)data)->bfm->dest_name));
	  sprintf (pathname, "%s%s", ((GDisplay *)data)->bfm->dest_dir, filename);
	  file_save (item->gimage->ID, pathname, filename); 
	}
      else
	printf ("WARNING: you just lost your changes to the file\n"); 
    }


  sfm_store_change_frame_create_dialog (((GDisplay *)data));

}

void 
sfm_set_dir_src (GDisplay *disp)
{
  char tmp[500];
  store_frame_manager *fm = disp->bfm->sfm;
  
  sprintf (tmp, "%s%s", disp->bfm->src_dir, bfm_get_name (disp->bfm->src_name));
  gtk_label_set_text(GTK_LABEL (fm->src_dir), tmp);
}

void 
sfm_store_recent_src (GtkWidget *w, gpointer data)
{
}

void 
sfm_set_dir_dest (GDisplay *disp)
{
  char tmp[500];
  store_frame_manager *fm = disp->bfm->sfm;

  sprintf (tmp, "%s%s", disp->bfm->dest_dir, bfm_get_name (disp->bfm->dest_name));
  gtk_label_set_text(GTK_LABEL (fm->dest_dir),tmp); 
}

void 
sfm_store_recent_dest (GtkWidget *w, gpointer data)
{
  bfm_onionskin_rm ((GDisplay*)data);
}

void 
sfm_store_load_smart (GtkWidget *w, gpointer data)
{
  ((GDisplay *)data)->bfm->sfm->load_smart = ((GDisplay *)data)->bfm->sfm->load_smart ? 0 : 1; 
  bfm_onionskin_rm ((GDisplay*)data);
  
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
  new_store->special = 0;
  new_store->remove = 1;


  if (readonly)
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, "RO");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, ""); 
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 1, "A");
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 2, "F");
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 3, "");
  if (img->dirty)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "");
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 5, prune_filename (img->filename));

  if (active)
    gtk_clist_select_row (GTK_CLIST (fm->store_list), num, 5);

  if (!g_slist_length (fm->stores))
    {
      bfm_set_dir_src (disp, img->filename);
      bfm_set_dir_dest (disp, img->filename);

      sfm_set_dir_src (disp);
      sfm_set_dir_dest (disp);
    }
  return new_store;
}

static void
sfm_store_make_cur (GDisplay *gdisplay, int row)
{
  store *item;
  store_frame_manager *fm;
  char tmp[256];

  if (row == -1)
    return;

  fm = gdisplay->bfm->sfm;
  fm->fg = row;

  if (g_slist_length (fm->stores) <= row)
    return;

  item = (store*)(g_slist_nth (fm->stores, row)->data);

  item->fg = 1;

  /* display the new store */
  gdisplay->gimage = item->gimage;
  gdisplay->ID = item->gimage->ID;
  gdisplays_update_title (gdisplay->gimage->ID);
  gdisplay_add_update_area (gdisplay, fm->s_x, fm->s_y, fm->e_x, fm->e_y);
#if 0
  if (active_tool->type == CLONE)
    clone_flip_image ();
#endif
  bfm_set_fg (gdisplay, 
      ((store*)(g_slist_nth (fm->stores, fm->fg)->data))->gimage); 

  sprintf (tmp, "Store Frame Manager for %s", gdisplay->gimage->filename);
  gtk_window_set_title (GTK_WINDOW (gdisplay->bfm->sfm->shell), tmp);
  
  gdisplays_flush ();
}

static void 
sfm_store_select (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  bfm_onionskin_rm (gdisplay);
  
  if (!gdisplay || row == -1)
    return;

  if (col != 5 && col != -1)
    {
      sfm_store_set_op (gdisplay, row, col); 
      gtk_clist_select_row (GTK_CLIST (gdisplay->bfm->sfm->store_list), 
	  gdisplay->bfm->sfm->fg,
	  5);

      return; 
    }
  sfm_store_make_cur (gdisplay, row);

}

static void 
sfm_store_unselect (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{

  GDisplay *gdisplay = (GDisplay*) client_pointer;
  store_frame_manager *fm;
  bfm_onionskin_rm (gdisplay);
 
  if (!gdisplay)
    return;
  
  fm = gdisplay->bfm->sfm;

 /* 
  gtk_clist_select_row (GTK_CLIST(fm->store_list), fm->fg, 0); 
*/
  if (col == 5)
    {
  ((store*)g_slist_nth (fm->stores, fm->fg)->data)->fg = 0;
  fm->fg = -1;
    }
}

void
sfm_dirty (GDisplay* disp)
{

  if (disp->gimage->dirty)
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 4, "");

}

GDisplay *
sfm_load_image_into_fm (GDisplay* disp, GImage *img)
{

  store_frame_manager *fm = disp->bfm->sfm; 
  char *text[6]={NULL, NULL, NULL, NULL, NULL, NULL}; 
  store *new_store;

  gtk_clist_insert (GTK_CLIST (fm->store_list), fm->fg+1, text);
  new_store = sfm_store_create (disp, img, 1, fm->fg+1, 1);
  new_store->special = 1;
  fm->stores = g_slist_insert (fm->stores, new_store, fm->fg+1);
  return disp;
}

static void 
sfm_store_set_op (GDisplay *gdisplay, gint row, gint col)
{
  store_frame_manager *fm;
  store *item;
  
  fm = gdisplay->bfm->sfm;

  if(-1 == row)
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
      if (fm->bg != -1 && fm->bg != row /*fm->fg*/)
	{
	  ((store*) g_slist_nth (fm->stores, fm->bg)->data)->bg = 0;
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->bg, 3, "");
	  fm->bg = -1;
	  bfm_set_bg (gdisplay, 0); 
	}
      item->bg = item->bg ? 0 : 1; 
      if (item->bg)
	{
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 3, "Bg"); 
	  fm->bg = row;
      
	  bfm_set_bg (gdisplay, 
	      ((store*)(g_slist_nth (fm->stores, fm->bg)->data))->gimage); 
	  
	}
      else
	{
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), row, 3, "");  
	  fm->bg = -1;
	  bfm_set_bg (gdisplay, 0); 
	}
      break;
    default:
      break;
      
    }
}

static void 
sfm_store_set_option (GtkCList *w, gint col, gpointer client_pointer)
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;

  bfm_onionskin_rm (gdisplay);

  if (!gdisplay)
    return;

  sfm_store_set_op (gdisplay, gdisplay->bfm->sfm->fg, col);

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
  bfm_onionskin_rm ((GDisplay*)data);
  return 1;
}

static ToolType tool=-1;

static gint 
sfm_set_aofi (GtkWidget *w, gpointer data)
{

  GDisplay *disp;
      
  disp = (GDisplay*)data;
  bfm_onionskin_rm (disp);

  if (tool == -1)
    {
     if (disp->select) 
      disp->select->hidden = FALSE;
      tool = active_tool->type;
      tools_select (RECT_SELECT);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->aofi), TRUE);
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
  bfm_onionskin_rm (disp);

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
  bfm_onionskin_rm ((GDisplay*)data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->aofi)))
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
  bfm_onionskin_rm (disp);

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
  char *whole, *raw, *frame;
  store_frame_manager *fm = disp->bfm->sfm;
  gint num_to_add = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_add));
  store *item;

  row = fm->fg;
  item = (store*) g_slist_nth (fm->stores, row)->data;

  switch (TYPE_TO_ADD)
    {
    case 1:
      frame = strdup (bfm_get_frame (item->gimage->filename));
      l = strlen (frame);
      f = atoi (frame);
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);

      for (i=0; i<num_to_add; i++)
	{

	  if (fm->load_smart)
	    bfm_this_filename (disp->bfm->dest_dir, disp->bfm->dest_name, whole, raw, f-(i+1));
	  else 
	    bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw, f-(i+1));

	  /* find the current */
	  if ((gimage=file_load_without_display (whole, raw, disp)))
	    {
	      sfm_store_add_image (gimage, disp, row+(i+1), 0, fm->readonly);
	    }
	  else
	    if (fm->load_smart)
	      {
		bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw, f-(i+1));
		if ((gimage=file_load_without_display (whole, raw, disp)))
		  {
		    sfm_store_add_image (gimage, disp, row+(i+1), 0, fm->readonly);
		  }
	      }
	}
      break;
    case 0:
      frame = strdup (bfm_get_frame (item->gimage->filename));
      l = strlen (frame);
      f = atoi (frame);
      whole = (char*) malloc (sizeof(char)*255);
      raw = (char*) malloc (sizeof(char)*255);

      for (i=0; i<num_to_add; i++)
	{
	  if (fm->load_smart)
	    bfm_this_filename (disp->bfm->dest_dir, disp->bfm->dest_name, whole, raw, f+(i+1));
	  else 
	    bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw, f+(i+1));

	  /* find the current */
	  if ((gimage=file_load_without_display (whole, raw, disp)))
	    {
	      sfm_store_add_image (gimage, disp, row+(i+1), 0, fm->readonly);
	    }
	  else
	    if (fm->load_smart)
	      {
		bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw, f+(i+1));
		if ((gimage=file_load_without_display (whole, raw, disp)))
		  {
		    sfm_store_add_image (gimage, disp, row+(i+1), 0, fm->readonly);
		  }
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
  char *options[3] =
    {
      "Load next frames",
      "Load prev frames",
      "Load copies of cur frame"
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
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      disp->bfm->sfm->num_to_add = gtk_spin_button_new (
	  GTK_ADJUSTMENT(gtk_adjustment_new (1, 1, 100, 1, 1, 0)), 1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->num_to_add, FALSE, FALSE, 2);

      gtk_widget_show (disp->bfm->sfm->num_to_add);
      
      label = gtk_label_new ("stores");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);
      
      /* radio buttons */
      for (i = 0; i < 3; i++)
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

/*
 * CHANGE FRAME NUMBER 
 */

static void
sfm_store_change_frame_to (GDisplay* disp)
{

  char whole[500], raw[500];

  store_frame_manager *fm = disp->bfm->sfm;
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg)->data);

  if (fm->load_smart) 
    bfm_this_filename (disp->bfm->dest_dir, disp->bfm->dest_name, whole, raw,
	atoi (gtk_editable_get_chars (GTK_EDITABLE(fm->change_to), 0, -1))); 
  else
    bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw,
	atoi (gtk_editable_get_chars (GTK_EDITABLE(fm->change_to), 0, -1)));
  if (file_load (whole, raw, disp))
    {
      disp->ID = disp->gimage->ID;
      item->gimage = disp->gimage;
      sfm_store_make_cur (disp, fm->fg);

      gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 
	  5, raw);

    }
  else
    if (fm->load_smart)
      {
	bfm_this_filename (disp->bfm->src_dir, disp->bfm->src_name, whole, raw,
	    atoi (gtk_editable_get_chars (GTK_EDITABLE(fm->change_to), 0, -1)));
	if (file_load (whole, raw, disp))
	  {
	    disp->ID = disp->gimage->ID;
	    item->gimage = disp->gimage;
	    sfm_store_make_cur (disp, fm->fg);

	    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg,
		5, raw);

	  }

     } 
  
}

static void
sfm_store_change_frame_dialog_ok (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
	{
	  gtk_widget_hide (fm->chg_frame_dialog);
	  sfm_store_change_frame_to ((GDisplay*) client_data);
	}
    }
}

static gint
sfm_store_change_frame_dialog_delete (GtkWidget *w, GdkEvent *e, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;


  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
	{
	  gtk_widget_hide (fm->chg_frame_dialog);
	}
    }
  return TRUE;
}

static void
sfm_store_change_frame_create_dialog (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  GtkWidget *hbox, *label;
  char tmp[500], *temp;
  store *item = (store*) (g_slist_nth (fm->stores, fm->fg)->data);

  static ActionAreaItem offset_action_items[1] =
    {
	{ "Ok", sfm_store_change_frame_dialog_ok, NULL, NULL },
    };

  if (!fm)
    return;

  if (!fm->chg_frame_dialog)
    {
      fm->chg_frame_dialog = gtk_dialog_new ();
      gtk_object_ref(GTK_OBJECT(fm->chg_frame_dialog));
      gtk_window_set_wmclass (GTK_WINDOW (fm->chg_frame_dialog), "Store Option", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (fm->chg_frame_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->chg_frame_dialog), "New Store Option");

      gtk_signal_connect (GTK_OBJECT (fm->chg_frame_dialog), "delete_event",
	  GTK_SIGNAL_FUNC (sfm_store_change_frame_dialog_delete),
	  disp);

#if 0
      gtk_widget_set_uposition(fm->add_dialog, generic_window_x, generic_window_y);
      layout_connect_window_position(fm->add_dialog, &generic_window_x, &generic_window_y);
      minimize_register(fm->add_dialog);
#endif

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->chg_frame_dialog)->vbox),
	  hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
  

      sprintf (&(tmp[0]), "%s.", bfm_get_name (item->gimage->filename));
      label = gtk_label_new (tmp);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      fm->change_to = gtk_entry_new ();
      temp = strdup (bfm_get_frame (item->gimage->filename));
      gtk_entry_set_text (GTK_ENTRY (fm->change_to), temp);
      gtk_box_pack_start (GTK_BOX (hbox), fm->change_to, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (fm->change_to), "activate",
	  (GtkSignalFunc) sfm_store_change_frame_dialog_ok,
	  disp);
      gtk_widget_show (fm->change_to);

      sprintf (&(tmp[0]), ".%s", bfm_get_ext (item->gimage->filename));
      label = gtk_label_new (tmp);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      offset_action_items[0].user_data = disp;
      build_action_area (GTK_DIALOG (fm->chg_frame_dialog), offset_action_items, 1, 0);

      gtk_widget_show (fm->chg_frame_dialog);
    }
  else
    {
    if (!GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
      {
	gtk_widget_show (fm->chg_frame_dialog);
      }
    }
}


