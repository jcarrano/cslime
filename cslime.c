/*
 * cslime.c
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

#include <stdio.h>
#include <math.h>
#include "vector.h"
#include "common.h"
#include "cslime.h"


#define CONSERVATION 1
#define CONSERVATION_WALL .9
#define TRANSMISSION_DOWN -0.3
#define TRANSMISSION -0.3

static const struct player def_player = {
	.body = {.pos = {0, 0}, .vel = {0, 0}, .acc = {0, PLAYER_G},
		.box = {AVATAR_W, AVATAR_H}, .mass = AVATAR_MASS, .drag = 0},
	.on_fire = 0, .points = 0
};

static const struct ball def_ball = {
	.body = {	.pos = {0, 0},
				.vel = {.0, .0},
				.acc = {.0, BALL_G},
				.box = {BALL_R*2, BALL_R*2},
				.mass = BALL_MASS,
				.drag = 0
			}
};

static const struct r_vector Default_p0_pos = {
       	     	    PLAYER_AREA_W * START_POS_RATIO - AVATAR_W/2,
					    GAME_AREA_H - AVATAR_H*2};
static const struct r_vector Default_p1_pos = {
       	            PLAYER_AREA_W * (1 - START_POS_RATIO) + AREA2_STARTX - AVATAR_W/2,
						GAME_AREA_H - AVATAR_H*2};


static const struct limit ZoneLimX[N_PLAYERS] = {{0, PLAYER_AREA_W},
				{AREA2_STARTX, AREA2_STARTX + PLAYER_AREA_W}};
static const struct limit LimX = {0, GAME_AREA_W};
static const struct limit LimY = {0, GAME_AREA_H};

static const struct free_body Net = {.pos = {PLAYER_AREA_W, PLAYER_AREA_H - NET_H},
				.box = {NET_W, NET_H}};

void game_reset(struct game *g, int turn)
{
	g->p[0].body.pos = Default_p0_pos;
	g->p[1].body.pos = Default_p1_pos;
	g->b.body.pos.x = g->p[turn].body.pos.x +
				(g->p[turn].body.box.x - g->b.body.box.x)/2;
	g->b.body.pos.y = START_BALL_Y;
	g->b.body.vel = r_zero;
}

struct game game_init(int start_points, int first_turn)
{
	struct game g;
	int i;

	for (i = 0; i < N_PLAYERS; i++) {
		g.p[i] = def_player;
		g.p[i].points = start_points;
	}
	g.b = def_ball;

	game_reset(&g, first_turn);

	return g;
}

enum {REBOUND_OFF, REBOUND_ON};

struct kinetic {
	struct r_vector pos, vel;
};

/* Physics simulation:
	1- 'kinetic_step': Propose a new state
		proposed_state <- kinetic_step(body)
	2- For every collision:
	 	given the proposed state, and the current state, generate a new
	 	proposition, by accepting, modifying, or reverting.
	 	proposed_state <- Collision_n(body, proposed_state, etc..)
	3- Accept the proposition
		body <- proposed_state
*/
static struct kinetic kinetic_step(struct free_body b)
{
	struct kinetic new_k;

	new_k.pos = r_sum(b.pos, b.vel);
	new_k.vel = r_subs(r_sum(b.vel, b.acc),
			r_scale(r_scale(b.vel, r_abs2(b.vel)), b.drag));

	return new_k;
}

static struct kinetic oblique_collision(
		struct free_body b, struct kinetic proposed, struct r_vector extra_vel,
		struct r_vector normal, float conservation, float v_transmission)
{
	struct kinetic new_k;

	if (r_dot(normal, r_subs(proposed.vel, extra_vel)) < 0) {
		new_k.vel = r_sum(	r_subs(	r_scale(proposed.vel, 1),
						r_scale(normal,
							(1+conservation)*r_dot(proposed.vel, normal)
						)
					),
					r_scale(normal,
						(1 + v_transmission)*r_dot(extra_vel, normal)
					)
				);
		/*new_k.pos = r_sum(b.pos, new_k.vel);*/
		/*new_k.pos = r_sum(b.pos, extra_vel);*/
		/*new_k.pos = r_sum(r_sum(b.pos, extra_vel), new_k.vel);*/
		/*new_k.pos = r_sum(r_sum(proposed.pos, extra_vel), new_k.vel);*/
		/*new_k.pos = r_sum(r_sum(b.pos, extra_vel),
					r_scale(normal,r_dot(extra_vel, normal)));*/
		/*new_k.pos = r_sum(b.pos, r_scale(normal, r_dot(extra_vel, normal)));*/
		/* new_k.pos = b.pos;*/
		{
			struct r_vector tangent = r_normal(normal);

			new_k.pos = r_sum(b.pos,
					r_sum(r_scale(tangent, r_dot(proposed.vel, tangent)),
						r_scale(normal, r_dot(extra_vel, normal))
					));
		}
	} else {
		new_k = proposed;
	}

	return new_k;
}

static struct kinetic original_collision(
		struct free_body b, struct kinetic proposed, struct free_body p,
		float bouncicity)
{
 	struct kinetic new_k;
	struct r_vector p_center = r_sum(p.pos, r_make(p.box.x/2, p.box.y));
	struct r_vector b_center = r_sum(proposed.pos,r_make(b.box.x/2, b.box.y/2));
	struct r_vector delta_p = r_subs(b_center, p_center);

        float dist = r_abs(delta_p);
	/*if (r_dot(delta_p, r_subs(proposed.vel, p.vel)) < 0) {*/
	if (dist < (p.box.x + b.box.y/2) && delta_p.y < 0) {
		struct r_vector delta_v = r_subs(proposed.vel, p.vel);
		float bounce_coeff;
                bounce_coeff = (delta_v.x * delta_p.x * BOUNCICITY_X
				+ delta_v.y * delta_p.y)/dist;
		new_k.pos = r_sum(p_center, r_scale(delta_p,
	 		    		 (p.box.x/2 + b.box.y/2)/dist));
		new_k.pos = r_subs(new_k.pos, r_make(b.box.x/2, b.box.y/2));
		new_k.vel = proposed.vel;
		if (bounce_coeff <= 0) {
			struct r_vector delta_p2 = delta_p;
			delta_p2.x *= BOUNCICITY_X;
 			new_k.vel = r_subs(r_sum(new_k.vel, p.vel),
				r_scale(delta_p2, bouncicity*bounce_coeff/dist));
			new_k.vel = r_absclip(new_k.vel, vlimitx, vlimity);
		}
	} else {
		new_k = proposed;
	}

	return new_k;
}

static struct kinetic world_limit_collision(struct free_body b,
			struct kinetic proposed, struct limit limx,
						struct limit limy, int rebound)
{
	struct kinetic new_k;
	struct r_vector normal = {0,0};

	if ((proposed.pos.x + b.box.x) > limx.max) {
		normal.x += -1;
	} else if (proposed.pos.x < limx.min) {
		normal.x += 1;
	}

	if ((proposed.pos.y + b.box.y) > limy.max)  {
		normal.y += -1;
	} else if (proposed.pos.y < limy.min) {
		normal.y += 1;
	}

	new_k = proposed;
	if (normal.x)
		new_k = oblique_collision(b, new_k, r_zero, r_make(normal.x, 0),
					rebound? CONSERVATION_WALL: 0, 0);
	if (normal.y)
		new_k = oblique_collision(b, new_k, r_zero, r_make(0, normal.y),
					rebound? CONSERVATION_WALL: 0, 0);

	return new_k;
}


/* return true if 'angle' is contained in the section definen by sweeping
 * 	from edgeangles.min in the positive direction up to edgeangles.max
 * 	all angle must be in the in interval [-pi pi]
 */
static bool angle_in_range(float angle, struct limit edgeangles)
{
	return 	  (edgeangles.min < edgeangles.max
			&& (angle > edgeangles.min && angle < edgeangles.max))
		|| (edgeangles.min > edgeangles.max
			&& (angle > edgeangles.min || angle < edgeangles.max));
}

static struct kinetic ball_edge_collision(
		struct ball b, struct kinetic proposed, struct kinetic edge,
		struct limit edgeangles, float conservation, float v_transmission)
{
	struct r_vector b_center;
	struct p_vector incidence;
	struct kinetic new_k;

	b_center = r_make(proposed.pos.x + b.body.box.x / 2,
					proposed.pos.y + b.body.box.y/2);

	incidence = r_to_p(r_subs(b_center, edge.pos));

	if (incidence.value < b.body.box.y / 2
			&& !angle_in_range(incidence.titha, edgeangles)) {
		struct r_vector normal = p_to_r(p_make(1, incidence.titha));
		new_k = oblique_collision(b.body, proposed, edge.vel, normal,
					conservation, v_transmission);
	} else {
		new_k = proposed;
	}

	return new_k;
}

static bool point_segment_near(struct r_vector p, struct r_vector v1,
					struct r_vector v2, float h)
{
	float cos_a, sin_a, d;
	struct r_vector hyp, w1, w2, p2;

	hyp = r_subs(v1, v2);
	d = r_abs(hyp);
	cos_a = hyp.x / d;
	sin_a = hyp.y / d;

	w1.x = cos_a * v1.x + sin_a * v1.y;
	w1.y = -sin_a * v1.x + cos_a * v1.y;

	w2.x = cos_a * v2.x + sin_a * v2.y;
	w2.y = -sin_a * v2.x + cos_a * v2.y;

	p2.x = cos_a * p.x + sin_a * p.y;
	p2.y = -sin_a * p.x + cos_a * p.y;

	return (p2.x < w1.x) && (p2.x > w2.x) && (p2.y <= w2.y + h) && (p2.y >= w2.y);
}

static struct kinetic ball_poly_collision(struct ball b, struct kinetic proposed,
		const struct r_vector *edges, int n_edges, struct r_vector extra_vel,
		float conservation, float v_transmission)
{
	struct kinetic new_k;
	int i;

	new_k = proposed;
	for (i = 0; i < n_edges; i++) {
		int i_next;

		i_next = (i + 1)%n_edges;
		if (point_segment_near(r_sum(proposed.pos, r_scale(b.body.box, .5)),
				   edges[i], edges[i_next], b.body.box.y/2)) {
			struct r_vector normal = r_unit(r_normal(
						r_subs(edges[i], edges[i_next])));
			new_k = oblique_collision(b.body, new_k, extra_vel, normal,
					conservation, v_transmission);
		} else {
			int i_prev = (i == 0)? n_edges - 1 : i - 1;
			struct limit l;
			struct kinetic edge;

			edge.pos = edges[i];
			edge.vel = extra_vel;
			l.min = r_to_p(r_subs(edges[i], edges[i_prev])).titha;
			l.min = r_to_p(r_subs(edges[i_next], edges[i])).titha;

			new_k = ball_edge_collision(b, new_k, edge,
						l, conservation, v_transmission);
		}
	}

	return new_k;
}

static struct kinetic ball_player_collision(struct ball b, struct kinetic k, struct player p)
{
	struct kinetic new_k;
	struct r_vector b_center;
	struct r_vector p_center;
	struct p_vector incidence;
	float min_dist = b.body.box.y/2 + p.body.box.y;

	b_center = r_make(k.pos.x + b.body.box.x / 2,
					k.pos.y + b.body.box.y/2);
	p_center = r_make(p.body.pos.x + p.body.box.x / 2,
					p.body.pos.y + p.body.box.y);

	incidence = r_to_p(r_subs(b_center, p_center));

	if (incidence.value <= min_dist && incidence.titha <= 0
						&& incidence.titha >= -M_PI) {
		/* lets reflect the V vector on the axis given by the normal */
		/*struct r_vector normal = p_to_r(p_make(1, incidence.titha));*/
		/*new_k = oblique_collision(b.body, k, p.body.vel, normal,
						CONSERVATION, TRANSMISSION);
		*/
                /*NADA*/
        	new_k = original_collision(b.body, k, p.body, BOUNCICITY);
	} else if (
		fabsf(b_center.y - (p.body.pos.y + p.body.box.y)) <  b.body.box.y/2
		&& b_center.x > p.body.pos.x
		&& b_center.x < p.body.pos.x + p.body.box.x
		) {
		/*new_k = oblique_collision(b.body, k, p.body.vel, r_make(0, 1),
						CONSERVATION, TRANSMISSION_DOWN); */
	     new_k = original_collision(b.body, k, p.body, BOUNCICITY);
	} else{
		struct kinetic edge;
		struct limit edgeangles;

		edge.vel = p.body.vel;
		if (incidence.titha < M_PI_2) {
			edge.pos = r_sum(p.body.pos, p.body.box);
			edgeangles = l_make(-M_PI, -M_PI_2);
		} else {
			edge.pos = p.body.pos;
			edge.pos.y += p.body.box.y;
			edgeangles = l_make(-M_PI_2, 0);
		}
		new_k = ball_edge_collision(b, k, edge, edgeangles,
						CONSERVATION, TRANSMISSION);
	}

	return new_k;
}

static inline void apply_kinetic(struct free_body *b, struct kinetic k)
{
	b->pos = k.pos;
	b->vel = k.vel;
}

static void apply_player_comm(struct player *p,  struct pcontrol pcomm)
{
	if (pcomm.u && p->body.vel.y == 0)
		p->body.vel.y = -AVATAR_VY;

	if (pcomm.l && !pcomm.r)
		p->body.vel.x = -AVATAR_VX;
	else if (!pcomm.l && pcomm.r)
		p->body.vel.x = AVATAR_VX;
	else
		p->body.vel.x = 0;
}

struct game_result game_umpire(struct game *g)
{
	struct game_result gr = {0};

	if (fabsf((g->b.body.pos.y + g->b.body.box.y) - GAME_AREA_H)
		< FLOOR_HIT_TOL) {
		if ((g->b.body.pos.x + g->b.body.box.x / 2) < GAME_AREA_W/2) {
			g->p[0].points--;
			g->p[1].points++;
			gr.scorer_player = 1;
			gr.has_to_start = 1;
		} else {
			g->p[1].points--;
			g->p[0].points++;
			gr.scorer_player = 0;
			gr.has_to_start = 0;
		}
		gr.set_end = 1;
		if (g->p[0].points == 0 || g->p[1].points == 0)
			gr.game_end = 1;
	}

	return gr;
}

static void _run_game(struct game *g, struct commands comm)
{
	int i;
	struct kinetic kin;

	for (i = 0; i < N_PLAYERS; i++) {
		apply_player_comm(g->p + i, comm.player[i]);
	}

	/*player physics */
	for (i = 0; i < N_PLAYERS; i++) {
		kin = kinetic_step(g->p[i].body);
		kin = world_limit_collision(g->p[i].body, kin, ZoneLimX[i], LimY, REBOUND_OFF);
		apply_kinetic(&(g->p[i].body), kin);
	}

	/* ball physics */
	kin = kinetic_step(g->b.body);
	/*kin = world_limit_collision(g->b.body, kin, LimX, LimY, REBOUND_ON);*/
	kin = ball_poly_collision(g->b, kin, world_poly, ARSIZE(world_poly), r_zero, CONSERVATION, 0);

	for (i = 0; i < N_PLAYERS; i++) {
		kin = ball_player_collision(g->b, kin, g->p[i]);
	}

	kin = ball_poly_collision(g->b, kin, net_poly, ARSIZE(net_poly), r_zero, CONSERVATION, 0);
	kin = ball_poly_collision(g->b, kin, world_poly, ARSIZE(world_poly), r_zero, CONSERVATION, 0);
/*	kin = world_limit_collision(g->b.body, kin, LimX, LimY, REBOUND_ON);*/

	apply_kinetic(&(g->b.body), kin);
}

struct game_result run_game(struct game *g, struct commands comm)
{
	int i;
	struct game_result gr;

	for (i = 0; i < OVERSAMPLING; i++) {
		_run_game(g, comm);
		gr = game_umpire(g);
		if (gr.set_end) {
			break;
		}
	}
	return gr;
}
