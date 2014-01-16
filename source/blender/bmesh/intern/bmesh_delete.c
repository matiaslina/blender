/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2007 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Geoffrey Bantle.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/bmesh/intern/bmesh_delete.c
 *  \ingroup bmesh
 *
 * BM remove functions.
 */

#include "MEM_guardedalloc.h"

#include "BLI_utildefines.h"

#include "bmesh.h"
#include "intern/bmesh_private.h"

/**
 * Called by operators to remove elements that they have marked for
 * removal.
 */
static void bmo_remove_tagged_faces(BMesh *bm, const short oflag)
{
	BMFace *f, *f_next;
	BMIter iter;

	BM_ITER_MESH_MUTABLE (f, f_next, &iter, bm, BM_FACES_OF_MESH) {
		if (BMO_elem_flag_test(bm, f, oflag)) {
			BM_face_kill(bm, f);
		}
	}
}

static void bmo_remove_tagged_edges(BMesh *bm, const short oflag)
{
	BMEdge *e, *e_next;
	BMIter iter;

	BM_ITER_MESH_MUTABLE (e, e_next, &iter, bm, BM_EDGES_OF_MESH) {
		if (BMO_elem_flag_test(bm, e, oflag)) {
			BM_edge_kill(bm, e);
		}
	}
}

static void bmo_remove_tagged_verts(BMesh *bm, const short oflag)
{
	BMVert *v, *v_next;
	BMIter iter;

	BM_ITER_MESH_MUTABLE (v, v_next, &iter, bm, BM_VERTS_OF_MESH) {
		if (BMO_elem_flag_test(bm, v, oflag)) {
			BM_vert_kill(bm, v);
		}
	}
}

#define DEL_WIREVERT	(1 << 10)

/**
 * \warning oflag applies to different types in some contexts,
 * not just the type being removed.
 *
 * \warning take care, uses operator flag DEL_WIREVERT
 */
void BMO_remove_tagged_context(BMesh *bm, const short oflag, const int type)
{
	BMVert *v;
	BMEdge *e;
	BMFace *f;

	BMIter viter;
	BMIter eiter;
	BMIter fiter;

	switch (type) {
		case DEL_VERTS:
		{
			bmo_remove_tagged_verts(bm, oflag);

			break;
		}
		case DEL_EDGES:
		{
			/* flush down to vert */
			BM_ITER_MESH (e, &eiter, bm, BM_EDGES_OF_MESH) {
				if (BMO_elem_flag_test(bm, e, oflag)) {
					BMO_elem_flag_enable(bm, e->v1, oflag);
					BMO_elem_flag_enable(bm, e->v2, oflag);
				}
			}
			bmo_remove_tagged_edges(bm, oflag);
			/* remove loose vertice */
			BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
				if (BMO_elem_flag_test(bm, v, oflag) && (!(v->e)))
					BMO_elem_flag_enable(bm, v, DEL_WIREVERT);
			}
			bmo_remove_tagged_verts(bm, DEL_WIREVERT);

			break;
		}
		case DEL_EDGESFACES:
		{
			bmo_remove_tagged_edges(bm, oflag);

			break;
		}
		case DEL_ONLYFACES:
		{
			bmo_remove_tagged_faces(bm, oflag);

			break;
		}
		case DEL_ONLYTAGGED:
		{
			bmo_remove_tagged_faces(bm, oflag);
			bmo_remove_tagged_edges(bm, oflag);
			bmo_remove_tagged_verts(bm, oflag);

			break;
		}
		case DEL_FACES:
		{
			/* go through and mark all edges and all verts of all faces for delete */
			BM_ITER_MESH (f, &fiter, bm, BM_FACES_OF_MESH) {
				if (BMO_elem_flag_test(bm, f, oflag)) {
					BMLoop *l_first = BM_FACE_FIRST_LOOP(f);
					BMLoop *l_iter;

					l_iter = l_first;
					do {
						BMO_elem_flag_enable(bm, l_iter->v, oflag);
						BMO_elem_flag_enable(bm, l_iter->e, oflag);
					} while ((l_iter = l_iter->next) != l_first);
				}
			}
			/* now go through and mark all remaining faces all edges for keeping */
			BM_ITER_MESH (f, &fiter, bm, BM_FACES_OF_MESH) {
				if (!BMO_elem_flag_test(bm, f, oflag)) {
					BMLoop *l_first = BM_FACE_FIRST_LOOP(f);
					BMLoop *l_iter;

					l_iter = l_first;
					do {
						BMO_elem_flag_disable(bm, l_iter->v, oflag);
						BMO_elem_flag_disable(bm, l_iter->e, oflag);
					} while ((l_iter = l_iter->next) != l_first);
				}
			}
			/* also mark all the vertices of remaining edges for keeping */
			BM_ITER_MESH (e, &eiter, bm, BM_EDGES_OF_MESH) {
				if (!BMO_elem_flag_test(bm, e, oflag)) {
					BMO_elem_flag_disable(bm, e->v1, oflag);
					BMO_elem_flag_disable(bm, e->v2, oflag);
				}
			}
			/* now delete marked face */
			bmo_remove_tagged_faces(bm, oflag);
			/* delete marked edge */
			bmo_remove_tagged_edges(bm, oflag);
			/* remove loose vertice */
			bmo_remove_tagged_verts(bm, oflag);

			break;
		}
	}
}