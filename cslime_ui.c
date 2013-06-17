/*
 * cslime_ui.c
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
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include "common.h"
#include "cslime.h"
#include "cslime_ai.h"


#define DEFAULT_SCALE 1500
#define MIN_SCALE 150

#define POINT_RADIUS (GAME_AREA_H/30)
#define POINT_BORDER (POINT_RADIUS/6)
#define POINT_SPACING ((POINT_RADIUS + POINT_BORDER)*1.20)
#define POINTS_Y (POINT_RADIUS*4)
#define POINTS_LEFT_X (POINTS_Y)
#define POINTS_RIGHT_X (GAME_AREA_W-POINTS_LEFT_X)
#define AUX_AREA_H .03

#define TO_PIXELS(x, factor) ((int)roundf((x)*(factor)))

#define SCR_W (GAME_AREA_W)
#define SCR_H (GAME_AREA_H + AUX_AREA_H)

#define ASPECT_RATIO (SCR_W/SCR_H)

static const struct r_vector NameLoc[N_PLAYERS] = {
	{GAME_AREA_W/8, GAME_AREA_H + AUX_AREA_H/4},
	{5*GAME_AREA_W/8, GAME_AREA_H + AUX_AREA_H/4}
};

#define GAME_NAME "Subcritical CSlime v0.1"

#define INCMOD(q, p) (((q)+1)%(p))
#define DECMOD(q, p) (((q)+(p)-1)%(p))

struct RGB {
	unsigned char r, g, b;
};

inline struct RGB rgb(unsigned char r, unsigned char g, unsigned char b)
{
	struct RGB ret;
	ret.r = r;
	ret.g = g;
	ret.b = b;
	return ret;
}

struct avatar {
	void (*painter)(SDL_Surface *surf, float scale, float t,
					struct player p, void *cb_data);
	void *painter_data;
	char *name;
};

static const struct avatar NullAvatar = {NULL, NULL, NULL};

void classic_avatar_painter(SDL_Surface *surf, float scale, float t,
					struct player p, void *_col);

#define RGBlit(r,g,b) (&((const struct RGB){(r), (g), (b)}))
struct avatar Avatars[] = {
	{classic_avatar_painter, RGBlit(0x00, 0x00, 0x00), "Afro Slime"},
	{classic_avatar_painter, RGBlit(0xFF, 0xFF, 0xFF), "Palid Slime"},
	{classic_avatar_painter, RGBlit(0xC8, 0x2B, 0x00), "Ginger Slime"},
	{classic_avatar_painter, RGBlit(0x1A, 0x91, 0x00), "Hulk Slime"},
	{classic_avatar_painter, RGBlit(0xBF, 0x34, 0xA5), "Gay Slime"},
	{classic_avatar_painter, RGBlit(0xEF, 0x10, 0x10), "Federal Slime"},
};

#define N_AVATARS ARSIZE(Avatars)

struct uidata {
	SDL_Surface *surf;
	float t;
	float scale;
	struct avatar avatars[N_PLAYERS];
	struct game_result gr;
};

SDL_Rect vec2rect(struct r_vector pos, struct r_vector box, float scale)
{
	SDL_Rect r;

	r.x = TO_PIXELS(pos.x, scale);
	r.y = TO_PIXELS(pos.y, scale);
	r.w = TO_PIXELS(box.x, scale);
	r.h = TO_PIXELS(box.y, scale);

	return r;
}

void classic_avatar_painter(SDL_Surface *surf, float scale, float t,
						struct player p, void *_col) {
	struct RGB *col = _col;

	int x = TO_PIXELS(p.body.pos.x + p.body.box.x/2, scale);
	int r = TO_PIXELS(p.body.box.y, scale);
	int y = TO_PIXELS(p.body.pos.y + p.body.box.y, scale);
	/*int r_eye = TO_PIXELS(p.body.box.y/7, scale);*/

	filledPieRGBA(surf, x, y, r, 180, 0, col->r, col->g, col->b, 255);
	/*filledCircleRGBA(surf, x, y, r_eye, 255, 255, 255, 255);*/
}

int filledPolygonRGBA2(SDL_Surface *dst, const struct r_vector *edges, int n_edges,
				float scale, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	Sint16 *vertices;

	if (NMALLOC(vertices, n_edges*2)) {
		int i;
		for (i = 0; i < n_edges; i++) {
			vertices[i] = edges[i].x*scale;
			vertices[i+n_edges] = edges[i].y*scale;
		}
		filledPolygonRGBA(dst, vertices, vertices + n_edges, n_edges,
								r, g, b, a);
		free(vertices);
		return 0;
	} else {
		return -1;
	}
}

enum {POINTS_LR, POINTS_RL};

#define _TO_PIX(x) TO_PIXELS((x), ui->scale)
void drawPoints(struct uidata *ui, float x, float y, int direction, int points,
				struct RGB fg, struct RGB bg, Uint8 alpha)
{
	int i;
	float sign;

	sign = (direction == POINTS_LR)? 1: -1;

	for (i = 0; i < points; i++) {
		filledCircleRGBA(ui->surf,
			_TO_PIX(x + sign*i*(POINT_RADIUS + POINT_SPACING)),
			_TO_PIX(y),
			_TO_PIX(POINT_RADIUS), fg.r, fg.g, fg.b, alpha);
		filledCircleRGBA(ui->surf,
			_TO_PIX(x + sign*i*(POINT_RADIUS + POINT_SPACING)),
			_TO_PIX(y),
			_TO_PIX(POINT_RADIUS - POINT_BORDER), bg.r, bg.g, bg.b, alpha);
	}
}

void draw_game(struct game g, struct uidata *ui)
{
	SDL_Rect net, aux_area;
	int i;
	struct r_vector ball_center = BODY_CENTER(g.b.body);
	Uint32 overcolor;
	SDL_Surface *alpha;

	SDL_FillRect(ui->surf, NULL, SDL_MapRGB(ui->surf->format, 40, 40, 180));
	/*boxRGBA(ui->surf, 0, 0, ui->surf->w, ui->surf->h, 40, 40, 40, 100);*/

	net.x = _TO_PIX(PLAYER_AREA_W);
	net.y = _TO_PIX(GAME_AREA_H - NET_H);
	net.w = _TO_PIX(NET_W);
	net.h = _TO_PIX(NET_H);
	SDL_FillRect(ui->surf, &net, SDL_MapRGB(ui->surf->format, 250, 250, 250));
	filledPolygonRGBA2(ui->surf, net_poly, ARSIZE(net_poly), ui->scale, 200, 100, 200, 255);
	filledCircleRGBA(ui->surf, _TO_PIX(ball_center.x),
			_TO_PIX(ball_center.y),
			_TO_PIX(g.b.body.box.y/2), 180, 180, 180, 255);

	for (i = 0; i < N_PLAYERS; i++) {
		if (ui->avatars[i].painter != NULL) {
			ui->avatars[i].painter(ui->surf, ui->scale, ui->t,
					g.p[i], ui->avatars[i].painter_data);
		} else {
			SDL_Rect r = vec2rect(g.p[i].body.pos, g.p[i].body.box,
								ui->scale);
			SDL_FillRect(ui->surf, &r, SDL_MapRGB(ui->surf->format, 0, 60, 160));
		}
		stringRGBA(ui->surf, _TO_PIX(NameLoc[i].x), _TO_PIX(NameLoc[i].y),
			(ui->avatars[i].name != NULL) ? ui->avatars[i].name : "No name",
			200, 200, 200, 255);
	}
	drawPoints(ui, POINTS_LEFT_X, POINTS_Y, POINTS_LR, g.p[0].points,
				rgb(200, 200, 200), rgb(20, 200, 200), 100);
	drawPoints(ui, POINTS_RIGHT_X, POINTS_Y, POINTS_RL, g.p[1].points,
				rgb(200, 200, 200), rgb(200, 100, 20), 100);

	aux_area.x = 0;
	aux_area.y = _TO_PIX(GAME_AREA_H)+2; /*arreglar!!!!*/
	aux_area.w = _TO_PIX(GAME_AREA_W);
	aux_area.h = _TO_PIX(AUX_AREA_H);

	SDL_FillRect(ui->surf, &aux_area, SDL_MapRGB(ui->surf->format, 40, 40, 40));

	for (i = 0; i < N_PLAYERS; i++) {
		stringRGBA(ui->surf, _TO_PIX(NameLoc[i].x), _TO_PIX(NameLoc[i].y),
			(ui->avatars[i].name != NULL) ? ui->avatars[i].name : "No name",
			200, 200, 200, 255);
	}

	SDL_Flip(ui->surf);
}

int EFilter(const SDL_Event *event)
{
	if(	event->type == SDL_QUIT
	   || 	event->type == SDL_KEYDOWN
	   ||	event->type == SDL_VIDEORESIZE
	   )
		return 1;
	else
		return 0;
}

void flush_events()
{
	SDL_Event dummy;
	while(SDL_PollEvent(&dummy));
}

struct input {
	struct commands comm;
	bool pause, quit, reset;
	struct {
		bool requested;
		int w, h;
	} resize;
};

#define PAUSE_KEY SDLK_p

struct input poll_input()
{
	struct input inp = {{{{0}}}, 0};
	Uint8 *keys;
	SDL_Event ev;

	SDL_PumpEvents();
	keys = SDL_GetKeyState(NULL);

	inp.comm.player[0].u = keys[SDLK_w];
	/*inp.comm.player[0].d = keys[SDLK_s];*/
	inp.comm.player[0].l = keys[SDLK_a];
	inp.comm.player[0].r = keys[SDLK_d];
	/*inp.comm.player[0].aux = keys[SDLK_LSHIFT];*/

	inp.comm.player[1].u = keys[SDLK_UP];
	/*inp.comm.player[1].d = ;keys[SDLK_DOWN];*/
	inp.comm.player[1].l = keys[SDLK_LEFT];
	inp.comm.player[1].r = keys[SDLK_RIGHT];
	/*inp.comm.player[1].aux = keys[SDLK_KP_ENTER];*/

	/* inp.comm.aux = keys[SDLK_SPACE]; */

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			inp.quit = 1;
		} else if (ev.type == SDL_VIDEORESIZE) {
			inp.resize.requested = 1;
			inp.resize.w = ev.resize.w;
			inp.resize.h = ev.resize.h;
		} else if (ev.type == SDL_KEYDOWN){
			switch(ev.key.keysym.sym){
			case SDLK_s:		inp.comm.player[0].d = 1; break;
			case SDLK_DOWN:		inp.comm.player[1].d = 1; break;
			case SDLK_LSHIFT:	inp.comm.player[0].aux = 1; break;
			case SDLK_KP_ENTER:	inp.comm.player[1].aux = 1; break;
			case SDLK_SPACE:	inp.comm.aux = 1; 	break;
			case SDLK_r:		inp.reset = 1;		break;
			case PAUSE_KEY:		inp.pause = 1;		break;

			case SDLK_q:	/* fall-through */
			case SDLK_ESCAPE:
						inp.quit = 1;	break;

			default: 		break;
			}
		}
	}

	return inp;
}

void wait_unpause()
{
	SDL_Event ev;

	while(SDL_WaitEvent(&ev)) {
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == PAUSE_KEY)
			break;
	}
}

#define FRAMERATE SIMSTEP
#define SET_INTERVAL (1*1000)

#define SDL_VFLAGS (SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE|SDL_SRCALPHA)

void _ui_putscreen(struct uidata *ui, float scale)
{
	const SDL_VideoInfo* vinfo = SDL_GetVideoInfo();

	ui->surf = SDL_SetVideoMode(TO_PIXELS(SCR_W, scale),
				TO_PIXELS(SCR_H, scale),
				vinfo->vfmt->BitsPerPixel,
				SDL_VFLAGS);
	ui->scale = scale;
}

void uiresize(struct uidata *ui, int w, int h)
{
	_ui_putscreen(ui, fmaxf(w/SCR_W, h/SCR_H));
}

struct uidata ui_init(float scale)
{
	struct uidata ui;

	SDL_WM_SetCaption(GAME_NAME, GAME_NAME);
	_ui_putscreen(&ui, scale);

	ui.t = 0;
	ui.avatars[0] = Avatars[2];
	ui.avatars[1] = Avatars[4]; /*Avatars[4];*/

	return ui;
}

#define ui_ok(u) (((u).surf) != NULL)

void ui_uninit(struct uidata *ui)
{
	SDL_FreeSurface(ui->surf);
}

enum {NEURAL_PLAYER, GREEDY_PLAYER};
#define NEURAL_CFG_FILE "player.net"

int main(int argc, char **argv)
{
	struct uidata ui;

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
		goto end_program;

	ui = ui_init(DEFAULT_SCALE);
	if (!ui_ok(ui))
		goto free_ui;

	srand(time(NULL));
	{
		struct game g = game_init(DEF_START_POINTS, rand()%2);
		int t0 = SDL_GetTicks();
		int p0_manual = 1, p1_manual = 1;
		int player_type;
		NeuralData brain;
		FILE *cfg;

		if ((cfg = fopen(NEURAL_CFG_FILE, "r")) != NULL
		    && neural_bp_player_valid_data(
				brain = neural_bp_player_fread(cfg)
			)) {
			player_type = NEURAL_PLAYER;
			fprintf(stderr, "Loaded neural network player\n");
		} else {
			player_type = GREEDY_PLAYER;
			fprintf(stderr,
"Oops, problem with neural network player, will revert to greedy player\n");
		}

		while (1) {
			struct input inp;
			int new_sleep, t1, i;

			inp = poll_input();
			if (inp.comm.player[0].aux)
				p0_manual = !p0_manual;
			if (inp.comm.player[1].aux)
				p1_manual = !p1_manual;
			if (inp.quit)
				break;

			if (inp.resize.requested) {
				uiresize(&ui, inp.resize.w, inp.resize.h);
			}

			for (i = 0; i < N_PLAYERS; i++) {
				if (inp.comm.player[i].d)
					ui.avatars[i] = Avatars[rand()%N_AVATARS];
			}
			if (!p1_manual) {
				if (player_type == NEURAL_PLAYER)
					inp.comm.player[1] =
						neural_bp_player(g , 1, brain);
				else
					inp.comm.player[1] = greedy_player(g , 1, 1);
			}
			if (!p0_manual)
				inp.comm.player[0] = greedy_player(g , 0, 1);
			ui.gr = run_game(&g, inp.comm);
			draw_game(g, &ui);

			ui.t += FRAMERATE;

			t1 = SDL_GetTicks();
			new_sleep = 2*FRAMERATE - (t1-t0);
			if (new_sleep > 0)
				SDL_Delay(new_sleep);

                        if(ui.gr.game_end || ui.gr.set_end) {
                                SDL_Delay(SET_INTERVAL);
			}
			if (inp.pause) {
				wait_unpause();
				t0 = SDL_GetTicks();
			} else {
				t0 = t1;
			}
			if (ui.gr.game_end)
				g = game_init(DEF_START_POINTS, rand()%2);
			else if (ui.gr.set_end)
				game_reset(&g, ui.gr.has_to_start);
		}

		if (player_type == NEURAL_PLAYER)
			neural_bp_player_destroy_data(brain);
	}


free_ui:
	ui_uninit(&ui);
end_program:
	SDL_Quit();

	return 0;
}

