/*
 * mat_math.h
 *
 * Matrix routines.
 * Juan I. Ubeira & Juan I. Carrano.
 */

#ifndef MATAGREGADOS_H_INCLUDED
#define MATAGREGADOS_H_INCLUDED

#include "mat.h"

/* Warning: The functions decared here DO NOT check for consistent dimensions.
 * 	If you mess up you will get a segfault (if you are lucky) or incorrect
 * 	results.
 */

void mat_copy(struct matrix dest, struct matrix src);

void mat_randFill(struct matrix m, numeric a);

struct matrix mat_getRow(struct matrix mat, int row);
void mat_setRow(struct matrix mat, struct matrix rowMatrix, int rowToSet);

struct matrix mat_getCol(struct matrix mat, int col);
void mat_setCol(struct matrix mat, struct matrix colMatrix, int colToSet);

void mat_setV(struct matrix dest, struct matrix src);

/* Matrix operations */

void mat_Product(struct matrix m1, struct matrix m2, struct matrix save);
	/* Performs matrix product */
void mat_Product2(struct matrix m1, struct matrix m2, struct matrix save,
							numeric (*f)(numeric));
	/* Like mat_product except each element of the result is passed
	 * through the scalar function 'f'
	 */

void mat_FMA(struct matrix m1, struct matrix m2, struct matrix m3,
							struct matrix save);
	/* Fused Multiply and Add.
	 * Equivalent to
	 * 	mat_product(m1, m2, save);
	 * 	mat_vAdd(save, m3, save);
	 * but calculated using c99's 'fma()' function if available
	 */
void mat_FMA2(struct matrix m1, struct matrix m2, struct matrix m3,
				struct matrix save, numeric (*f)(numeric));
	/* Like mat_fma except each element of the result is passed
	 * through the scalar function 'f'
	 */

/* Vector operations */

void mat_tensorProduct(struct matrix v1, struct matrix v2, struct matrix save);
	/* Tensor product between two vectors (also known as outer product)
	 * equal to v1*v2'
	 */

/* Mixed operations*/

void mat_tensorFMA(struct matrix v1, struct matrix v2, struct matrix m,
							struct matrix save);
	/* Do tensor product between v1 and v2 and add m.
	 * Uses 'fma()' if available
	 * 'save' can be the same as 'm'.
	 */

/* Universal operations
 * The following functions operate in vector or matrices in an element to element
 * fashion. Therefore it is perfectly safe if the "save" vector is one of the
 * operands. In that case the operation is performed in place
 */
void mat_vScale(struct matrix vect, numeric k, struct matrix save);
	/* Multiply each element by a scalar */

void mat_vSubstract(struct matrix v1, struct matrix v2, struct matrix save);
void mat_vAdd(struct matrix v1, struct matrix v2, struct matrix save);
void mat_vMultiply(struct matrix v1, struct matrix v2, struct matrix save);
	/*Element by element substraction, addition and multiplication*/

numeric mat_dotProduct(struct matrix v1, struct matrix v2);
	/* Dot product between vectors. For matrices it returns the Frobenius
	 * product (the sum of the element-by-element product
	 */

/*
 * Tests
 */

int mat_all_leas(struct matrix mat, numeric v);
	/* MATrix ALL elements Less or Equal in Absolute value to Scalar
	 * devuelve 1 si todos los elementos son menores o iguales a v
	 */

int mat_any_nan(struct matrix mat);
	/* returns true if any element is NaN */

#endif /* MATAGREGADOS_H_INCLUDED */
