#include <stdlib.h>
#include <string.h>

#include "game.h"

game_t *game_create(int w, int h)
{
	game_t *g = (game_t *) malloc( sizeof(game_t) );
	if ( !g ) return NULL;

	g->cells = (cell_t *) malloc( w*h*sizeof(cell_t) );
	if ( !g->cells ) {
		free(g);
		return NULL;
	}

	g->ticks = 0;
	g->w = w;
	g->h = h;
	g->level = 0;
	g->score = 0;
	g->game_over = 0;

	memset(g->cells, EMPTY_CELL, w*h);

	//g->cells[1] = 1;
	//g->cells[3*w+1] = 1;

	return g;
}

void game_destroy(game_t *g)
{
	free(g->cells);
	free(g);
}

void game_field(const game_t *g, cell_t *cp)
{
	memcpy(cp, g->cells, g->w*g->h*sizeof(cell_t));
}

void game_tick(game_t *g)
{
	if (g->game_over) return;

	if (g->ticks >= g->h) {
		g->game_over = 1;
		return;
	}

	g->cells[g->ticks*g->w+3] = 1;
	if (g->ticks > 0) {
		g->cells[(g->ticks-1)*g->w+3] = EMPTY_CELL;
	}

	++g->ticks;
	++g->level;
}

/*
 *
 *	0: #
 *
 *
 *	1: ##
 *
 *
 *	2: ###
 *
 *
 *	3: #
 *	   ##
 *
 *
 */

void gen_figure()
{
	int typ = rand() % 4;
}

