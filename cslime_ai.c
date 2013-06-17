/*
 * cslime_ai.c
 *
 * Copyright 2013:
 * 	Juan I Carrano <juan@carrano.com.ar>
 * 	Marcelo Lerendegui <marcelolerendegui@gmail.com>
 * 	Juan Manuel Oxoby <jm@oxoby.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <math.h>

#ifdef AI_TRAIN_NN
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#endif

#include "cslime_ai.h"
#include "vector.h"
#include "nn.h"

/* Greedy player : tries to make the best move using only current information.
 * 		Some randomness is required to serve and to avoid infinite loops.
 */

static float kinematic_solve_t(float x0, float x1, float v0, float acc)
{
	/* x1 = x0 + v0*t + 0.5*acc*t^2 */
	float delta;
	float a = acc/2;
	float b = v0;
	float c = x0 - x1;

	delta =  b*b - 4*a*c; /*determinant */
	if (delta >= 0) {
		float t1, t2;

		t1 = (-b - sqrt(delta)) / (2*a);
		t2 = (-b + sqrt(delta)) / (2*a);

		return fmaxf(t1, t2);
	} else {
		return NAN;
	}
}

static float kinematic_solve_x(float x0, float v0, float acc, float delta_t)
{
	return x0 + v0*delta_t + 0.5 * acc * delta_t * delta_t;
}

struct pcontrol greedy_player(struct game g, int player_number, bool aggressive)
{
	struct player me;
	struct r_vector my_center, b_center;
	float t_ground, delta_x;
	struct pcontrol r = {0};

	me = g.p[player_number];

	my_center = r_sum(me.body.pos, r_scale(me.body.box, .5));
	b_center = r_sum(g.b.body.pos, r_scale(g.b.body.box, .5));
/* delta_x = x0 + v*t + .5*a*t^2
 */
	t_ground = kinematic_solve_t(g.b.body.pos.y + g.b.body.box.y,
			my_center.y, g.b.body.vel.y, g.b.body.acc.y);
	if (isnanf(t_ground)) {
		delta_x = 0;
	} else if (t_ground < 0) {
		delta_x = g.b.body.pos.x - my_center.x;
	} else {
		float x_ground;
		x_ground = kinematic_solve_x(b_center.x, g.b.body.vel.x,
						g.b.body.acc.x, t_ground);
		if (x_ground > GAME_AREA_W)
			x_ground = 2*GAME_AREA_W - x_ground;
		else if (x_ground < 0)
			x_ground = -x_ground;

		delta_x = x_ground - my_center.x;
	}

	if (delta_x != 0) {
		if (delta_x > 0)
			r.r = 1;
		else
			r.l = 1;
	}

	if (aggressive) {
		struct p_vector incidence;
		incidence = r_to_p(r_subs(b_center, my_center));

		if ( incidence.value < me.body.box.y*2
		    && t_ground > 0
		    && fmaxf(fabsf(delta_x) - me.body.box.x/2, 0)/t_ground > AVATAR_VX) {
			r.u = 1;
		} else if (incidence.value < me.body.box.y*(1.0 + 1.0/3)) {
			if (incidence.titha < -M_PI_4 && incidence.titha > -3*M_PI_4)
				r.u = (rand()%8 == 0);
			if (fabsf(b_center.x - my_center.x) < me.body.box.x/4) {
				bool a = rand()%2;
				r.l = a;
				r.r = !a;
			}
		}
	}

	return r;
}

/* Backpropagation neural-network player */

enum {BP_INPUT_PX, BP_INPUT_PY, BP_INPUT_BX, BP_INPUT_BY, BP_INPUT_BVX,
	BP_INPUT_BVY, BP_N_INPUTS};

enum {BP_OUTPUT_DIRECTION, BP_OUTPUT_JUMP, BP_N_OUTPUTS};

struct MLP neural_bp_player_fread(FILE *f)
{
	struct MLP r;
	r = MLP_fread(f);
	if (MLP_valid(r)) {
		if (MLP_n_inputs(r) != BP_N_INPUTS
		   || MLP_n_outputs(r) != BP_N_OUTPUTS) {
			MLP_destroy(r);
			MLP_mark_invalid(r);
		   }
	}

	return r;
}

#define BP_LOGIC_LEVEL (.8)
#define BP_MOVE_RIGHT BP_LOGIC_LEVEL
#define BP_MOVE_LEFT (-BP_MOVE_RIGHT)
#define BP_NO_MOVE 0

static void _bp_player_load_inputs(struct game g, int player_number,
						numeric inputs[BP_N_INPUTS])
{
	inputs[BP_INPUT_PX] = g.p[player_number].body.pos.x;
	inputs[BP_INPUT_PY] = g.p[player_number].body.pos.y;
	inputs[BP_INPUT_BX] = g.b.body.pos.x;
	inputs[BP_INPUT_BY] = g.b.body.pos.y;
	inputs[BP_INPUT_BVX] = g.b.body.vel.x;
	inputs[BP_INPUT_BVY] = g.b.body.vel.y;
}

static void _bp_player_load_outputs(struct pcontrol ctlr,
						numeric outputs[BP_N_OUTPUTS])
{
	int moving = (ctlr.l != ctlr.r);
	outputs[BP_OUTPUT_DIRECTION] = moving?
			(ctlr.r? BP_MOVE_RIGHT : BP_MOVE_LEFT) : BP_NO_MOVE;
	outputs[BP_OUTPUT_JUMP] = ctlr.u? BP_LOGIC_LEVEL: -BP_LOGIC_LEVEL;
}

static struct pcontrol _bp_player_read_outputs(numeric outputs[BP_N_OUTPUTS])
{
	struct pcontrol r;

	r.aux = 0;
	r.d = 0;
	r.u = (outputs[BP_OUTPUT_JUMP] > 0.0f);

	/* This should be ">" instead of "<", but it doesn't work */
	if (outputs[BP_OUTPUT_DIRECTION] < 0.0f) {
		r.r = 1;
		r.l = 0;
	} else {
		r.l = 1;
		r.r = 0;
	}

	return r;
}

struct pcontrol neural_bp_player(struct game g, int player_number,
							struct MLP brain)
{
	numeric inputs[BP_N_INPUTS];
	numeric outputs[BP_N_OUTPUTS];

	_bp_player_load_inputs(g, player_number, inputs);
	MLP_eval(brain, A_TO_VMATRIX(inputs), A_TO_VMATRIX(outputs));

	return _bp_player_read_outputs(outputs);
}

void bp_player_train_step(struct game g, int player_number,
			struct pcontrol ctrl_out, numeric mu, struct MLP brain,
			MLPTrainSpace train_space)
{
	numeric inputs[BP_N_INPUTS];
	numeric outputs[BP_N_OUTPUTS];

	_bp_player_load_inputs(g, player_number, inputs);
	_bp_player_load_outputs(ctrl_out, outputs);

	MLP_eval_update(brain, A_TO_VMATRIX(inputs), A_TO_VMATRIX(outputs),
						train_space, mu);
}

#ifdef AI_TRAIN_NN

#define TRAIN_DECIMATION 1
#define MU .002

static bool running = 1;
static const int bp_topology[] = {BP_N_INPUTS, 12, BP_N_OUTPUTS};

static void _stop_training(int s)
{
	running = 0;
}

int main(int argc, char *argv[])
{
	struct game g;
	struct commands comm;
	struct MLP brain;
	MLPTrainSpace ts;
	int updates = 0, code = 0;

	srand(time(NULL));
	g  = game_init(DEF_START_POINTS, rand()%2);
	comm.aux = 0;

	signal(SIGINT, _stop_training);

	brain = MLP_create(bp_topology, ARSIZE(bp_topology), &code);
	if (code < 0)
		goto ai_train_fail_brain;

	ts = MLP_create_train_space(brain);
	if (!MLP_ts_valid(ts)) {
		code = -E_NOMEM;
		goto ai_train_fail_ts;
	}

	fprintf(stderr, "Start training\n");

	while (running) {
		struct game_result gr;

		comm.player[1] = greedy_player(g , 1, 1);
		comm.player[0] = greedy_player(g , 0, 1);
		if (!(rand() % TRAIN_DECIMATION)) {
			int pn;

			updates++;
			pn = 1;

			bp_player_train_step(g, pn, comm.player[pn], MU, brain,
									ts);
		}

		gr = run_game(&g, comm);

		if (gr.game_end)
			g = game_init(DEF_START_POINTS, rand()%2);
		else if (gr.set_end)
			game_reset(&g, gr.has_to_start);
	}

	fprintf(stderr, "Stopped training: %d updates\n", updates);

	{
		int k;
		k = MLP_fwrite(brain, stdout);
		fprintf(stderr, "wrote %d bytes\n", k);
	}

	MLP_destroy_train_space(brain, ts);
ai_train_fail_ts:
	MLP_destroy(brain);

ai_train_fail_brain:
	if (code < 0)
		fprintf(stderr, "fatal error\n");

	return -code;
}
#endif /*AI_TRAIN_NN*/
