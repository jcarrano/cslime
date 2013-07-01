/*
 * mat_math.c
 *
 * Matrix routines.
 * Juan I. Ubeira & Juan I. Carrano.
 */

#include <math.h>
#include "../common.h"
#include "mat.h"
#include <stdio.h>
#include <string.h>

#define NFMA fmaf

void mat_vScale(struct matrix vect, numeric k, struct matrix save)
{
    int i;
    for (i = 0; i < mat_length(vect); i++)
        mat_vset(save, k*mat_vget(vect, i), i);
}

void mat_vSubstract(struct matrix v1, struct matrix v2, struct matrix save)
{	/* V1 - V2 elemento a elemento */
    int i;
    for (i = 0; i < mat_length(v1); i++)
        mat_vset(save, mat_vget(v1, i) - mat_vget(v2, i), i);
    return;
}

void mat_vAdd(struct matrix v1, struct matrix v2, struct matrix save)
{  /* V1 + V2 elemento a elemento */
    int i;
    for (i = 0; i < mat_length(v1); i++)
        mat_vset(save, mat_vget(v1, i) + mat_vget(v2, i), i);
    return;
}

void mat_vMultiply(struct matrix v1, struct matrix v2, struct matrix save)
{  /* V1 * V2 elemento a elemento */
    int i;
    for (i = 0; i < mat_length(v1); i++)
        mat_vset(save, mat_vget(v1, i) * mat_vget(v2, i), i);
    return;
}

void mat_copy(struct matrix dest, struct matrix src)
{
	int N = mat_length(dest);

	memcpy(dest.M, src.M, N*sizeof(*dest.M));
}

void mat_randFill(struct matrix m, numeric a)
{
    int i;
    for (i = 0; i < mat_length(m); i++)
        mat_vset(m, ((numeric)rand()/(numeric)RAND_MAX - NUMSUFFIX(.5))*2*a, i);
    return;
}

struct matrix mat_getRow(struct matrix mat, int row)
{
    struct matrix aux = mat_vcreate(mat.col);
    int i;
    for (i = 0; i < mat.col; i++)
        aux.M[i] = mat_get(mat, row, i);

    return aux;
}

struct matrix mat_getCol(struct matrix mat, int col)
{
    struct matrix aux = mat_vcreate(mat.row);
    int i;
    for (i = 0; i < mat.row; i++)
        aux.M[i] = mat_get(mat, i, col);

    return aux;
}

void mat_setRow(struct matrix mat, struct matrix rowMatrix, int rowToSet)
{
    int i;
    for (i = 0; i < mat_length(rowMatrix); i++)
        mat_set(mat, mat_vget(rowMatrix, i), rowToSet, i);
}

void mat_setCol(struct matrix mat, struct matrix colMatrix, int colToSet)
{ /* colMatrix sigue siendo un vector lineal, que se trabaja con vget */
    int i;
    for (i = 0; i < mat_length(colMatrix); i++)
    {
        mat_set(mat, mat_vget(colMatrix, i), i, colToSet);
    }
}

void mat_setV(struct matrix dest, struct matrix src)
{
	mat_setCol(dest, src, 1);
}

inline void mat_Product2(struct matrix m1, struct matrix m2, struct matrix save,
							numeric (*f)(numeric))
{
    int j, i, k;
    for (j = 0; j < m1.row; j++) {
	for (i = 0; i < m2.col; i++) {
	    numeric x = 0;

	    for (k = 0; k < m1.col; k++)
		x += mat_get(m1, j, k)*mat_get(m2, k, i);

	    mat_set(save,f(x), j, i);
	}
    }
}

static numeric _noop(numeric x)
{
    return x;
}

void mat_Product(struct matrix m1, struct matrix m2, struct matrix save)
{
    mat_Product2(m1, m2, save, _noop);
}

void mat_FMA2(struct matrix m1, struct matrix m2, struct matrix m3,
				    struct matrix save, numeric (*f)(numeric))
{
    int j, i, k;
    for (j = 0; j < m1.row; j++) {
	for (i = 0; i < m2.col; i++) {
	    numeric x = 0;

	    for (k = 0; k < m1.col; k++)
		x += NFMA(mat_get(m1, j, k), mat_get(m2, k, i),
							    mat_get(m3, j, i));

	    mat_set(save,f(x), j, i);
	}
    }
}

void mat_FMA(struct matrix m1, struct matrix m2, struct matrix m3,
							    struct matrix save)
{
    mat_FMA2(m1, m2, m3, save, _noop);
}

int mat_all_leas(struct matrix mat, numeric v)
{
	int i, N = mat_length(mat);
	for (i = 0; i < N; i++) {
		if (numabs(mat_vget(mat, i)) > v)
			return 0;
	}
	return 1;
}

int mat_any_nan(struct matrix mat)
{
	int i, N = mat_length(mat);
	for (i = 0; i < N; i++) {
		if (isnan(mat_vget(mat, i)))
			return 1;
	}
	return 0;
}

void mat_tensorProduct(struct matrix v1, struct matrix v2, struct matrix save)
{
    int j, i;

    for (j = 0; j < mat_length(v1); j++) {
	for (i = 0; i < mat_length(v2); i++) {
	    mat_set(save, mat_vget(v1, j)*mat_vget(v2, i), j, i);
	}
    }
}

void mat_tensorFMA(struct matrix v1, struct matrix v2, struct matrix m,
							struct matrix save)
{
    int j, i;

    for (j = 0; j < mat_length(v1); j++) {
	numeric a0 = mat_vget(v1, j);
	for (i = 0; i < mat_length(v2); i++) {
	    mat_set(save, NFMA(a0, mat_vget(v2, i), mat_get(m, j, i)),
		    j, i);
	}
    }
}

numeric mat_dotProduct(struct matrix v1, struct matrix v2)
{
    numeric result = 0;
    int i;

    for(i = 0; i < mat_length(v1); i++)
        result += mat_vget(v1, i)*mat_vget(v2, i);

    return result;
}
