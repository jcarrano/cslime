/*
 * cslime.h
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

#ifndef _CSLIME_H_
#define _CSLIME_H_

#include "common.h"
#include "vector.h"

#define SIMSTEP 10
#define OVERSAMPLING 16
#define SAMPLE_FACTOR ((OVERSAMPLING*10.0)/SIMSTEP)

#define GAME_AREA_H .3
#define PLAYER_AREA_W .363
#define PLAYER_AREA_H GAME_AREA_H
#define NET_W .004
#define NET_H .038
#define AREA2_STARTX (PLAYER_AREA_W + NET_W)
#define GAME_AREA_W (2*PLAYER_AREA_W + NET_W)
#define AVATAR_H .035 /*.036*/
#define AVATAR_W (AVATAR_H*2)
#define AVATAR_MASS 1
#define BALL_MASS 1
/*#define BALL_R .00875*/
#define BALL_R .0104
#define START_POS_RATIO 0.3925

#define START_BALL_Y .1

#define BALL_G (.00013/(SAMPLE_FACTOR*SAMPLE_FACTOR))
static const struct limit vlimitx = {(.000/SAMPLE_FACTOR), (.0065/SAMPLE_FACTOR)};
static const struct limit vlimity = {(.000/SAMPLE_FACTOR), (.00515/SAMPLE_FACTOR)};
#define VISCOUS_DRAG (60)
#define PLAYER_G (BALL_G*2)
#define AVATAR_VY (.006/SAMPLE_FACTOR)
#define AVATAR_VX (((PLAYER_AREA_W)*0.01)/SAMPLE_FACTOR)
#define AVATAR_AX (.0002/(SAMPLE_FACTOR*SAMPLE_FACTOR))
#define FIRE_SPEEDUP 2
#define BOUNCICITY 2
#define BOUNCICITY_X 2
#define FLOOR_HIT_TOL (GAME_AREA_H/200)

#define N_PLAYERS 2
#define DEF_START_POINTS 5

struct free_body {
	struct r_vector pos; /* top-left corner */
	struct r_vector vel, acc;
	struct r_vector box; /*size of the bounding box */
	float mass;
	float drag;
};

struct player {
	struct free_body body;
	bool on_fire;
	int points;
};

struct ball {
	struct free_body body;
};

struct game {
	struct player p[N_PLAYERS];
	struct ball b;
};

struct game_result {
	int scorer_player;
	int has_to_start;
	int set_end;
	int game_end;
};

struct commands {
	struct pcontrol {
		bool u, d, l, r;
		bool aux;
	} player[2];
	bool aux;
};

#define BODY_CENTER(b) (r_sum((b).pos, r_scale((b).box, .5)))

void game_reset(struct game *g, int turn);
struct game game_init(int start_points, int first_turn);
struct game_result run_game(struct game *g, struct commands comm);

static const struct r_vector net_poly[] = {
	{PLAYER_AREA_W, PLAYER_AREA_H - NET_H},
	{PLAYER_AREA_W + NET_W, PLAYER_AREA_H - NET_H},
	{PLAYER_AREA_W + NET_W, PLAYER_AREA_H},
	{PLAYER_AREA_W, PLAYER_AREA_H}
};
static const struct r_vector world_poly[] = {
	{0, GAME_AREA_H},
	{GAME_AREA_W, GAME_AREA_H},
	{GAME_AREA_W, 0},
	{0, 0},
};

#endif /* _CSLIME_H_ */
