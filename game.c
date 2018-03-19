#include <stdlib.h>
#include <string.h>

#include "game.h"

game_t *game_create(int w, int h)
{
	game_t *g = (game_t *) malloc( sizeof(game_t) );
	if ( !g ) return g;

	g->cells = (cell_t *) malloc( w*h );
	g->ticks = 0;
	g->w = w;
	g->h = h;

	memset(g->cells, EMPTY_CELL, w*h);

	g->cells[3*w+1] = 1;

	return g;
}

void game_destroy(game_t *g)
{
	free(g->cells);
	free(g);
}

const cell_t *game_field(const game_t *g)
{
	return g->cells;
}

void game_tick(game_t *g)
{
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
