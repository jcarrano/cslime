/*
 * cslime_ai.h
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

#ifndef __CSLIME_AI_H__
#define __CSLIME_AI_H__

#include "cslime.h"

struct pcontrol greedy_player(struct game g, int player_number, bool aggressive);

#endif /*__CSLIME_AI_H__*/
