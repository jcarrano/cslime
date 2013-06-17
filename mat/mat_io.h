/*
 * mat_io.h
 *
 * Read / write matrices to disk
 */

#ifndef __MAT_IO_H__
#define __MAT_IO_H__

#include <stdio.h>
#include "mat.h"

#define MAT_USE_START 1
#define MAT_USE_END (1<<1)
#define MAT_USE_COMMAS (1<<2)

int mat_fwrite(struct matrix m, int options, FILE *f);
struct matrix mat_fread(FILE *f);

#endif /*__MAT_IO_H__*/
