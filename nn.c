/*
 * nn.c
 *
 * Multilayer perceptrons
 */

#include <stdio.h>
#include <math.h>
#include "common.h"

#ifdef NN_DEBUG
#include "vector.h"
#endif

#include "nn.h"
#include "mat/mat_math.h"
#include "mat/mat_io.h"

/* activation function */
#define perceptron_tf tanhf

static inline numeric _sq(numeric x)
{
	return x*x;
}

/* derivative of the activation function
 * d/dx(tanh(x)) = sech^2(x) = (1/cosh(x))^2 */
static inline numeric d_perceptron_tf(numeric x)
{
	return _sq(1/coshf(x));
}

const struct MLPLayer MLPLayer_INVALID = {MAT_INVALID_TXT, MAT_INVALID_TXT};

struct MLPLayer MLPLayer_create(int n_neurons, int n_inputs, int *ret_code)
{
	struct MLPLayer r;

	if (ret_code != NULL)
		*ret_code = -E_NOMEM;

	if(mat_valid(r.w = mat_create(n_neurons, n_inputs))) {
		if(mat_valid(r.w0 = mat_vcreate(n_neurons))) {
			if (ret_code != NULL)
				*ret_code = -E_OK;
		} else {
			mat_destroy(r.w);
		}
	}

	return r;
}

int MLPLayer_fwrite(struct MLPLayer l, FILE *f)
{
	return mat_fwrite(l.w, MLP_WRITE_OPTS, f) +
		mat_fwrite(l.w0, MLP_WRITE_OPTS, f);;
}

struct MLPLayer MLPLayer_fread(FILE *f)
{
	struct MLPLayer r = MLPLayer_INVALID;

	if (!(mat_valid(r.w = mat_fread(f))
	     && mat_valid(r.w0 = mat_fread(f))
	     && MLPLayer_n_neurons(r) == mat_length(r.w0))) {
		mat_destroy(r.w);
		mat_destroy(r.w0);
		r = MLPLayer_INVALID;
	}

	return r;
}

void MLPLayer_destroy(struct MLPLayer l)
{
	mat_destroy(l.w);
	mat_destroy(l.w0);
}

void MLPLayer_randFill(struct MLPLayer l)
{
	mat_randFill(l.w, 0.5);
	mat_randFill(l.w0, 0.5);
}

void MLPLayer_eval(struct MLPLayer *l, struct matrix vec, struct matrix dest)
{
	mat_FMA2(l->w, vec, l->w0, dest, perceptron_tf);
}

/* err y new_err pueden superponerse en la memoria */
void MLPLayer_backpropagate(struct MLPLayer *l, numeric mu, struct matrix v_in,
					struct matrix err, struct matrix new_err)
{
	struct matrix delta = mat_vcreate(MLPLayer_n_neurons(*l));

	/*we will perform the update :
	 * w(n+1) = w(n) + mu*err*f'(w*v_in)*v_out
	 */

	/* calculate the slope of the activation */
	mat_FMA2(l->w, v_in, l->w0, delta, d_perceptron_tf);
#ifdef NN_DIM_DEBUG
	printf("(%d, %d) * (%d, %d) + (%d, %d) =  (%d, %d)\n",
		l->w.row, l->w.col,
		v_in.row, v_in.col,
		l->w0.row, l->w0.col,
		delta.row, delta.col);
#endif
	mat_vMultiply(delta, err, delta);

	/* backpropagate the error */
	mat_Product(mat_vtransposed(delta), l->w, new_err);

	/* apply learning coefficient */
	mat_vScale(delta, mu, delta);

	/* update the values of w and w0*/
	mat_tensorFMA(delta, v_in, l->w, l->w);
	mat_vAdd(l->w0, delta, l->w0);

	mat_destroy(delta);
}

inline int MLP_n_outputs(struct MLP mlp)
{
	return MLPLayer_n_neurons(mlp.layers[mlp.n_layers - 1]);
}

void _destroy_layers(struct MLPLayer *layers, int n_layers)
{
	int i;

	if (layers != NULL) {
		for (i = 0; i < n_layers; i++)
			MLPLayer_destroy(layers[i]);
		free(layers);
	}
}

void MLP_destroy(struct MLP mlp)
{
	_destroy_layers(mlp.layers, mlp.n_layers);

	free(mlp.work_area_even);
	free(mlp.work_area_odd);
}

struct MLP MLP_create_from_layers(struct MLPLayer *layers, int n_layers,
								int *ret_code)
{
	struct MLP r;
	int i, max_wa_even_size = 0, max_wa_odd_size = 0, prev_output_size;
	int _ret_code = -E_OK;;

	r.n_layers = n_layers;
	r.layers = layers;
	r.work_area_even = NULL;
	r.work_area_odd = NULL;

	for (i = 0; i < r.n_layers; i++) {
		int output_size = MLPLayer_n_neurons(layers[i]);

		switch (i % 2) {
		case 0:
			if (output_size > max_wa_even_size)
				max_wa_even_size = output_size;
			break;
		case 1:
			if (output_size > max_wa_odd_size)
				max_wa_odd_size = output_size;
			break;
		}
		if (i != 0 && MLPLayer_n_inputs(layers[i]) != prev_output_size){
			_ret_code = -E_BADCFG;
			goto MLP_create_from_layers_end;
		}

		prev_output_size = output_size;
	}

	if (NMALLOC(r.work_area_even, max_wa_even_size) == NULL
	    || NMALLOC(r.work_area_odd, max_wa_odd_size) == NULL) {
		_ret_code = -E_NOMEM;
		goto MLP_create_from_layers_end;
	}

	r.work_area_larger = (max_wa_even_size > max_wa_odd_size)?
				r.work_area_even : r.work_area_odd;


MLP_create_from_layers_end:

	if (_ret_code < 0) {
		free(r.work_area_even);
		free(r.work_area_odd);
		r.work_area_even = NULL;
		r.work_area_odd = NULL;
	}

	if (ret_code != NULL)
		*ret_code = _ret_code;

	return r;
}

struct MLP MLP_create(const int *layer_sizes, int layer_sizes_n, int *ret_code)
{
	struct MLP r;
	struct MLPLayer *layers;
	int n_layers, _ret_code = -E_OK;

	n_layers = layer_sizes_n - 1;

	if (NMALLOC(layers, n_layers) != NULL){
		int i;

		for (i = 0; i < n_layers; i++) {
			layers[i] = MLPLayer_INVALID;
		}

		for (i = 0; i < n_layers; i++) {
			int l_status;

			layers[i] = MLPLayer_create(layer_sizes[i+1],
						layer_sizes[i], &l_status);

			if (l_status < 0) {
				_ret_code = l_status;
				goto MLP_create_end;
			}
			MLPLayer_randFill(layers[i]);
		}
	} else {
		_ret_code = -E_NOMEM;
	}

MLP_create_end:

	if (_ret_code < 0) {
		struct MLP _r = {0};

		_destroy_layers(layers, n_layers);

		r = _r;
	} else {
		r = MLP_create_from_layers(layers, n_layers, &_ret_code);
		if (_ret_code < 0)
			MLP_destroy(r);
	}

	if (ret_code != NULL)
		*ret_code = _ret_code;

	return r;
}

int MLP_fwrite(struct MLP mlp, FILE *f)
{
	int count = 0, i;

	count += fprintf(f, NN_LAYERS_TAG" %d\n", mlp.n_layers);
	for (i = 0; i < mlp.n_layers; i++) {
		count += MLPLayer_fwrite(mlp.layers[i], f);
	}

	return count;
}

struct MLP MLP_fread(FILE *f)
{
	struct MLP r;
	struct MLPLayer *layers;
	int code = -E_OK, n_layers;

	if (fscanf(f, NN_LAYERS_TAG" %d\n", &n_layers) == 1
	    && NMALLOC(layers, n_layers) != NULL) {
		int i;

		for (i = 0; i < n_layers; i++) {
			layers[i] = MLPLayer_INVALID;
		}

		for (i = 0; i < n_layers; i++) {
			if (!MLPLayer_valid(layers[i] = MLPLayer_fread(f))) {
				code = -E_OTHER;
				goto MLP_fread_end;
			}
		}

		r = MLP_create_from_layers(layers, n_layers, &code);
	} else {
		MLP_mark_invalid(r);
	}

MLP_fread_end:
	if (code == -E_OTHER) {
		_destroy_layers(layers, n_layers);
		MLP_mark_invalid(r);
	} else if (code < 0) {
		MLP_destroy(r);
		MLP_mark_invalid(r);
	}

	return r;
}

void MLP_destroy_train_space(struct MLP mlp, MLPTrainSpace ts)
{
	int i;

	for (i = 0; i < mlp.n_layers; i++) {
		mat_destroy(ts[i]);
	}

	free(ts);
}

MLPTrainSpace MLP_create_train_space(struct MLP mlp)
{
	MLPTrainSpace r;

	if (NMALLOC(r, mlp.n_layers)) {
		int i;

		for (i = 0; i < mlp.n_layers; i++) {
			r[i] = MAT_INVALID;
		}

		for (i = 0; i < mlp.n_layers; i++) {
			r[i] = mat_vcreate(MLPLayer_n_neurons(mlp.layers[i]));
			if (!mat_valid(r[i]))
				goto MLP_create_train_space_failed;
		}
	}

	return r;

MLP_create_train_space_failed:
	MLP_destroy_train_space(mlp, r);

	return NULL;
}

void MLP_eval(struct MLP mlp, struct matrix in, struct matrix out)
{
	int i;
	struct matrix layer_input, layer_output;

	for (i = 0; i < mlp.n_layers; i++) {
		if (i == 0)
			layer_input = in;
		else
			layer_input = layer_output;

		if (i == mlp.n_layers - 1)
			layer_output = out;
		else
			layer_output = a_to_vmatrix(
				(i % 2)? mlp.work_area_odd : mlp.work_area_even,
				MLPLayer_n_neurons(mlp.layers[i]));

		MLPLayer_eval(mlp.layers + i, layer_input, layer_output);
	}
}

void MLP_eval_update(struct MLP mlp, struct matrix in, struct matrix out,
				MLPTrainSpace outputs, numeric mu)
{
	int i;
	struct matrix layer_input, err;

	for (i = 0; i < mlp.n_layers; i++) {
		if (i == 0)
			layer_input = in;
		else
			layer_input = outputs[i - 1];

		MLPLayer_eval(mlp.layers + i, layer_input, outputs[i]);
	}

	for (i = mlp.n_layers - 1; i >= 0; i--) {
		if (i == mlp.n_layers - 1) {
			err = a_to_vmatrix( mlp.work_area_larger,
				MLPLayer_n_neurons(mlp.layers[i]));
			mat_vSubstract(out, outputs[i], err);
		}
		if (i == 0) {
			layer_input = in;
		} else {
			layer_input = outputs[i - 1];
		}
#ifdef NN_DIM_DEBUG
		printf("%d: ", i);
#endif
		MLPLayer_backpropagate(mlp.layers + i, mu, layer_input,
				a_to_vmatrix(mlp.work_area_larger,
						MLPLayer_n_neurons(mlp.layers[i])),
				a_to_vmatrix(mlp.work_area_larger,
						MLPLayer_n_inputs(mlp.layers[i])));
	}
}
/*
void MLP_train(struct MLP mlp, numeric *v_in, numeric *v_out, int n_vectors,
						int epochs, numeric mu)
{

}
*/

#ifdef NN_DEBUG

#define TRAIN_CYCLES (8000*45*4)
#define N_EVAL 1000

#define MU 0.002f

const int layer_sz[] = {1, 10, 10, 2};
const struct limit tlimit = {-10, 10};

int main(int argc, char **argv)
{
	int i;
	struct MLP mlp1;
	MLPTrainSpace ts;

	if (argc != 2) {
		fprintf(stderr, "specify r or w\n");
		return 1;
	}

	switch (argv[1][0]) {
	case 'w':
		mlp1 = MLP_create(layer_sz, ARSIZE(layer_sz), NULL);
		ts = MLP_create_train_space(mlp1);

		for (i = 0; i < TRAIN_CYCLES; i++) {
			numeric t = rand_f(tlimit);
			numeric xy[2];
			struct matrix in, out;

			//xy[0] = .05f*(t - sinf(t));
			//xy[1] = .5f*(1 - sinf(t)) - 0.5f;
			xy[0] = (t > 0)? -1: 1;
			xy[1] = (t > 0)? 1: -1;

			in = a_to_vmatrix(&t, 1);
			out = a_to_vmatrix(xy, 2);

			MLP_eval_update(mlp1, in, out, ts, MU);
		}

		MLP_destroy_train_space(mlp1, ts);
		fprintf(stderr, "%d bytes written\n", MLP_fwrite(mlp1, stdout));
		break;
	case 'r':
		mlp1 = MLP_fread(stdin);
		if (!MLP_valid(mlp1)) {
			fprintf(stderr, "wrong configuration\n");
			return 3;
		}

		for (i = 0; i < N_EVAL; i++) {
			numeric t = ((tlimit.max - tlimit.min)/N_EVAL)*i + tlimit.min;
			numeric xy[2];
			struct matrix in, out;

			in = a_to_vmatrix(&t, 1);
			out = a_to_vmatrix(xy, 2);

			MLP_eval(mlp1, in, out);
			printf("%g, %g, %g\n", t, xy[0], xy[1]);
		}

		break;
	default:
		fprintf(stderr, "command <<%s>> not understood\n", argv[1]);
		return 2;
	}

	MLP_destroy(mlp1);

	return 0;
}
#endif

