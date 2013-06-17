/*
 * mat.c
 *
 * Matrix routines.
 * Juan I. Ubeira & Juan I. Carrano.
 */

#include <string.h>
#include <math.h>
#include "../common.h"
#include "mat.h"

struct matrix mat_create(int row, int col)
{
	/* Crea matriz*/
	struct matrix m;
	int N = row*col;

	if (NMALLOC(m.M, N) != NULL) {
		m.row = row;
		m.col = col;
	}

	return m;
}

void mat_fill(struct matrix m, numeric v)
{
	int N = m.row*m.col;
	int i;

	for (i = 0; i < N; i++)
		mat_vset(m, v, i);
}

struct matrix mat_create0(int row, int col)
{
	/* Crea matriz e inicializa con 0 */
	struct matrix m;

	if (mat_valid(m = mat_create(row, col)))
		mat_fill(m, 0);

	return m;
}

struct matrix a_to_vmatrix(numeric *v, int l)
{
	struct matrix nm;

	nm.row = l;
	nm.col = 1;
	nm.M = v;

	return nm;
}

struct matrix mat_vcreate(int n)
{
	/* crea un vector, que no tiene "dirección" */
	return mat_create(n, 1);
}

struct matrix mat_vcreate0(int n)
{
	/* crea un vector e inicializa en 0*/
	struct matrix vec;

	if (mat_valid(vec = mat_create(n, 1)))
		mat_fill(vec, 0);

	return vec;
}

int mat_length(struct matrix m)
{
	return m.row*m.col;
}

int mat_valid(struct matrix m)
{
	return m.M != NULL;
}

void mat_destroy(struct matrix m)
{
	/* no destruyas 2 veces la misma matriz */
	free(m.M);
}

struct matrix mat_clone(struct matrix m)
{
	struct matrix m2;

	m2 = mat_create(m.row, m.col);
	if (mat_valid(m2)) {
		memcpy(m2.M, m.M, m.row*m.col*sizeof(*m.M));
	}
	return m2;
}

numeric mat_get(struct matrix m, int row, int col)
{
	return m.M[row * m.col + col];
}

numeric mat_vget(struct matrix m, int n)
{
	/* indice lineal */
	return m.M[n];
}

void mat_set(struct matrix m, numeric x, int row, int col)
{
	m.M[row * m.col + col] = x;
}

void mat_vset(struct matrix m, numeric x, int n)
{
	/* indice lineal */
	m.M[n] = x;
}

struct matrix mat_vtransposed(struct matrix v)
{
	struct matrix r;

	r.M = v.M;
	r.col = v.row;
	r.row = v.col;

	return r;
}

struct mat_loc mat_absmax(struct matrix m, int row0, int row1, int col0, int col1)
{
	int r, c;
	struct mat_loc lmax;

	lmax.row = row0;
	lmax.col = col0;
	lmax.v = 0;

	for (r = row0; r < row1; r++) {
		for (c = col0; c < col1; c++) {
			numeric x;
			x = mat_get(m, r, c);
			if (numabs(x) > numabs(lmax.v)) {
				lmax.v = x;
				lmax.row = r;
				lmax.col = c;
			}
		}
	}

	return lmax;
}

struct mat_loc mat_wabsmax(struct matrix m, int row0, int row1, int col0,
						int col1, int el, int mode)
{
	int i, imin, imax;
	struct mat_loc lmax;

	imin = (mode == MAT_ROWMODE)? row0 : col0;
	imax = (mode == MAT_ROWMODE)? row1 : col1;

	lmax.v = 0;
	lmax.row = row0;
	lmax.col = col0;

	for (i = imin; i < imax; i++) {
		numeric x, y;
		struct mat_loc mfactor;

		if (mode == MAT_ROWMODE) {
			mfactor = mat_absmax(m, i, i+1, col0, col1);
			x = mat_get(m, i, el);
		} else {
			mfactor = mat_absmax(m, row0, row1, i, i+1);
			x = mat_get(m, el, i);
		}

		y = x / mfactor.v;
		if (numabs(y) > numabs(lmax.v)) {
			lmax.v = x;
			lmax.row = (mode == MAT_ROWMODE)? i : el;
			lmax.col = (mode == MAT_ROWMODE)? el : i;
		}
	}

	return lmax;
}
