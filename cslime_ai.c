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
#include <stdio.h>
#include "cslime_ai.h"
#include "vector.h"

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
