#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__

#include "gimpimage_decl.h"
#include "tile_manager_decl.h"
#include "layer_decl.h"
#include "drawable_decl.h"
#include "pixel_region_decl.h"
#include "channel_decl.h"
#include "tile_decl.h"



#define GIMP_IMAGE(obj) GTK_CHECK_CAST (obj, gimp_image_get_type (), GimpImage)


#define GIMP_IMAGE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_image_get_type(), GimpImageClass)
     
     
#define GIMP_IS_IMAGE(obj) GTK_CHECK_TYPE (obj, gimp_image_get_type())
     

/* the image types */
typedef enum
{
	RGB_GIMAGE,
	RGBA_GIMAGE,
	GRAY_GIMAGE,
	GRAYA_GIMAGE,  
	INDEXED_GIMAGE,
	INDEXEDA_GIMAGE
} GimpImageType;



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
} GimpImageBaseType;



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


guint gimp_image_get_type(void);

GimpImage* gimp_image_new(int width, int height, GimpImageBaseType base_type);
void            gimage_set_filename           (GimpImage*, char*);
void            gimage_resize                 (GimpImage*, int, int, int, int);
void            gimage_scale                  (GimpImage*, int, int);
GimpImage*        gimage_get_named              (char*);
GimpImage*        gimage_get_ID                 (int);
TileManager*   gimage_shadow                 (GimpImage*, int, int, int);
void            gimage_free_shadow            (GimpImage*);
void            gimage_delete                 (GimpImage*);
void            gimage_apply_image            (GimpImage*, GimpDrawable*, PixelRegion*, int, int, int,
					       TileManager*, int, int);
void            gimage_replace_image          (GimpImage*, GimpDrawable*, PixelRegion*, int, int,
					       PixelRegion*, int, int);
void            gimage_get_foreground         (GimpImage*, GimpDrawable*, unsigned char*);
void            gimage_get_background         (GimpImage*, GimpDrawable*, unsigned char*);
void            gimage_get_color              (GimpImage*, int, unsigned char*,
					       unsigned char*);
void            gimage_transform_color        (GimpImage*, GimpDrawable*, unsigned char*,
					       unsigned char*, int);
Guide*          gimage_add_hguide             (GimpImage*);
Guide*          gimage_add_vguide             (GimpImage*);
void            gimage_add_guide              (GimpImage*, Guide*);
void            gimage_remove_guide           (GimpImage*, Guide*);
void            gimage_delete_guide           (GimpImage*, Guide*);


/*  layer/channel functions */

int             gimage_get_layer_index        (GimpImage*, Layer*);
int             gimage_get_channel_index      (GimpImage*, Channel*);
Layer*         gimage_get_active_layer       (GimpImage*);
Channel*       gimage_get_active_channel     (GimpImage*);
Channel*       gimage_get_mask               (GimpImage*);
int             gimage_get_component_active   (GimpImage*, ChannelType);
int             gimage_get_component_visible  (GimpImage*, ChannelType);
int             gimage_layer_boundary         (GimpImage*, BoundSeg**, int*);
Layer*         gimage_set_active_layer       (GimpImage*, Layer*);
Channel*       gimage_set_active_channel     (GimpImage*, Channel*);
Channel*       gimage_unset_active_channel   (GimpImage*);
void            gimage_set_component_active   (GimpImage*, ChannelType, int);
void            gimage_set_component_visible  (GimpImage*, ChannelType, int);
Layer*         gimage_pick_correlate_layer   (GimpImage*, int, int);
void            gimage_set_layer_mask_apply   (GimpImage*, int);
void            gimage_set_layer_mask_edit    (GimpImage*, Layer*, int);
void            gimage_set_layer_mask_show    (GimpImage*, int);
Layer*         gimage_raise_layer            (GimpImage*, Layer*);
Layer*         gimage_lower_layer            (GimpImage*, Layer*);
Layer*         gimage_merge_visible_layers   (GimpImage*, MergeType);
Layer*         gimage_flatten                (GimpImage*);
Layer*         gimage_merge_layers           (GimpImage*, GSList*, MergeType);
Layer*         gimage_add_layer              (GimpImage*, Layer*, int);
Layer*         gimage_remove_layer           (GimpImage*, Layer*);
LayerMask*     gimage_add_layer_mask         (GimpImage*, Layer*, LayerMask*);
Channel*       gimage_remove_layer_mask      (GimpImage*, Layer*, int);
Channel*       gimage_raise_channel          (GimpImage*, Channel*);
Channel*       gimage_lower_channel          (GimpImage*, Channel*);
Channel*       gimage_add_channel            (GimpImage*, Channel*, int);
Channel*       gimage_remove_channel         (GimpImage*, Channel*);
void            gimage_construct              (GimpImage*, int, int, int, int);
void            gimage_invalidate             (GimpImage*, int, int, int, int, int, int, int, int);
void            gimage_validate               (TileManager*, Tile*, int);
void            gimage_inflate                (GimpImage*);
void            gimage_deflate                (GimpImage*);


/*  Access functions */

int             gimage_is_flat                (GimpImage*);
int             gimage_is_empty               (GimpImage*);
GimpDrawable*  gimage_active_drawable        (GimpImage*);
int             gimage_base_type              (GimpImage*);
int             gimage_base_type_with_alpha   (GimpImage*);
char*          gimage_filename               (GimpImage*);
int             gimage_enable_undo            (GimpImage*);
int             gimage_disable_undo           (GimpImage*);
int             gimage_dirty                  (GimpImage*);
int             gimage_clean                  (GimpImage*);
void            gimage_clean_all              (GimpImage*);
Layer*         gimage_floating_sel           (GimpImage*);
unsigned char* gimage_cmap                   (GimpImage*);


/*  projection access functions */

TileManager*   gimage_projection             (GimpImage*);
int             gimage_projection_type        (GimpImage*);
int             gimage_projection_bytes       (GimpImage*);
int             gimage_projection_opacity     (GimpImage*);
void            gimage_projection_realloc     (GimpImage*);


/*  composite access functions */

TileManager*   gimage_composite              (GimpImage*);
int             gimage_composite_type         (GimpImage*);
int             gimage_composite_bytes        (GimpImage*);
TempBuf*       gimage_composite_preview      (GimpImage*, ChannelType, int, int);
int             gimage_preview_valid          (GimpImage*, ChannelType);
void            gimage_invalidate_preview     (GimpImage*);

void            gimage_invalidate_previews    (void);

/* from drawable.c*/
GimpImage*        drawable_gimage               (GimpDrawable*);
#endif
