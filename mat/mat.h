/*
 * mat.h
 *
 * Matrix routines.
 * Juan I. Ubeira & Juan I. Carrano.
 */

#ifndef _MAT_H_
#define _MAT_H_

#include <stdlib.h>

typedef float numeric;
#define numabs fabsf

enum {MAT_ROWMODE, MAT_COLMODE};

/* row: cantidad de rows
   col: cantidad de cols
*/
struct matrix {
	int row, col;
	numeric *M;
};

struct mat_loc {
	int row, col;
	numeric v;
};

#define MAT_INVALID_TXT {0, 0, NULL}
static const struct matrix MAT_INVALID = MAT_INVALID_TXT;

extern struct matrix mat_create(int row, int col);
extern struct matrix mat_vcreate(int n);

extern struct matrix a_to_vmatrix(numeric *v, int l);
#define A_TO_VMATRIX(a) a_to_vmatrix((a), ARSIZE(a))

int mat_length(struct matrix m);
extern int mat_valid(struct matrix m);
extern void mat_destroy(struct matrix m);
extern struct matrix mat_clone(struct matrix m);
extern numeric mat_get(struct matrix m, int row, int col);
extern numeric mat_vget(struct matrix m, int n);
extern void mat_set(struct matrix m, numeric x, int row, int col);
extern void mat_vset(struct matrix m, numeric x, int n);

extern struct matrix mat_vtransposed(struct matrix v);

extern struct mat_loc mat_absmax(struct matrix m, int row0, int row1,
							int col0, int col1);

extern struct mat_loc mat_wabsmax(struct matrix m, int row0, int row1, int col0,
						int col1, int el, int mode);

#endif /* _MAT_H_ */
