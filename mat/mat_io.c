/*
 * mat_io.c
 *
 * Read / write matrices to disk
 */

#include <stdio.h>

#ifdef MAT_IO_TEST
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mat_math.h"
#endif /* MAT_IO_TEST*/

#include "mat_io.h"

#define SIZE_TAG "size"
#define END_TAG "matrix-end"

int mat_fwrite(struct matrix m, int options, FILE *f)
{
	int count = 0, i, j;

	if (options & MAT_USE_START)
		count += fprintf(f, SIZE_TAG" %d %d\n", m.row, m.col);

	for (j = 0; j < m.row; j++) {
		for (i = 0; i < m.col; i++) {
			count += fprintf(f, "%.12g", mat_get(m, j, i));
			if (i != m.col - 1) {
				if (options & MAT_USE_COMMAS) {
					fputc(',', f);
					count++;
				}
				fputc(' ', f);
				count++;
			}
		}
		fputc('\n', f);
		count++;
	}

	if (options & MAT_USE_END)
		count += fprintf(f, END_TAG"\n");

	return count;
}

struct matrix mat_fread(FILE *f)
{
	struct matrix m = MAT_INVALID_TXT;
	int rows, cols;
	int i, j;

	if (fscanf(f, SIZE_TAG" %d %d\n", &rows, &cols) != 2)
		goto mat_fread_end;

	if (!mat_valid(m = mat_create(rows, cols)))
		goto mat_fread_end;

	for (j = 0; j < m.row; j++) {
		int found_newline_ok = 0;

		for (i = 0; i < m.col; i++) {
			numeric a;
			int c;

			if (fscanf(f, "%f", &a) != 1)
				goto mat_fread_fail;
			mat_set(m, a, j, i);
			switch (c = fgetc(f)) {
			case ',':
				if (fgetc(f) != ' ')
					goto mat_fread_fail;
			case ' ':
				break;
			case '\n':
				if (i == m.col-1) {
					found_newline_ok = 1;
				}
				goto mat_fread_read_line;
			default:
				goto mat_fread_fail;
			}
		}
		/* if we reach this point without a \n it is an error */
mat_fread_read_line:
		if (!found_newline_ok)
			goto mat_fread_fail;
	}

	if (0) {
mat_fread_fail:
		mat_destroy(m);
		m = MAT_INVALID;
	}
mat_fread_end:
	return m;
}

#ifdef MAT_IO_TEST

#define MAX_DIM 100
#define NO_FILE "-"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
	} else {
		FILE *f;
		int use_ext_file, k, r, c;
		struct matrix m;
		struct matrix m2;

		if (strcmp(argv[1], NO_FILE) == 0) {
			use_ext_file = 0;
			f = stdout;
		} else {
			use_ext_file = 1;
			f = fopen(argv[1], "w+");
		}

		srand(time(NULL));
		r = rand() % MAX_DIM;
		c = rand() % MAX_DIM;

		m = mat_create(r, c);
		mat_randFill(m, 1000);
		fprintf(stderr, "writing %d x %d\n", r, c);
		k = mat_fwrite(m, MAT_USE_START|MAT_USE_COMMAS, f);
		if (use_ext_file)
			fclose(f);
		fprintf(stderr, "wrote %d bytes???\n", k);

		if (use_ext_file) {
			fprintf(stderr, "reading\n");
			f = fopen(argv[1], "r");
			m2 = mat_fread(f);
			fclose(f);
			mat_fwrite(m2, MAT_USE_START|MAT_USE_COMMAS, stdout);
			mat_destroy(m2);
		}

		mat_destroy(m);
	}

	return 0;
}
#endif /* MAT_IO_TEST*/
