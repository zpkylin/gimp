#ifndef __GIMAGE_H__
#define __GIMAGE_H__

#include "gimpimage_decl.h"
#include "tile_manager_decl.h"
#include "layer_decl.h"
#include "drawable_decl.h"
#include "pixel_region_decl.h"
#include "channel_decl.h"
#include "tile_decl.h"
#include "boundary_decl.h"
#include "temp_buf_decl.h"




#define GIMAGE(obj) GTK_CHECK_CAST (obj, gimage_get_type (), GImage)


#define GIMAGE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimage_get_type(), GImageClass)
     
     
#define IS_GIMAGE(obj) GTK_CHECK_TYPE (obj, gimage_get_type())
     

/* the image types */
typedef enum
{
	RGB_GIMAGE,
	RGBA_GIMAGE,
	GRAY_GIMAGE,
	GRAYA_GIMAGE,  
	INDEXED_GIMAGE,
	INDEXEDA_GIMAGE
} GImageType;



#define TYPE_HAS_ALPHA(t)  ((t)==RGBA_GIMAGE || (t)==GRAYA_GIMAGE || (t)==INDEXEDA_GIMAGE)

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

typedef enum
{
	RGB,
	GRAY,
	INDEXED
} GImageBaseType;



#define MAX_CHANNELS     4

/* the image fill types */
#define BACKGROUND_FILL  0
#define WHITE_FILL       1
#define TRANSPARENT_FILL 2
#define NO_FILL          3

#define COLORMAP_SIZE    768

#define HORIZONTAL_GUIDE 1
#define VERTICAL_GUIDE   2

typedef enum
{
  Red,
  Green,
  Blue,
  Gray,
  Indexed,
  Auxillary
} ChannelType;

typedef enum
{
  ExpandAsNecessary,
  ClipToImage,
  ClipToBottomLayer,
  FlattenImage
} MergeType;

struct _Guide
{
  int ref_count;
  int position;
  int orientation;
};

typedef struct _Guide Guide;


guint gimage_get_type(void);

GImage* gimage_new(int width, int height, GImageBaseType base_type);
void            gimage_set_filename           (GImage*, char*);
void            gimage_resize                 (GImage*, int, int, int, int);
void            gimage_scale                  (GImage*, int, int);
GImage*        gimage_get_ID                 (int);
TileManager*   gimage_shadow                 (GImage*, int, int, int);
void            gimage_free_shadow            (GImage*);
void            gimage_delete                 (GImage*);
void            gimage_apply_image            (GImage*, GimpDrawable*, PixelRegion*, int, int, int,
					       TileManager*, int, int);
void            gimage_replace_image          (GImage*, GimpDrawable*, PixelRegion*, int, int,
					       PixelRegion*, int, int);
void            gimage_get_foreground         (GImage*, GimpDrawable*, unsigned char*);
void            gimage_get_background         (GImage*, GimpDrawable*, unsigned char*);
void            gimage_get_color              (GImage*, int, unsigned char*,
					       unsigned char*);
void            gimage_transform_color        (GImage*, GimpDrawable*, unsigned char*,
					       unsigned char*, int);
Guide*          gimage_add_hguide             (GImage*);
Guide*          gimage_add_vguide             (GImage*);
void            gimage_add_guide              (GImage*, Guide*);
void            gimage_remove_guide           (GImage*, Guide*);
void            gimage_delete_guide           (GImage*, Guide*);


/*  layer/channel functions */

int             gimage_get_layer_index        (GImage*, Layer*);
int             gimage_get_channel_index      (GImage*, Channel*);
Layer*         gimage_get_active_layer       (GImage*);
Channel*       gimage_get_active_channel     (GImage*);
Channel*       gimage_get_mask               (GImage*);
int             gimage_get_component_active   (GImage*, ChannelType);
int             gimage_get_component_visible  (GImage*, ChannelType);
int             gimage_layer_boundary         (GImage*, BoundSeg**, int*);
Layer*         gimage_set_active_layer       (GImage*, Layer*);
Channel*       gimage_set_active_channel     (GImage*, Channel*);
Channel*       gimage_unset_active_channel   (GImage*);
void            gimage_set_component_active   (GImage*, ChannelType, int);
void            gimage_set_component_visible  (GImage*, ChannelType, int);
Layer*         gimage_pick_correlate_layer   (GImage*, int, int);
void            gimage_set_layer_mask_apply   (GImage*, int);
void            gimage_set_layer_mask_edit    (GImage*, Layer*, int);
void            gimage_set_layer_mask_show    (GImage*, int);
Layer*         gimage_raise_layer            (GImage*, Layer*);
Layer*         gimage_lower_layer            (GImage*, Layer*);
Layer*         gimage_merge_visible_layers   (GImage*, MergeType);
Layer*         gimage_flatten                (GImage*);
Layer*         gimage_merge_layers           (GImage*, GSList*, MergeType);
Layer*         gimage_add_layer              (GImage*, Layer*, int);
Layer*         gimage_remove_layer           (GImage*, Layer*);
LayerMask*     gimage_add_layer_mask         (GImage*, Layer*, LayerMask*);
Channel*       gimage_remove_layer_mask      (GImage*, Layer*, int);
Channel*       gimage_raise_channel          (GImage*, Channel*);
Channel*       gimage_lower_channel          (GImage*, Channel*);
Channel*       gimage_add_channel            (GImage*, Channel*, int);
Channel*       gimage_remove_channel         (GImage*, Channel*);
void            gimage_construct              (GImage*, int, int, int, int);
void            gimage_invalidate             (GImage*, int, int, int, int, int, int, int, int);
void            gimage_validate               (TileManager*, Tile*, int);
void            gimage_inflate                (GImage*);
void            gimage_deflate                (GImage*);


/*  Access functions */

int             gimage_is_flat                (GImage*);
int             gimage_is_empty               (GImage*);
GimpDrawable*  gimage_active_drawable        (GImage*);
int             gimage_base_type              (GImage*);
int             gimage_base_type_with_alpha   (GImage*);
char*          gimage_filename               (GImage*);
int             gimage_enable_undo            (GImage*);
int             gimage_disable_undo           (GImage*);
int             gimage_dirty                  (GImage*);
int             gimage_clean                  (GImage*);
void            gimage_clean_all              (GImage*);
Layer*         gimage_floating_sel           (GImage*);
unsigned char* gimage_cmap                   (GImage*);


/*  projection access functions */

TileManager*   gimage_projection             (GImage*);
int             gimage_projection_type        (GImage*);
int             gimage_projection_bytes       (GImage*);
int             gimage_projection_opacity     (GImage*);
void            gimage_projection_realloc     (GImage*);


/*  composite access functions */

TileManager*   gimage_composite              (GImage*);
int             gimage_composite_type         (GImage*);
int             gimage_composite_bytes        (GImage*);
TempBuf*       gimage_composite_preview      (GImage*, ChannelType, int, int);
int             gimage_preview_valid          (GImage*, ChannelType);
void            gimage_invalidate_preview     (GImage*);

void            gimage_invalidate_previews    (void);

/* from drawable.c*/
GImage*        drawable_gimage               (GimpDrawable*);

/* Bad hack, but there are hundreds of accesses to gimage members in
   the code currently */

#include "gimpimage_pvt.h"


#endif
