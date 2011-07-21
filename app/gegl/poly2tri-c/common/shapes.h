/*
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Include guard
#ifndef SHAPES_H
#define SHAPES_H

#include <stddef.h>
#include <assert.h>
#include <math.h>
#include "poly2tri-private.h"
#include "cutils.h"

struct Point_
{
  double x, y;
  /// The edges this point constitutes an upper ending point
  P2tEdgePtrArray edge_list;
};

void p2t_point_init_dd (P2tPoint* THIS, double x, double y);
P2tPoint* p2t_point_new_dd (double x, double y);
void p2t_point_init (P2tPoint* THIS);
P2tPoint* p2t_point_new ();

void p2t_point_destroy (P2tPoint* THIS);
void
p2t_point_free (P2tPoint* THIS);

// Represents a simple polygon's edge

struct Edge_
{
  P2tPoint* p, *q;
};

void p2t_edge_init (P2tEdge* THIS, P2tPoint* p1, P2tPoint* p2);
P2tEdge* p2t_edge_new (P2tPoint* p1, P2tPoint* p2);

void p2t_edge_destroy (P2tEdge* THIS);
void
p2t_edge_free (P2tEdge* THIS);

// Triangle-based data structures are know to have better performance than quad-edge structures
// See: J. Shewchuk, "Triangle: Engineering a 2D Quality Mesh Generator and Delaunay Triangulator"
//      "Triangulations in CGAL"

struct Triangle_
{
  /// Flags to determine if an edge is a Constrained edge
  gboolean constrained_edge[3];
  /// Flags to determine if an edge is a Delauney edge
  gboolean delaunay_edge[3];

  //private:

  /// Triangle points
  P2tPoint * points_[3];
  /// Neighbor list
  struct Triangle_ * neighbors_[3];

  /// Has this triangle been marked as an interior triangle?
  gboolean interior_;
};

P2tTriangle* p2t_triangle_new (P2tPoint* a, P2tPoint* b, P2tPoint* c);
void p2t_triangle_init (P2tTriangle* THIS, P2tPoint* a, P2tPoint* b, P2tPoint* c);
P2tPoint* p2t_triangle_get_point (P2tTriangle* THIS, const int index);
P2tPoint* p2t_triangle_point_cw (P2tTriangle* THIS, P2tPoint* point);
P2tPoint* p2t_triangle_point_ccw (P2tTriangle* THIS, P2tPoint* point);
P2tPoint* p2t_triangle_opposite_point (P2tTriangle* THIS, P2tTriangle* t, P2tPoint* p);

P2tTriangle* p2t_triangle_get_neighbor (P2tTriangle* THIS, const int index);
void p2t_triangle_mark_neighbor_pt_pt_tr (P2tTriangle* THIS, P2tPoint* p1, P2tPoint* p2, P2tTriangle* t);
void p2t_triangle_mark_neighbor_tr (P2tTriangle* THIS, P2tTriangle *t);

void p2t_triangle_mark_constrained_edge_i (P2tTriangle* THIS, const int index);
void p2t_triangle_mark_constrained_edge_ed (P2tTriangle* THIS, P2tEdge* edge);
void p2t_triangle_mark_constrained_edge_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q);

int p2t_triangle_index (P2tTriangle* THIS, const P2tPoint* p);
int p2t_triangle_edge_index (P2tTriangle* THIS, const P2tPoint* p1, const P2tPoint* p2);

P2tTriangle* p2t_triangle_neighbor_cw (P2tTriangle* THIS, P2tPoint* point);
P2tTriangle* p2t_triangle_neighbor_ccw (P2tTriangle* THIS, P2tPoint* point);
gboolean p2t_triangle_get_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_get_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p);
void p2t_triangle_set_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean ce);
void p2t_triangle_set_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean ce);
gboolean p2t_triangle_get_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_get_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p);
void p2t_triangle_set_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean e);
void p2t_triangle_set_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean e);

gboolean p2t_triangle_contains_pt (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_contains_ed (P2tTriangle* THIS, const P2tEdge* e);
gboolean p2t_triangle_contains_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q);
void p2t_triangle_legalize_pt (P2tTriangle* THIS, P2tPoint* point);
void p2t_triangle_legalize_pt_pt (P2tTriangle* THIS, P2tPoint* opoint, P2tPoint* npoint);
/**
 * Clears all references to all other triangles and points
 */
void p2t_triangle_clear (P2tTriangle* THIS);
void p2t_triangle_clear_neighbor_tr (P2tTriangle* THIS, P2tTriangle *triangle);
void p2t_triangle_clear_neighbors (P2tTriangle* THIS);
void p2t_triangle_clear_delunay_edges (P2tTriangle* THIS);

gboolean p2t_triangle_is_interior (P2tTriangle* THIS);
void p2t_triangle_is_interior_b (P2tTriangle* THIS, gboolean b);

P2tTriangle* p2t_triangle_neighbor_across (P2tTriangle* THIS, P2tPoint* opoint);

void p2t_triangle_debug_print (P2tTriangle* THIS);

gint p2t_point_cmp (gconstpointer a, gconstpointer b);

//  gboolean operator == (const Point& a, const Point& b);
gboolean p2t_point_equals (const P2tPoint* a, const P2tPoint* b);

#endif


