/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpedit_pdb.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "gimp.h"


/**
 * SECTION: gimpedit
 * @title: gimpedit
 * @short_description: Edit menu functions (cut, copy, paste, clear, etc.)
 *
 * Edit menu functions (cut, copy, paste, clear, etc.)
 **/


/**
 * gimp_edit_cut:
 * @drawable_ID: The drawable to cut from.
 *
 * Cut from the specified drawable.
 *
 * If there is a selection in the image, then the area specified by the
 * selection is cut from the specified drawable and placed in an
 * internal GIMP edit buffer. It can subsequently be retrieved using
 * the gimp_edit_paste() command. If there is no selection, then the
 * specified drawable will be removed and its contents stored in the
 * internal GIMP edit buffer. This procedure will fail if the selected
 * area lies completely outside the bounds of the current drawable and
 * there is nothing to copy from.
 *
 * Returns: TRUE if the cut was successful, FALSE if there was nothing
 * to copy from.
 **/
gboolean
gimp_edit_cut (gint32 drawable_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean non_empty = FALSE;

  return_vals = gimp_run_procedure ("gimp-edit-cut",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    non_empty = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return non_empty;
}

/**
 * gimp_edit_copy:
 * @drawable_ID: The drawable to copy from.
 *
 * Copy from the specified drawable.
 *
 * If there is a selection in the image, then the area specified by the
 * selection is copied from the specified drawable and placed in an
 * internal GIMP edit buffer. It can subsequently be retrieved using
 * the gimp_edit_paste() command. If there is no selection, then the
 * specified drawable's contents will be stored in the internal GIMP
 * edit buffer. This procedure will fail if the selected area lies
 * completely outside the bounds of the current drawable and there is
 * nothing to copy from.
 *
 * Returns: TRUE if the cut was successful, FALSE if there was nothing
 * to copy from.
 **/
gboolean
gimp_edit_copy (gint32 drawable_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean non_empty = FALSE;

  return_vals = gimp_run_procedure ("gimp-edit-copy",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    non_empty = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return non_empty;
}

/**
 * gimp_edit_copy_visible:
 * @image_ID: The image to copy from.
 *
 * Copy from the projection.
 *
 * If there is a selection in the image, then the area specified by the
 * selection is copied from the projection and placed in an internal
 * GIMP edit buffer. It can subsequently be retrieved using the
 * gimp_edit_paste() command. If there is no selection, then the
 * projection's contents will be stored in the internal GIMP edit
 * buffer.
 *
 * Returns: TRUE if the copy was successful.
 *
 * Since: 2.2
 **/
gboolean
gimp_edit_copy_visible (gint32 image_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean non_empty = FALSE;

  return_vals = gimp_run_procedure ("gimp-edit-copy-visible",
                                    &nreturn_vals,
                                    GIMP_PDB_IMAGE, image_ID,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    non_empty = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return non_empty;
}

/**
 * gimp_edit_paste:
 * @drawable_ID: The drawable to paste to.
 * @paste_into: Clear selection, or paste behind it?
 *
 * Paste buffer to the specified drawable.
 *
 * This procedure pastes a copy of the internal GIMP edit buffer to the
 * specified drawable. The GIMP edit buffer will be empty unless a call
 * was previously made to either gimp_edit_cut() or gimp_edit_copy().
 * The \"paste_into\" option specifies whether to clear the current
 * image selection, or to paste the buffer \"behind\" the selection.
 * This allows the selection to act as a mask for the pasted buffer.
 * Anywhere that the selection mask is non-zero, the pasted buffer will
 * show through. The pasted buffer will be a new layer in the image
 * which is designated as the image floating selection. If the image
 * has a floating selection at the time of pasting, the old floating
 * selection will be anchored to its drawable before the new floating
 * selection is added. This procedure returns the new floating layer.
 * The resulting floating selection will already be attached to the
 * specified drawable, and a subsequent call to floating_sel_attach is
 * not needed.
 *
 * Returns: The new floating selection.
 **/
gint32
gimp_edit_paste (gint32   drawable_ID,
                 gboolean paste_into)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint32 floating_sel_ID = -1;

  return_vals = gimp_run_procedure ("gimp-edit-paste",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_INT32, paste_into,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    floating_sel_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return floating_sel_ID;
}

/**
 * gimp_edit_paste_as_new_image:
 *
 * Paste buffer to a new image.
 *
 * This procedure pastes a copy of the internal GIMP edit buffer to a
 * new image. The GIMP edit buffer will be empty unless a call was
 * previously made to either gimp_edit_cut() or gimp_edit_copy(). This
 * procedure returns the new image or -1 if the edit buffer was empty.
 *
 * Returns: The new image.
 *
 * Since: 2.10
 **/
gint32
gimp_edit_paste_as_new_image (void)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint32 image_ID = -1;

  return_vals = gimp_run_procedure ("gimp-edit-paste-as-new-image",
                                    &nreturn_vals,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

/**
 * gimp_edit_named_cut:
 * @drawable_ID: The drawable to cut from.
 * @buffer_name: The name of the buffer to create.
 *
 * Cut into a named buffer.
 *
 * This procedure works like gimp_edit_cut(), but additionally stores
 * the cut buffer into a named buffer that will stay available for
 * later pasting, regardless of any intermediate copy or cut
 * operations.
 *
 * Returns: The real name given to the buffer, or NULL if the cut
 * failed.
 *
 * Since: 2.4
 **/
gchar *
gimp_edit_named_cut (gint32       drawable_ID,
                     const gchar *buffer_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *real_name = NULL;

  return_vals = gimp_run_procedure ("gimp-edit-named-cut",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_STRING, buffer_name,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    real_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return real_name;
}

/**
 * gimp_edit_named_copy:
 * @drawable_ID: The drawable to copy from.
 * @buffer_name: The name of the buffer to create.
 *
 * Copy into a named buffer.
 *
 * This procedure works like gimp_edit_copy(), but additionally stores
 * the copied buffer into a named buffer that will stay available for
 * later pasting, regardless of any intermediate copy or cut
 * operations.
 *
 * Returns: The real name given to the buffer, or NULL if the copy
 * failed.
 *
 * Since: 2.4
 **/
gchar *
gimp_edit_named_copy (gint32       drawable_ID,
                      const gchar *buffer_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *real_name = NULL;

  return_vals = gimp_run_procedure ("gimp-edit-named-copy",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_STRING, buffer_name,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    real_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return real_name;
}

/**
 * gimp_edit_named_copy_visible:
 * @image_ID: The image to copy from.
 * @buffer_name: The name of the buffer to create.
 *
 * Copy from the projection into a named buffer.
 *
 * This procedure works like gimp_edit_copy_visible(), but additionally
 * stores the copied buffer into a named buffer that will stay
 * available for later pasting, regardless of any intermediate copy or
 * cut operations.
 *
 * Returns: The real name given to the buffer, or NULL if the copy
 * failed.
 *
 * Since: 2.4
 **/
gchar *
gimp_edit_named_copy_visible (gint32       image_ID,
                              const gchar *buffer_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *real_name = NULL;

  return_vals = gimp_run_procedure ("gimp-edit-named-copy-visible",
                                    &nreturn_vals,
                                    GIMP_PDB_IMAGE, image_ID,
                                    GIMP_PDB_STRING, buffer_name,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    real_name = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return real_name;
}

/**
 * gimp_edit_named_paste:
 * @drawable_ID: The drawable to paste to.
 * @buffer_name: The name of the buffer to paste.
 * @paste_into: Clear selection, or paste behind it?
 *
 * Paste named buffer to the specified drawable.
 *
 * This procedure works like gimp_edit_paste() but pastes a named
 * buffer instead of the global buffer.
 *
 * Returns: The new floating selection.
 *
 * Since: 2.4
 **/
gint32
gimp_edit_named_paste (gint32       drawable_ID,
                       const gchar *buffer_name,
                       gboolean     paste_into)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint32 floating_sel_ID = -1;

  return_vals = gimp_run_procedure ("gimp-edit-named-paste",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_STRING, buffer_name,
                                    GIMP_PDB_INT32, paste_into,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    floating_sel_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return floating_sel_ID;
}

/**
 * gimp_edit_named_paste_as_new_image:
 * @buffer_name: The name of the buffer to paste.
 *
 * Paste named buffer to a new image.
 *
 * This procedure works like gimp_edit_paste_as_new_image() but pastes
 * a named buffer instead of the global buffer.
 *
 * Returns: The new image.
 *
 * Since: 2.10
 **/
gint32
gimp_edit_named_paste_as_new_image (const gchar *buffer_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint32 image_ID = -1;

  return_vals = gimp_run_procedure ("gimp-edit-named-paste-as-new-image",
                                    &nreturn_vals,
                                    GIMP_PDB_STRING, buffer_name,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

/**
 * gimp_edit_clear:
 * @drawable_ID: The drawable to clear from.
 *
 * Clear selected area of drawable.
 *
 * This procedure clears the specified drawable. If the drawable has an
 * alpha channel, the cleared pixels will become transparent. If the
 * drawable does not have an alpha channel, cleared pixels will be set
 * to the background color. This procedure only affects regions within
 * a selection if there is a selection active.
 *
 * Deprecated: Use gimp_drawable_edit_clear() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_edit_clear (gint32 drawable_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-clear",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_fill:
 * @drawable_ID: The drawable to fill to.
 * @fill_type: The type of fill.
 *
 * Fill selected area of drawable.
 *
 * This procedure fills the specified drawable with the fill mode. If
 * the fill mode is foreground, the current foreground color is used.
 * If the fill mode is background, the current background color is
 * used. Other fill modes should not be used. This procedure only
 * affects regions within a selection if there is a selection active.
 * If you want to fill the whole drawable, regardless of the selection,
 * use gimp_drawable_fill().
 *
 * Deprecated: Use gimp_drawable_edit_fill() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_edit_fill (gint32       drawable_ID,
                GimpFillType fill_type)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-fill",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_INT32, fill_type,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_bucket_fill:
 * @drawable_ID: The affected drawable.
 * @fill_mode: The type of fill.
 * @paint_mode: The paint application mode.
 * @opacity: The opacity of the final bucket fill.
 * @threshold: The threshold determines how extensive the seed fill will be. It's value is specified in terms of intensity levels. This parameter is only valid when there is no selection in the specified image.
 * @sample_merged: Use the composite image, not the drawable.
 * @x: The x coordinate of this bucket fill's application. This parameter is only valid when there is no selection in the specified image.
 * @y: The y coordinate of this bucket fill's application. This parameter is only valid when there is no selection in the specified image.
 *
 * Fill the area specified either by the current selection if there is
 * one, or by a seed fill starting at the specified coordinates.
 *
 * This tool requires information on the paint application mode, and
 * the fill mode, which can either be in the foreground color, or in
 * the currently active pattern. If there is no selection, a seed fill
 * is executed at the specified coordinates and extends outward in
 * keeping with the threshold parameter. If there is a selection in the
 * target image, the threshold, sample merged, x, and y arguments are
 * unused. If the sample_merged parameter is TRUE, the data of the
 * composite image will be used instead of that for the specified
 * drawable. This is equivalent to sampling for colors after merging
 * all visible layers. In the case of merged sampling, the x and y
 * coordinates are relative to the image's origin; otherwise, they are
 * relative to the drawable's origin.
 *
 * Deprecated: Use gimp_drawable_edit_bucket_fill() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_edit_bucket_fill (gint32             drawable_ID,
                       GimpBucketFillMode fill_mode,
                       GimpLayerMode      paint_mode,
                       gdouble            opacity,
                       gdouble            threshold,
                       gboolean           sample_merged,
                       gdouble            x,
                       gdouble            y)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-bucket-fill",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_INT32, fill_mode,
                                    GIMP_PDB_INT32, paint_mode,
                                    GIMP_PDB_FLOAT, opacity,
                                    GIMP_PDB_FLOAT, threshold,
                                    GIMP_PDB_INT32, sample_merged,
                                    GIMP_PDB_FLOAT, x,
                                    GIMP_PDB_FLOAT, y,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_bucket_fill_full:
 * @drawable_ID: The affected drawable.
 * @fill_mode: The type of fill.
 * @paint_mode: The paint application mode.
 * @opacity: The opacity of the final bucket fill.
 * @threshold: The threshold determines how extensive the seed fill will be. It's value is specified in terms of intensity levels. This parameter is only valid when there is no selection in the specified image.
 * @sample_merged: Use the composite image, not the drawable.
 * @fill_transparent: Whether to consider transparent pixels for filling. If TRUE, transparency is considered as a unique fillable color.
 * @select_criterion: The criterion used to determine color similarity. SELECT_CRITERION_COMPOSITE is the standard choice.
 * @x: The x coordinate of this bucket fill's application. This parameter is only valid when there is no selection in the specified image.
 * @y: The y coordinate of this bucket fill's application. This parameter is only valid when there is no selection in the specified image.
 *
 * Fill the area specified either by the current selection if there is
 * one, or by a seed fill starting at the specified coordinates.
 *
 * This tool requires information on the paint application mode, and
 * the fill mode, which can either be in the foreground color, or in
 * the currently active pattern. If there is no selection, a seed fill
 * is executed at the specified coordinates and extends outward in
 * keeping with the threshold parameter. If there is a selection in the
 * target image, the threshold, sample merged, x, and y arguments are
 * unused. If the sample_merged parameter is TRUE, the data of the
 * composite image will be used instead of that for the specified
 * drawable. This is equivalent to sampling for colors after merging
 * all visible layers. In the case of merged sampling, the x and y
 * coordinates are relative to the image's origin; otherwise, they are
 * relative to the drawable's origin.
 *
 * Deprecated: Use gimp_drawable_edit_bucket_fill() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_edit_bucket_fill_full (gint32              drawable_ID,
                            GimpBucketFillMode  fill_mode,
                            GimpLayerMode       paint_mode,
                            gdouble             opacity,
                            gdouble             threshold,
                            gboolean            sample_merged,
                            gboolean            fill_transparent,
                            GimpSelectCriterion select_criterion,
                            gdouble             x,
                            gdouble             y)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-bucket-fill-full",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_INT32, fill_mode,
                                    GIMP_PDB_INT32, paint_mode,
                                    GIMP_PDB_FLOAT, opacity,
                                    GIMP_PDB_FLOAT, threshold,
                                    GIMP_PDB_INT32, sample_merged,
                                    GIMP_PDB_INT32, fill_transparent,
                                    GIMP_PDB_INT32, select_criterion,
                                    GIMP_PDB_FLOAT, x,
                                    GIMP_PDB_FLOAT, y,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_blend:
 * @drawable_ID: The affected drawable.
 * @blend_mode: The type of blend.
 * @paint_mode: The paint application mode.
 * @gradient_type: The type of gradient.
 * @opacity: The opacity of the final blend.
 * @offset: Offset relates to the starting and ending coordinates specified for the blend. This parameter is mode dependent.
 * @repeat: Repeat mode.
 * @reverse: Use the reverse gradient.
 * @supersample: Do adaptive supersampling.
 * @max_depth: Maximum recursion levels for supersampling.
 * @threshold: Supersampling threshold.
 * @dither: Use dithering to reduce banding.
 * @x1: The x coordinate of this blend's starting point.
 * @y1: The y coordinate of this blend's starting point.
 * @x2: The x coordinate of this blend's ending point.
 * @y2: The y coordinate of this blend's ending point.
 *
 * Blend between the starting and ending coordinates with the specified
 * blend mode and gradient type.
 *
 * This tool requires information on the paint application mode, the
 * blend mode, and the gradient type. It creates the specified variety
 * of blend using the starting and ending coordinates as defined for
 * each gradient type. For shapeburst gradient types, the context's
 * distance metric is also relevant and can be updated with
 * gimp_context_set_distance_metric().
 *
 * Deprecated: Use gimp_drawable_edit_gradient_fill() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_edit_blend (gint32           drawable_ID,
                 GimpBlendMode    blend_mode,
                 GimpLayerMode    paint_mode,
                 GimpGradientType gradient_type,
                 gdouble          opacity,
                 gdouble          offset,
                 GimpRepeatMode   repeat,
                 gboolean         reverse,
                 gboolean         supersample,
                 gint             max_depth,
                 gdouble          threshold,
                 gboolean         dither,
                 gdouble          x1,
                 gdouble          y1,
                 gdouble          x2,
                 gdouble          y2)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-blend",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_INT32, blend_mode,
                                    GIMP_PDB_INT32, paint_mode,
                                    GIMP_PDB_INT32, gradient_type,
                                    GIMP_PDB_FLOAT, opacity,
                                    GIMP_PDB_FLOAT, offset,
                                    GIMP_PDB_INT32, repeat,
                                    GIMP_PDB_INT32, reverse,
                                    GIMP_PDB_INT32, supersample,
                                    GIMP_PDB_INT32, max_depth,
                                    GIMP_PDB_FLOAT, threshold,
                                    GIMP_PDB_INT32, dither,
                                    GIMP_PDB_FLOAT, x1,
                                    GIMP_PDB_FLOAT, y1,
                                    GIMP_PDB_FLOAT, x2,
                                    GIMP_PDB_FLOAT, y2,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_stroke:
 * @drawable_ID: The drawable to stroke to.
 *
 * Stroke the current selection
 *
 * This procedure strokes the current selection, painting along the
 * selection boundary with the active brush and foreground color. The
 * paint is applied to the specified drawable regardless of the active
 * selection.
 *
 * Deprecated: Use gimp_drawable_edit_stroke_selection() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_edit_stroke (gint32 drawable_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-stroke",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_edit_stroke_vectors:
 * @drawable_ID: The drawable to stroke to.
 * @vectors_ID: The vectors object.
 *
 * Stroke the specified vectors object
 *
 * This procedure strokes the specified vectors object, painting along
 * the path with the active brush and foreground color.
 *
 * Deprecated: Use gimp_drawable_edit_stroke_item() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_edit_stroke_vectors (gint32 drawable_ID,
                          gint32 vectors_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp-edit-stroke-vectors",
                                    &nreturn_vals,
                                    GIMP_PDB_DRAWABLE, drawable_ID,
                                    GIMP_PDB_VECTORS, vectors_ID,
                                    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}
