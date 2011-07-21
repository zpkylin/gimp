/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationfindoutline.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimpoperationfindoutline.h"

#include <stdio.h> /* TODO: get rid of this debugging way! */

enum
{
  PROP0,
  PROP_POINTS,
  PROP_THRESHOLD,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};

static void
gimp_operation_find_outline_init (GimpOperationFindOutline *self);

static void
gimp_operation_find_outline_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec);

static void
gimp_operation_find_outline_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);


static gboolean gimp_operation_find_outline_process (GeglOperation       *operation,
                                                     GeglBuffer          *in_buf,
                                                     const GeglRectangle *roi);

static void
gimp_operation_find_outline_prepare (GeglOperation *operation);

G_DEFINE_TYPE (GimpOperationFindOutline, gimp_operation_find_outline,
               GEGL_TYPE_OPERATION_SINK)

#define parent_class gimp_operation_find_outline_parent_class

static void
gimp_operation_find_outline_class_init (GimpOperationFindOutlineClass *klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->set_property = gimp_operation_find_outline_set_property;
  object_class->get_property = gimp_operation_find_outline_get_property;

  operation_class->prepare     = gimp_operation_find_outline_prepare;
  operation_class->name        = "gimp:find-outline";
  operation_class->categories  = "programming";
  operation_class->description = "GIMP Outline Finding operation";

  sink_class->process    = gimp_operation_find_outline_process;
  sink_class->needs_full = TRUE;

  g_object_class_install_property (object_class, PROP_POINTS,
                                   g_param_spec_pointer ("points",
                                                         "Points",
                                                         "A GPtrArray* of struct {gint x, y;} containing the points",
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_THRESHOLD,
                                   g_param_spec_float ("threshold",
                                                         "Threshold",
                                                         "The threshold for determining the difference between black and white (white >= thres)",
                                                         -G_MAXFLOAT, G_MAXFLOAT, 0.5f,
                                                         G_PARAM_READWRITE));

  /* Workaround around wrong ROI calculation in GEGL for Sink ops when needs_full is set */
  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "X",
                                                     "The minimal x of the scaning rect",
                                                     -G_MAXINT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y",
                                                     "The minimal Y of the scaning rect",
                                                     -G_MAXINT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                     "Width",
                                                     "The width of the scaning rect",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                     "Height",
                                                     "The height of the scaning rect",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
}

static void
gimp_operation_find_outline_init (GimpOperationFindOutline *self)
{
}

static void
gimp_operation_find_outline_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpOperationFindOutline *self = GIMP_OPERATION_FIND_OUTLINE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      g_value_set_pointer (value, self->ptList);
      break;
    case PROP_THRESHOLD:
      g_value_set_float (value, self->threshold);
      break;
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, self->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, self->height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_find_outline_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpOperationFindOutline *self = GIMP_OPERATION_FIND_OUTLINE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      self->ptList = g_value_get_pointer (value);
      break;
    case PROP_THRESHOLD:
      self->threshold = g_value_get_float (value);
      break;
    case PROP_X:
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      self->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*******************************************************************/
/* Real operation logic begins here                                */
/*******************************************************************/

/* Define the directions
 *
 *          0
 *         
 *    7     N     1
 *          ^
 *          |
 *          |           
 * 6  W<----+---->E  2
 *          |              =====>X
 *          |             ||
 *          v             ||
 *    5     S     3       ||
 *                        vv
 *          4             Y
 */
typedef enum {
  D_N = 0,    /* 000 */
  D_NE = 1,   /* 001 */
  D_E = 2,    /* 010 */
  D_SE = 3,   /* 011 */
  D_S = 4,    /* 100 */
  D_SW = 5,   /* 101 */
  D_W = 6,    /* 110 */
  D_NW = 7    /* 111 */
} OUTLINE_DIRECTION;

#define cwdirection(t)       (((t)+1)%8)
#define ccwdirection(t)      (((t)+7)%8)
#define oppositedirection(t) (((t)+4)%8)

#define isSouth(s) (((s) == D_S) || ((s) == D_SE) || ((s) == D_SW))
#define isNorth(s) (((s) == D_N) || ((s) == D_NE) || ((s) == D_NW))

#define isEast(s) (((s) == D_NE) || ((s) == D_E) || ((s) == D_SE))
#define isWest(s) (((s) == D_NW) || ((s) == D_W) || ((s) == D_SW))

#define BABL_FORMAT
typedef struct {
  gint x, y;
} SPoint;

/**
 * Add a COPY of the given point into the array pts. The original point CAN
 * be freed later!
 */
static void
gimp_operation_find_outline_add_point (GPtrArray* pts, SPoint *pt)
{
  printf ("Added %d,%d\n", pt->x, pt->y);
  if (pts == NULL)
    return;
  SPoint *cpy = g_new (SPoint, 1);
  cpy->x = pt->x;
  cpy->y = pt->y;

  g_ptr_array_add (pts, cpy);
}

static inline void
gimp_operation_find_outline_move (SPoint *pt, OUTLINE_DIRECTION t, SPoint *dest)
{
  dest->x = pt->x + (isEast(t) ? 1 : (isWest(t) ? -1 : 0));
  dest->y = pt->y + (isSouth(t) ? 1 : (isNorth(t) ? -1 : 0));
}

#define in_range(val,min,max)  (((min) <= (val)) && ((val) <= (max)))

static inline gboolean
gimp_operation_find_outline_is_inside (OutlineProcPrivate *OPP, SPoint *pt)
{
  return in_range (pt->x, OPP->xmin, OPP->xmax)
         && in_range (pt->y, OPP->ymin, OPP->ymax);
}

#define get_value(OPP,x,y)  (((OPP)->im)[(((y)-(OPP)->ymin)*(OPP)->rowstride+(x)-(OPP)->xmin)*4+3])
#define get_valuePT(OPP,pt) get_value(OPP,(pt)->x,(pt)->y)

static inline gboolean
gimp_operation_find_outline_is_white (OutlineProcPrivate *OPP, SPoint *pt)
{
  return gimp_operation_find_outline_is_inside (OPP, pt) && get_valuePT (OPP,pt) >= OPP->threshold;
}

/* This function receives a white pixel (pt) and the direction of the movement
 * that lead to it (prevdirection). It will return the direction leading to the
 * next white pixel (while following the edges in CW order), and the pixel
 * itself would be stored in dest.
 *
 * The logic is simple:
 * 1. Try to continue in the same direction that lead us to the current pixel
 * 2. Dprev = oppposite(prevdirection)
 * 3. Dnow  = cw(Dprev)
 * 4. While moving to Dnow is white:
 * 4.1. Dprev = Dnow
 * 4.2. Dnow  = cw(D)
 * 5. Return the result - moving by Dprev
 */
static inline OUTLINE_DIRECTION
gimp_operation_find_outline_walk_cw (OutlineProcPrivate *OPP,
                                     OUTLINE_DIRECTION   prevdirection,
                                     SPoint             *pt,
                                     SPoint             *dest)
{
  OUTLINE_DIRECTION Dprev = oppositedirection(prevdirection);
  OUTLINE_DIRECTION Dnow = cwdirection (Dprev);
 
  SPoint ptN, ptP;

  gimp_operation_find_outline_move (pt, Dprev, &ptP);
  gimp_operation_find_outline_move (pt, Dnow, &ptN);

  while (gimp_operation_find_outline_is_white (OPP, &ptN))
    {
       Dprev = Dnow;
       ptP.x = ptN.x;
       ptP.y = ptN.y;
       Dnow = cwdirection (Dprev);
       gimp_operation_find_outline_move (pt, Dnow, &ptN);
    }

  dest->x = ptP.x;
  dest->y = ptP.y;
  return Dprev;
}

#define pteq(pt1,pt2) (((pt1)->x == (pt2)->x) && ((pt1)->y == (pt2)->y))

static void
gimp_operation_find_outline_find_outline_ccw (OutlineProcPrivate *OPP, GPtrArray *pts)
{
  gint x = OPP->xmin;
  gint y;
  gboolean found = FALSE;
  SPoint START, pt, ptN;
  OUTLINE_DIRECTION DIR, DIRN;
  
  /* First of all try to find a white pixel */
  for (y = OPP->ymin; y < OPP->ymax; y++)
    {
      for (x = OPP->xmin; x < OPP->xmax; x++)
        {
          if (get_value (OPP, x, y) >= OPP->threshold)
            {
               printf ("Found first white on %d,%d\n",x,y);
               found = TRUE;
               break;
            }
        }
      if (found) break;
    }

  if (!found)
    return;
  
  pt.x = START.x = x;
  pt.y = START.y = y;
  DIR = D_NW;

  gimp_operation_find_outline_add_point (pts, &START);

  DIRN = gimp_operation_find_outline_walk_cw (OPP, DIR,&pt,&ptN);

  while (! pteq(&ptN,&START))
    {
      gimp_operation_find_outline_add_point (pts, &ptN);
      pt.x = ptN.x;
      pt.y = ptN.y;
      DIR = DIRN;
      DIRN = gimp_operation_find_outline_walk_cw (OPP, DIR,&pt,&ptN);
    }
}

static gboolean
gimp_operation_find_outline_process (GeglOperation       *operation,
                                     GeglBuffer          *input,
                                     const GeglRectangle *result)
{
  GimpOperationFindOutline *self = GIMP_OPERATION_FIND_OUTLINE (operation);
  OutlineProcPrivate *OPP = (OutlineProcPrivate *) &GIMP_OPERATION_FIND_OUTLINE(operation)->opp;
  GeglRectangle rect;

  rect.x = OPP->xmin = self->x;
  rect.width = self->width;
  OPP->xmax = self->x + self->width - 1;

  rect.y = OPP->ymin = self->y;
  rect.height = self->height;
  OPP->ymax = self->y + self->height - 1;

  OPP->threshold = self->threshold;
  OPP->im = g_new (gfloat,4 * (rect.width) * rect.height);
  OPP->rowstride = rect.width;

  gegl_buffer_get (input, 1.0, &rect, babl_format("RGBA float"), OPP->im, GEGL_AUTO_ROWSTRIDE);

  gimp_operation_find_outline_find_outline_ccw (OPP, self->ptList);

  g_free (OPP->im);
  OPP->im = NULL;

  return  TRUE;
}

static void
gimp_operation_find_outline_prepare (GeglOperation *operation)
{
  printf ("Preparing!\n");

  gegl_operation_set_format (operation, "input", babl_format("RGBA float"));
}
