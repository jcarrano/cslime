/*
 * nn.h
 *
 * Multilayer perceptrons with backpropagation
 */

#ifndef __NN_H__
#define __NN_H__

#include "mat/mat.h"

struct MLPLayer {
	struct matrix w; /* Weights */
	struct matrix w0; /* Bias inputs */
};

#define MLPLayer_n_neurons(l) ((l).w.row)
#define MLPLayer_n_inputs(l) ((l).w.col)
#define MLPLayer_valid(l) (mat_valid((l).w))

#define MLP_WRITE_OPTS (MAT_USE_START)

struct MLP {
	int n_layers; /*numero REAL de capas (la capa de entrada no cuenta) */
	struct MLPLayer *layers;
	numeric *work_area_even;
	numeric *work_area_odd;
	numeric *work_area_larger;
};

typedef struct matrix *MLPTrainSpace;

#define MLP_n_inputs(mlp) MLPLayer_n_inputs(((mlp).layers[0]))
int MLP_n_outputs(struct MLP mlp);

#define MLP_valid(mlp) ((mlp).layers != NULL)
#define MLP_mark_invalid(mlp) ((mlp).layers = NULL)

#define NN_LAYERS_TAG "layers"

struct MLP MLP_create(const int *layer_sizes, int layer_sizes_n, int *ret_code);
void MLP_destroy(struct MLP mlp);

MLPTrainSpace MLP_create_train_space(struct MLP mlp);
void MLP_destroy_train_space(struct MLP mlp, MLPTrainSpace ts);
#define MLP_ts_valid(ts) ((ts) != NULL)

struct MLP MLP_fread(FILE *f);
int MLP_fwrite(struct MLP mlp, FILE *f);

void MLP_eval(struct MLP mlp, struct matrix in, struct matrix out);
void MLP_eval_update(struct MLP mlp, struct matrix in, struct matrix out,
				MLPTrainSpace outputs, numeric mu);

#endif /* __NN_H__ */
