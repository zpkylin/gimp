#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "hu.h"
#include "util.h"
#include <txt.h>
#include "parsley.h"

extern "C" 
{
extern void gimp_extension_process (guint timeout);
extern void gimp_extension_ack     (void);
}

typedef enum
{
  PARSLEY_IMAGE = 0,
  PARSLEY_DRAWABLE,
  PARSLEY_LAYER,
  PARSLEY_CHANNEL,
  PARSLEY_COLOR,
  PARSLEY_TOGGLE,
  PARSLEY_VALUE
} ParsleyArgType;

typedef struct
{
  gfloat    color[3];
  gfloat   old_color[3];
}ParsleyColor;

typedef union 
{
  gint32      pa_image;
  gint32      pa_drawable;
  gint32      pa_layer;
  gint32      pa_channel;
  ParsleyColor     pa_color;
  gint32      pa_toggle;
  gchar *     pa_value;
}ParsleyArgValue;

typedef struct
{
  gchar *      script_name;
  gchar *      description;
  gchar *      help;
  gchar *      author;
  gchar *      copyright;
  gchar *      date;
  gchar *      img_types;
  gint         num_args;
  ParsleyArgType *  arg_types;
  gchar **     arg_labels;
  ParsleyArgValue * arg_defaults;
  ParsleyArgValue * arg_values;
  gint32       image_based;
}ParsleyScript;

static struct stat filestat;
static GList *script_list = NULL;

/* External functions
 */


/* Declare local functions.
 */

static void quit (void);
static void init (void);
static void query  (void);
static void run    (char      *name,
		         int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static Twine 
gimp_register_call (
				List <Twine>& argv, 
				HU_ParserState& state
				);
static Twine 
gimp_proc_db_call (
				List <Twine>& argv, 
				HU_ParserState& state
				);
static void
parsley_script_proc (char     *name,
		       int       nparams,
		       GParam   *params,
		       int      *nreturn_vals,
		       GParam  **return_vals);

static void parsley_add_scripts (void);
static ParsleyScript * parsley_find_script (gchar *script_name);
static void parsley_free_script (ParsleyScript *script);
static void parsley_add_gimp_procedures (void);
static void parsley_add_gimp_constants (void);
static void convert_pdb_to_parsley_string (char *str, char * parsley_str);
static void convert_parsley_to_pdb_string (char *parsley_str, char * str);

GPlugInInfo PLUG_IN_INFO =
{
  init,         /* init_proc */
  quit,         /* quit_proc */
  query,        /* query_proc */
  run,          /* run_proc */
};

int main (int argc, char *argv[]) 
{ 
  return gimp_main (argc, argv); 
}

static void
init (void)
{
#ifdef DEBUG
printf("in parsley init\n");
#endif
}

static void
quit (void)
{
#ifdef DEBUG
printf("in parsley quit\n");
#endif
}

static void
query (void)
{
#ifdef DEBUG
printf("in parsley query\n");
#endif

  gimp_install_procedure ("parsley_extension",
			  "A parsley extension for gimp",
			  "",
			  "Calvin Williamson",
			  "Calvin Williamson",
			  "1999",
			  NULL,
			  NULL,
			  PROC_EXTENSION,
			  0, 0, NULL, NULL);
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  char * argv[] = {"parsley_extension"};

#ifdef DEBUG
  printf("in parsley run\n");
#endif

  /* Dont write any audit files or display parsley banner */  
  putenv ("noaudit=");
  putenv ("nobanner=");

  /* add a function so parsley scripts can register with the pdb */ 
  HU_AddFunction ("gimpRegister", gimp_register_call, "Registers a parsley script with the pdb");

  /* 
     Make the gimp pdb routines available from parsley, using a 
     marshall function to route the routines to the pdb 
  */ 

  parsley_add_gimp_procedures ();
  parsley_add_gimp_constants ();

  /* What should I pass here...dont know? */ 
  HU_Init (1, argv); 
 

  /* basically the next sources all the parsley scripts lying around 
     (eg in the gimps parsley scripts directory) 
     defining all those routines, and registering with the pdb 
  */

  parsley_add_scripts ();

  /*  Acknowledge that the extension is properly initialized  */
  gimp_extension_ack ();

  while (1)   /* send any commands we receive to gimp */
    gimp_extension_process (0);
}

static void
parsley_add_gimp_procedures ()
{
  char **proc_list;
  char *proc_name;
  char *proc_name_parsley;
  char *arg_name;
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int nparams;
  int nreturn_vals;
  GParamDef *params;
  GParamDef *return_vals;
  int num_procs;
  int i;

/*
  gimp_query_database (char   *name_regexp,
		     char   *blurb_regexp,
		     char   *help_regexp,
		     char   *author_regexp,
		     char   *copyright_regexp,
		     char   *date_regexp,
		     char   *proc_type_regexp,
		     int    *nprocs,
		     char ***proc_names)
*/

  gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*", &num_procs, &proc_list);

  /*  register each procedure as a parsley func  */
  for (i = 0; i < num_procs; i++)
    {
      proc_name = g_strdup (proc_list[i]);
      proc_name_parsley = g_strdup (proc_list[i]);

      /*  lookup the procedure  */
      if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
				&proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
				&params, &return_vals) == TRUE)
	{
	  /*convert the names to parsley-like naming conventions  */

	  convert_pdb_to_parsley_string (proc_name, proc_name_parsley);
	  /*printf ("old procedure %s, new procedure %s\n", proc_name, proc_name_parsley);*/

	  /* add the function to parsley, so that it calls gimp_proc_db_call
	     when it must execute this function. */

	  HU_AddFunction (proc_name_parsley, gimp_proc_db_call, proc_help);
	  
	  /*  free the queried information  */
	  g_free (proc_blurb);
	  g_free (proc_help);
	  g_free (proc_author);
	  g_free (proc_copyright);
	  g_free (proc_date);
	  g_free (params);
	  g_free (return_vals);
	}
	g_free (proc_name);
	g_free (proc_name_parsley);
    }

  g_free (proc_list);
}

static void
parsley_add_gimp_constants ()
{
  HU_SetVariable("PARSLEY_IMAGE", "0"); 
  HU_SetVariable("PARSLEY_DRAWABLE", "1"); 
  HU_SetVariable("PARSLEY_LAYER", "2"); 
  HU_SetVariable("PARSLEY_CHANNEL", "3"); 
  HU_SetVariable("PARSLEY_COLOR", "4"); 
  HU_SetVariable("PARSLEY_TOGGLE", "5"); 
  HU_SetVariable("PARSLEY_VALUE", "6"); 
  HU_SetVariable("noaudit", "1"); 

#if 0
  gparam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_data_dir",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-data-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_plugin_dir",
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-plugin-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);

  setvar (cintern ("NORMAL"), flocons (0), NIL);
  setvar (cintern ("DISSOLVE"), flocons (1), NIL);
  setvar (cintern ("BEHIND"), flocons (2), NIL);
  setvar (cintern ("MULTIPLY"), flocons (3), NIL);
  setvar (cintern ("SCREEN"), flocons (4), NIL);
  setvar (cintern ("OVERLAY"), flocons (5), NIL);
  setvar (cintern ("DIFFERENCE"), flocons (6), NIL);
  setvar (cintern ("ADDITION"), flocons (7), NIL);
  setvar (cintern ("SUBTRACT"), flocons (8), NIL);
  setvar (cintern ("DARKEN-ONLY"), flocons (9), NIL);
  setvar (cintern ("LIGHTEN-ONLY"), flocons (10), NIL);
  setvar (cintern ("HUE"), flocons (11), NIL);
  setvar (cintern ("SATURATION"), flocons (12), NIL);
  setvar (cintern ("COLOR"), flocons (13), NIL);
  setvar (cintern ("VALUE"), flocons (14), NIL);

  setvar (cintern ("FG-BG-RGB"), flocons (0), NIL);
  setvar (cintern ("FG-BG-HSV"), flocons (1), NIL);
  setvar (cintern ("FG-TRANS"), flocons (2), NIL);
  setvar (cintern ("CUSTOM"), flocons (3), NIL);

  setvar (cintern ("LINEAR"), flocons (0), NIL);
  setvar (cintern ("BILINEAR"), flocons (1), NIL);
  setvar (cintern ("RADIAL"), flocons (2), NIL);
  setvar (cintern ("SQUARE"), flocons (3), NIL);
  setvar (cintern ("CONICAL-SYMMETRIC"), flocons (4), NIL);
  setvar (cintern ("CONICAL-ASYMMETRIC"), flocons (5), NIL);
  setvar (cintern ("SHAPEBURST-ANGULAR"), flocons (6), NIL);
  setvar (cintern ("SHAPEBURST-SPHERICAL"), flocons (7), NIL);
  setvar (cintern ("SHAPEBURST-DIMPLED"), flocons (8), NIL);

  setvar (cintern ("REPEAT-NONE"), flocons(0), NIL);
  setvar (cintern ("REPEAT-SAWTOOTH"), flocons(1), NIL);
  setvar (cintern ("REPEAT-TRIANGULAR"), flocons(2), NIL);

  setvar (cintern ("FG-BUCKET-FILL"), flocons (0), NIL);
  setvar (cintern ("BG-BUCKET-FILL"), flocons (1), NIL);
  setvar (cintern ("PATTERN-BUCKET-FILL"), flocons (2), NIL);

  setvar (cintern ("BG-IMAGE-FILL"), flocons (0), NIL);
  setvar (cintern ("WHITE-IMAGE-FILL"), flocons (1), NIL);
  setvar (cintern ("TRANS-IMAGE-FILL"), flocons (2), NIL);
  setvar (cintern ("NO-IMAGE-FILL"), flocons (3), NIL);

  setvar (cintern ("RGB"), flocons (0), NIL);
  setvar (cintern ("GRAY"), flocons (1), NIL);
  setvar (cintern ("INDEXED"), flocons (2), NIL);
  setvar (cintern ("U16_RGB"), flocons (3), NIL);
  setvar (cintern ("U16_GRAY"), flocons (4), NIL);
  setvar (cintern ("U16_INDEXED"), flocons (5), NIL);
  setvar (cintern ("FLOAT_RGB"), flocons (6), NIL);
  setvar (cintern ("FLOAT_GRAY"), flocons (7), NIL);
  setvar (cintern ("FLOAT16_RGB"), flocons (6), NIL);
  setvar (cintern ("FLOAT16_GRAY"), flocons (7), NIL);

  setvar (cintern ("RGB_IMAGE"), flocons (0), NIL);
  setvar (cintern ("RGBA_IMAGE"), flocons (1), NIL);
  setvar (cintern ("GRAY_IMAGE"), flocons (2), NIL);
  setvar (cintern ("GRAYA_IMAGE"), flocons (3), NIL);
  setvar (cintern ("INDEXED_IMAGE"), flocons (4), NIL);
  setvar (cintern ("INDEXEDA_IMAGE"), flocons (5), NIL);
  setvar (cintern ("U16_RGB_IMAGE"), flocons (6), NIL);
  setvar (cintern ("U16_RGBA_IMAGE"), flocons (7), NIL);
  setvar (cintern ("U16_GRAY_IMAGE"), flocons (8), NIL);
  setvar (cintern ("U16_GRAYA_IMAGE"), flocons (9), NIL);
  setvar (cintern ("U16_INDEXED_IMAGE"), flocons (10), NIL);
  setvar (cintern ("U16_INDEXEDA_IMAGE"), flocons (11), NIL);
  setvar (cintern ("FLOAT_RGB_IMAGE"), flocons (12), NIL);
  setvar (cintern ("FLOAT_RGBA_IMAGE"), flocons (13), NIL);
  setvar (cintern ("FLOAT_GRAY_IMAGE"), flocons (14), NIL);
  setvar (cintern ("FLOAT_GRAYA_IMAGE"), flocons (15), NIL);
  setvar (cintern ("FLOAT16_RGB_IMAGE"), flocons (12), NIL);
  setvar (cintern ("FLOAT16_RGBA_IMAGE"), flocons (13), NIL);
  setvar (cintern ("FLOAT16_GRAY_IMAGE"), flocons (14), NIL);
  setvar (cintern ("FLOAT16_GRAYA_IMAGE"), flocons (15), NIL);

  setvar (cintern ("RED-CHANNEL"), flocons (0), NIL);
  setvar (cintern ("GREEN-CHANNEL"), flocons (1), NIL);
  setvar (cintern ("BLUE-CHANNEL"), flocons (2), NIL);
  setvar (cintern ("GRAY-CHANNEL"), flocons (3), NIL);
  setvar (cintern ("INDEXED-CHANNEL"), flocons (4), NIL);

  setvar (cintern ("WHITE-MASK"), flocons (0), NIL);
  setvar (cintern ("BLACK-MASK"), flocons (1), NIL);
  setvar (cintern ("ALPHA-MASK"), flocons (2), NIL);

  setvar (cintern ("APPLY"), flocons (0), NIL);
  setvar (cintern ("DISCARD"), flocons (1), NIL);

  setvar (cintern ("EXPAND-AS-NECESSARY"), flocons (0), NIL);
  setvar (cintern ("CLIP-TO-IMAGE"), flocons (1), NIL);
  setvar (cintern ("CLIP-TO-BOTTOM-LAYER"), flocons (2), NIL);

  setvar (cintern ("ADD"), flocons (0), NIL);
  setvar (cintern ("SUB"), flocons (1), NIL);
  setvar (cintern ("REPLACE"), flocons (2), NIL);
  setvar (cintern ("INTERSECT"), flocons (3), NIL);

  setvar (cintern ("PIXELS"), flocons (0), NIL);
  setvar (cintern ("POINTS"), flocons (1), NIL);

  setvar (cintern ("IMAGE-CLONE"), flocons (0), NIL);
  setvar (cintern ("PATTERN-CLONE"), flocons (1), NIL);

  setvar (cintern ("BLUR"), flocons (0), NIL);
  setvar (cintern ("SHARPEN"), flocons (1), NIL);

  setvar (cintern ("TRUE"), flocons (1), NIL);
  setvar (cintern ("FALSE"), flocons (0), NIL);

  /*  Script-fu types  */
  setvar (cintern ("SF-IMAGE"), flocons (SF_IMAGE), NIL);
  setvar (cintern ("SF-DRAWABLE"), flocons (SF_DRAWABLE), NIL);
  setvar (cintern ("SF-LAYER"), flocons (SF_LAYER), NIL);
  setvar (cintern ("SF-CHANNEL"), flocons (SF_CHANNEL), NIL);
  setvar (cintern ("SF-COLOR"), flocons (SF_COLOR), NIL);
  setvar (cintern ("SF-TOGGLE"), flocons (SF_TOGGLE), NIL);
  setvar (cintern ("SF-VALUE"), flocons (SF_VALUE), NIL);
#endif
}


/* 
   Search through the gimps parsley directories and source
   any scripts that we find. This will also register any
   parsley scripts with the pdb so that we can call them
   via the pdb (so from the menus, in particular).
*/

void
parsley_add_scripts (void)
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *path_str;
  gchar *home;
  gchar *local_path;
  gchar *path;
  gchar *filename;
  gchar *token;
  gchar *next_token;
  gchar *command;
  gint   err;
  DIR   *dir;
  struct dirent *dir_ent;

  /*  Make sure to clear any existing scripts  */
  if (script_list != NULL)
    {
      GList *list;
      ParsleyScript *script;

      list = script_list;
      while (list)
	{
	  script = (ParsleyScript *) list->data;
	  parsley_free_script (script);
	  list = list->next;
	}

      if (script_list)
	g_list_free (script_list);
      script_list = NULL;
    }

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "parsley-scripts-path",
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {

      path_str = return_vals[1].data.d_string;

      if (path_str == NULL)
	return;

      /* Set local path to contain temp_path, where (supposedly)
       * there may be working files.
       */

      home = getenv ("HOME");
      local_path = g_strdup (path_str);

      /* Search through all directories in the local path */

      next_token = local_path;

      token = strtok (next_token, ":");

      while (token)
	{
	  if (*token == '~')
	    {
	      path = (gchar *)g_malloc (strlen (home) + strlen (token) + 2);
	      sprintf (path, "%s%s", home, token + 1);
	    }
	  else
	    {
	      path = (gchar *)g_malloc (strlen (token) + 2);
	      strcpy (path, token);
	    } /* else */

	  /* Check if directory exists and if it has any items in it */
	  err = stat (path, &filestat);

	  if (!err && S_ISDIR (filestat.st_mode))
	    {
	      if (path[strlen (path) - 1] != '/')
		strcat (path, "/");

	      /* Open directory */
	      dir = opendir (path);

	      if (!dir)
		g_warning("error reading script directory \"%s\"", path);
	      else
		{
		  while ((dir_ent = readdir (dir)))
		    {
		      filename = (gchar*)g_malloc (strlen(path) + strlen (dir_ent->d_name) + 1);

		      sprintf (filename, "%s%s", path, dir_ent->d_name);

		      if (strcmp (filename + strlen (filename) - 4, ".psh") == 0)
			{
			  /* Check the file and see that it is not a sub-directory */
			  err = stat (filename, &filestat);

			  if (!err && S_ISREG (filestat.st_mode))
			    {
			      HU_SourceFile (filename);
			    }
			}

		      g_free (filename);
		    } /* while */

		  closedir (dir);
		} /* else */
	    } /* if */

	  g_free (path);

	  token = strtok (NULL, ":");
	} /* while */

      g_free(local_path);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}


/*
  Converts from a pdb to a parsley type name.
  example: this_is_a_routine becomes thisIsARoutine.
*/

static void
convert_pdb_to_parsley_string (char *str, char *parsley_str)
{
  gint make_cap = FALSE;
  while (*str)
    {
      if (*str != '_') 
	{
	  *parsley_str = *str;
	  if (make_cap)
	    {
	       *parsley_str = toupper (*parsley_str);
	       make_cap = FALSE;
	    }
          str++;
          parsley_str++;
	}
      else  /* its an underscore */
	{
          str++;
	  make_cap = TRUE;
	}
    }
   *parsley_str = '\0';
}

static void
convert_parsley_to_pdb_string (char *parsley_str, char *str)
{
  gint make_small = FALSE;
  while (*parsley_str)
    {
      if (isupper (*parsley_str)) 
	{
	  *str = '_';
	  *str++;
	  *str++ = tolower (*parsley_str++);
	}
      else
       *str++ = *parsley_str++;
    }
   *str = '\0';
}

/* 
This is the marshaller routine for calling the gimp procedures from parsley
scripts. argv has the name of the proc and all the arguments. We grab those and
set up a call to the pdb using gimp_run_procedure2. 
*/

Twine
gimp_proc_db_call(
		   List <Twine>& argv,
		   HU_ParserState& state 
		)
{
  Twine return_val;
  GParam *args;
  GParam *values;
  int nvalues;
  char *proc_name;
  char *proc_name_parsley;
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int nparams;
  int nreturn_vals;
  GParamDef *params;
  GParamDef *return_vals;
  char error_str[256];
  int i, j;
  int success = TRUE;
  char *string;
  int string_len;
  char buffer[128];
  int length;
  
  /*  Make sure there are arguments  */
  if (!argv.size())
    {
      printf("pdb marshaller was called with no arguments\n"); 
      return return_val; 
    }

  /*  Derive the pdb procedure name from the argument or first argument of a list  */
  proc_name_parsley = (char*)(const char *)argv[0];
 
  char *c = proc_name_parsley;
  int count = 0;
  while (*c)
    {
      if (isupper (*c))
	  count++;
	c++;
    }
	
  proc_name = (char *) g_new (char, strlen (proc_name_parsley) + count);
  convert_parsley_to_pdb_string (proc_name_parsley, proc_name);

  /*  Attempt to fetch the procedure from the database  */
  if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
			    &proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
			    &params, &return_vals) == FALSE)
    {
      printf("Invalid procedure name %s specified.\n", proc_name); 
      return return_val; 
    }

  /*  Check the supplied number of arguments  */
  if ((argv.size() - 1) != nparams)
    {
      printf ("Invalid arguments passed to %s \n", proc_name );
      printf ("got %d expected %d\n", argv.size()-1, nparams);
      return return_val; 
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = (GParam *) g_new (GParam, nparams);
  else
    args = NULL;

  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
	{
  	case PARAM_INT32:
	  args[i].type = PARAM_INT32;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_INT16:
	  args[i].type = PARAM_INT16;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_INT8:
	  args[i].type = PARAM_INT8;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_FLOAT:
	  args[i].type = PARAM_FLOAT;
	  args[i].data.d_float = atof ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_STRING:
	  args[i].type = PARAM_STRING;
	  args[i].data.d_string = (char *)(const char *)argv[i+1];
	  success = TRUE;
	  break;
	case PARAM_INT32ARRAY:
	case PARAM_INT16ARRAY:
	case PARAM_INT8ARRAY:
	case PARAM_FLOATARRAY:
	case PARAM_STRINGARRAY:
	  printf("gimp_proc_db_call: arrays not supported as argument\n");
	  success = FALSE;
	  break;
#if 0
	case PARAM_INT32ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT32ARRAY;
	      args[i].data.d_int32array = (gint32*) (car (a))->storage_as.long_array.data;
	    }
	  break;
	case PARAM_INT16ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT16ARRAY;
	      args[i].data.d_int16array = (short *) (car (a))->storage_as.long_array.data;
	    }
	  break;
	case PARAM_INT8ARRAY:
	  if (!TYPEP (car (a), tc_byte_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT8ARRAY;
	      args[i].data.d_int8array = (gint8 *) (car (a))->storage_as.string.data;
	    }
	  break;
	case PARAM_FLOATARRAY:
	  if (!TYPEP (car (a), tc_double_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_FLOATARRAY;
	      args[i].data.d_floatarray = (car (a))->storage_as.double_array.data;
	    }
	  break;
	case PARAM_STRINGARRAY:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_STRINGARRAY;

	      /*  Set the array  */
	      {
		gint j;
		gint num_strings;
		gchar **array;
		LISP list;

		list = car (a);
		num_strings = args[i - 1].data.d_int32;
		if (nlength (list) != num_strings)
		  return err ("String array argument has incorrectly specified length", NIL);
		array = args[i].data.d_stringarray = g_new (char *, num_strings);

		for (j = 0; j < num_strings; j++)
		  {
		    array[j] = get_c_string (car (list));
		    list = cdr (list);
		  }
	      }
	    }
	  break;
#endif
	case PARAM_COLOR:
#if 0
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_COLOR;
	      color_list = car (a);
	      args[i].data.d_color.red = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.green = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.blue = get_c_long (car (color_list));
	    }
#endif
	  printf("gimp_proc_db_call: color not supported as argument\n");
	  success = FALSE;
	  break;
	case PARAM_REGION:
	  printf("gimp_proc_db_call: Regions not supported as argument\n");
	  success = FALSE;
	  break;
	case PARAM_DISPLAY:
	  args[i].type = PARAM_DISPLAY;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_IMAGE:
	  args[i].type = PARAM_IMAGE;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_LAYER:
	  args[i].type = PARAM_LAYER;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_CHANNEL:
	  args[i].type = PARAM_CHANNEL;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_DRAWABLE:
	  args[i].type = PARAM_DRAWABLE;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_SELECTION:
	  args[i].type = PARAM_SELECTION;
	  args[i].data.d_int32 = atoi ((const char *)argv[i+1]);
	  success = TRUE;
	  break;
	case PARAM_BOUNDARY:
	  printf("gimp_proc_db_call: Boundaries unsupported as arguments\n");
	  success = FALSE;
	  break;
	case PARAM_PATH:
	  printf("gimp_proc_db_call: Paths unsupported as arguments\n");
	  success = FALSE;
	  break;
	case PARAM_STATUS:
	  printf("gimp_proc_db_call: PARAM_STATUS not a valid argument\n");
	  success = FALSE;
	  break;
	default:
	  printf("gimp_proc_db_call: Unsupported type for argument\n");
	  success = FALSE;
	  break;
	}
    }

  if (success)
    values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
  else
    {
      printf("gimp_proc_db_call: Invalid types specified for arguments\n");
      return return_val;
    }

  /*  Check the return status  */
  if (! values)
	{
	  printf("gimp_proc_db_call: pdb returned no values\n");
	  return return_val;
	}


  switch (values[0].data.d_status)
    {
    case STATUS_EXECUTION_ERROR:
      printf("gimp_proc_db_call: pdb execution error\n");
      return return_val;
      break;
    case STATUS_CALLING_ERROR:
      printf("gimp_proc_db_call: pdb execution failed invalid input\n");
      return return_val;
      break;
    case STATUS_SUCCESS:
      return_val = "";

      for (i = 0; i < nvalues - 1; i++)
	{
	  switch (return_vals[i].type)
	    {
	    case PARAM_INT32:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_INT16:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_INT8:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_FLOAT:
	      sprintf (buffer, "%f", values[i + 1].data.d_float);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_STRING:
	      return_val += Twine ((const char *)(values[i + 1].data.d_string));
	      break;
	    case PARAM_INT32ARRAY:
	      length = values[i].data.d_int32;
	      for (j = 0; j < length; j++)
		{
		  sprintf (buffer, "%d", values[i + 1].data.d_int32array[j]);
		  return_val += Twine (buffer); 	
		  if ( j < length - 1 )
		    return_val += " ";
		}
	      break;
	    case PARAM_INT16ARRAY:
	      length = values[i].data.d_int32;
	      for (j = 0; j < length; j++)
		{
		  sprintf (buffer, "%d", values[i + 1].data.d_int16array[j]);
		  return_val += Twine (buffer); 	
		  if ( j < length - 1 )
		    return_val += " ";
		}
	      break;
	    case PARAM_INT8ARRAY:
	      length = values[i].data.d_int32;
	      for (j = 0; j < values[i].data.d_int32; j++)
		{
		  sprintf (buffer, "%d", values[i + 1].data.d_int8array[j]);
		  return_val += Twine (buffer); 	
		  if ( j < length - 1 )
		    return_val += " ";
		}
	      break;
	    case PARAM_FLOATARRAY:
	      length = values[i].data.d_int32;
	      for (j = 0; j < values[i].data.d_int32; j++)
		{
		  sprintf (buffer, "%f", values[i + 1].data.d_floatarray[j]);
		  return_val += Twine (buffer); 	
		  if ( j < length - 1 )
		    return_val += " ";
		}
	      break;
	    case PARAM_STRINGARRAY:
	      length = values[i].data.d_int32;
	      for (j = 0; j < values[i].data.d_int32; j++)
		{
	          return_val += Twine ((const char *)(values[i + 1].data.d_stringarray[j]));
		  if ( j < length - 1 )
		    return_val += " ";
		}
	      break;
	    case PARAM_COLOR:
	      break;
	    case PARAM_REGION:
	      break;
	    case PARAM_DISPLAY:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_IMAGE:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_LAYER:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_CHANNEL:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_DRAWABLE:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_SELECTION:
	      sprintf (buffer, "%d", values[i + 1].data.d_int32);
	      return_val += Twine (buffer); 	
	      break;
	    case PARAM_BOUNDARY:
	      break;
	    case PARAM_PATH:
	      break;
	    case PARAM_STATUS:
	      break;
	    default:
	      break;
	    }
	  if ( i < nvalues - 2 )
	    return_val += " ";
	}
      break;
    }

  /*  free up the executed procedure return values  */
  gimp_destroy_params (values, nvalues);

  /*  free up arguments and values  */
  if (args)
    g_free (args);

  /*  free the query information  */
  g_free (proc_blurb);
  g_free (proc_name);
  g_free (proc_help);
  g_free (proc_author);
  g_free (proc_copyright);
  g_free (proc_date);
  g_free (params);
  g_free (return_vals);

  return return_val;
}

/* This registers a parsley script with gimp */
Twine
gimp_register_call (
			List <Twine>& argv,	
			HU_ParserState& state
			)
{
  ParsleyScript *script;
  GParamDef *args;
  char *val;
  int i;
  gfloat color[3];
  gint toggle_value;
  Twine return_val = "";

#if 0
  printf("gimp_register_called\n");

  for(i = 0; i < argv.size(); i++)
	printf("argv[%d] = %s\n", i, (const char *) argv[i]); 
#endif 

  /*  See if we passed enough args  */
  if (argv.size() < 8)
    {
      printf("Not enough args for gimp_register_call\n") ;
      return return_val;
    }

  /*  Create a new script  */
  script = g_new (ParsleyScript, 1);

  script->script_name = g_strdup (argv[1]);
  script->description = g_strdup (argv[2]);
  script->help = g_strdup (argv[3]);
  script->author = g_strdup (argv[4]);
  script->copyright = g_strdup (argv[5]);
  script->date = g_strdup (argv[6]);
  script->img_types = g_strdup (argv[7]);

  /*  Check the supplied number of arguments  */
  script->num_args = (argv.size() - 8) / 3;

  if (script->num_args > 0)
    {
      script->arg_types = g_new (ParsleyArgType, script->num_args);
      script->arg_labels = g_new (char *, script->num_args);
      script->arg_defaults = g_new (ParsleyArgValue, script->num_args);
      script->arg_values = g_new (ParsleyArgValue, script->num_args);
      args = g_new (GParamDef, script->num_args + 1);
      args[0].type = PARAM_INT32;
      args[0].name = "run_mode";
      args[0].description = "non-interactive";

      for (i = 0; i < script->num_args; i++)
	{
	  gint j = 8 + 3*i;
#if 0
	  printf("the next 3 args are %s %s %s\n",(const char *)argv[j], 
						  (const char *)argv[j+1], 
						  (const char *)argv[j+2]);

	 /* eg a set of three args is PARSLEY_IMAGE Image 0 */
#endif
	  if (argv[j])
	      script->arg_types[i] = (ParsleyArgType)atoi ((const char *)argv[j]);

	  if (argv[j +1])
	      script->arg_labels[i] = g_strdup ((const char *)argv[j+1]);

	  if (argv[j +2])
	    {
	      switch (script->arg_types[i])
		{
		case PARSLEY_IMAGE:
		case PARSLEY_DRAWABLE:
		case PARSLEY_LAYER:
		case PARSLEY_CHANNEL:
		  script->arg_defaults[i].pa_image = atoi ( (const char *)argv[j + 2]);
		  script->arg_values[i].pa_image = script->arg_defaults[i].pa_image;

		  switch (script->arg_types[i])
		    {
		    case PARSLEY_IMAGE:
		      args[i + 1].type = PARAM_IMAGE;
		      args[i + 1].name = "image";
		      break;
		    case PARSLEY_DRAWABLE:
		      args[i + 1].type = PARAM_DRAWABLE;
		      args[i + 1].name = "drawable";
		      break;
		    case PARSLEY_LAYER:
		      args[i + 1].type = PARAM_LAYER;
		      args[i + 1].name = "layer";
		      break;
		    case PARSLEY_CHANNEL:
		      args[i + 1].type = PARAM_CHANNEL;
		      args[i + 1].name = "channel";
		      break;
		    default:
		      break;
		    }

		  args[i + 1].description = script->arg_labels[i];
		  break;

		case PARSLEY_COLOR:

		  /* This must be a list of 3 numbers */

		  sscanf ( (const char *)argv[j+2], "%f %f %f" , &color[0], &color[1], &color[2]);
 		  for (i = 0 ; i < 3; i++)  
		    color[i] =  color[i] / 255.0;
		   
 		  for (i = 0 ; i < 3; i++)  
	  	    {	
		      script->arg_defaults[i].pa_color.color[i] =  color[i];
		      script->arg_values[i].pa_color.color[i] = color[i];
		    }

		  args[i + 1].type = PARAM_COLOR;
		  args[i + 1].name = "color";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case PARSLEY_TOGGLE:
		  toggle_value = ( atoi ((const char *)argv[j+2]) ) ? TRUE : FALSE;
		  script->arg_defaults[i].pa_toggle = toggle_value;
		  script->arg_values[i].pa_toggle = toggle_value;

		  args[i + 1].type = PARAM_INT32;
		  args[i + 1].name = "toggle";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case PARSLEY_VALUE:
		  script->arg_defaults[i].pa_value = g_strdup ((const char *)argv[j+2]);

		  args[i + 1].type = PARAM_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;
		default:
		  break;
		}
	    }
	  else
	    {
	    printf("gimp_register_call: missing default argument\n");
	    return return_val; 
	    }
	}
    }
#if 0
      for (i = 0; i < script->num_args+1; i++)
	{
	  printf ("args[%d].type is %d\n",  i, args[i].type);
	  printf ("args[%d].name is %s\n", i, args[i].name);
	  printf ("args[%d].description is %s\n", i, args[i].description);
	}
#endif

  gimp_install_temp_proc (script->script_name,
			  script->description,
			  script->help,
			  script->author,
			  script->copyright,
			  script->date,
			  script->description,
			  script->img_types,
			  PROC_TEMPORARY,
			  script->num_args + 1, 0,
			  args, NULL,
			  parsley_script_proc);


  g_free (args);
  script_list = g_list_append (script_list, script);
  return return_val;
}

/* 
   This executes the command after it is passed from gimp to
   the parsley extension.
*/ 

static void
parsley_script_proc (char     *name,
		       int       nparams,
		       GParam   *params,
		       int      *nreturn_vals,
		       GParam  **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;
  ParsleyScript *script;

  run_mode = (GRunModeType)params[0].data.d_int32;

  if (! (script = parsley_find_script (name)))
    status = STATUS_CALLING_ERROR;
  else
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_NONINTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != (script->num_args + 1))
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      gint err_msg;
	      char *text;
	      char *c;
	      char buffer[32];
	      int length;
	      int i;
	      Twine command;

		command = script->script_name;
		command += "(";

	      for (i = 0; i < script->num_args; i++)
		{
		  switch (script->arg_types[i])
		    {
		    case PARSLEY_IMAGE:
		    case PARSLEY_DRAWABLE:
		    case PARSLEY_LAYER:
		    case PARSLEY_CHANNEL:
		      sprintf (buffer, "%d", params[i + 1].data.d_image);
		      command += Twine (buffer);
		      break;
		    case PARSLEY_COLOR:
		      sprintf (buffer, "'(%d %d %d)",
			       params[i + 1].data.d_color.red,
			       params[i + 1].data.d_color.green,
			       params[i + 1].data.d_color.blue);
		      command += Twine (buffer);
		      break;
		    case PARSLEY_TOGGLE:
		      sprintf (buffer, "%s", (params[i + 1].data.d_int32) ? "TRUE" : "FALSE");
		      command += Twine (buffer);
		      break;
		    case PARSLEY_VALUE:
		      command += params[i + 1].data.d_string;
		      break;
		    default:
		      break;
		    }

		  if (i == script->num_args - 1)
			command += ")"; 
		  else
			command += ", ";
		}
	      /*  run the command through parsley  */
	      HU_Execute (HU_Expand (command));
	    }
	  break;

	default:
	  break;
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
parsley_free_script (ParsleyScript *script)
{
  int i;

  /*  Uninstall the temporary procedure for this script  */
  gimp_uninstall_temp_proc (script->script_name);

  if (script)
    {
      g_free (script->script_name);
      g_free (script->description);
      g_free (script->help);
      g_free (script->author);
      g_free (script->copyright);
      g_free (script->date);
      g_free (script->img_types);
      g_free (script->arg_types);

      for (i = 0; i < script->num_args; i++)
	{
	  g_free (script->arg_labels[i]);
	  switch (script->arg_types[i])
	    {
	    case PARSLEY_IMAGE:
	    case PARSLEY_DRAWABLE:
	    case PARSLEY_LAYER:
	    case PARSLEY_CHANNEL:
	    case PARSLEY_COLOR:
	      break;
	    case PARSLEY_VALUE:
	      g_free (script->arg_defaults[i].pa_value);
	      break;
	    default:
	      break;
	    }
	}

      g_free (script->arg_labels);
      g_free (script->arg_defaults);
      g_free (script->arg_values);

      g_free (script);
    }
}

static ParsleyScript *
parsley_find_script (gchar *script_name)
{
  GList *list;
  ParsleyScript *script;

  list = script_list;
  while (list)
    {
      script = (ParsleyScript *) list->data;
      if (! strcmp (script->script_name, script_name))
	return script;

      list = list->next;
    }

  return NULL;
}
