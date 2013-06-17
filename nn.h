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

#define MLP_WRITE_OPTS (MAT_USE_START)

struct MLP {
	int n_layers; /*numero REAL de capas (la capa de entrada no cuenta) */
	struct MLPLayer *layers;
	numeric *work_area_even;
	numeric *work_area_odd;
	numeric *work_area_larger;
};

#define MLP_n_inputs(mlp) MLPLayer_n_inputs(((mlp).layers[0]))

#endif /* __NN_H__ */
