#include <stdio.h>
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

	g->fig = (cell_t *) malloc( 3*3*sizeof(cell_t) );
	if ( !g->fig ) {
		free(g->cells);
		free(g);
		return NULL;
	}

	g->ticks = 0;
	g->w = w;
	g->h = h;
	g->level = 0;
	g->score = 0;
	g->game_over = 0;

	g->ftyp = -1;
	g->fx = -1;
	g->fy = -1;

	memset(g->cells, EMPTY_CELL, w*h*sizeof(cell_t));
	memset(g->fig, EMPTY_CELL, 3*3*sizeof(cell_t));

	//g->cells[1] = 1;
	//g->cells[3*w+1] = 1;

	return g;
}

void game_destroy(game_t *g)
{
	free(g->fig);
	free(g->cells);
	free(g);
}

void game_field(const game_t *g, cell_t *cp)
{
	int i, j;
	cell_t cell;

	memcpy(cp, g->cells, g->w * g->h * sizeof(cell_t));

	if (g->ftyp == -1) {
		return;
	}

	// copy figure
	for (j = 0; j < 3; ++j) {
		for (i = 0; i < 3; ++i) {
			cell = g->fig[j * 3 + i];
			if (cell != EMPTY_CELL) {
				cp[(g->fy+j) * g->w + (g->fx+i)] = cell;
			}
		}
	}
}

static void gen_figure(game_t *g);

void game_tick(game_t *g)
{
	if (g->game_over) return;

	if (g->ticks >= 6) {
		g->game_over = 1;
		return;
	}

	if (g->ftyp == -1) {
		gen_figure(g);
		goto tick_end;
	}

	if (g->ftyp != -1) {
		++g->fy;
	}

//	g->cells[g->ticks*g->w+3] = 1;
//	if (g->ticks > 0) {
//		g->cells[(g->ticks-1)*g->w+3] = EMPTY_CELL;
//	}

tick_end:
	++g->ticks;
//	++g->level;
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

static void gen_figure(game_t *g)
{
	g->ftyp = rand() % 4;
	memset(g->fig, EMPTY_CELL, 3*3*sizeof(cell_t));

	switch (g->ftyp) {
	case 0:
		g->fx = 3;
		g->fy = 0;
		g->fig[0] = rand() % 6;
		break;
	case 1:
		g->fx = 3;
		g->fy = 0;
		g->fig[0] = rand() % 6;
		g->fig[3] = rand() % 6;
		break;
	case 2:
		g->fx = 3;
		g->fy = 0;
		g->fig[0] = rand() % 6;
		g->fig[3] = rand() % 6;
		g->fig[6] = rand() % 6;
		break;
	case 3:
		g->fx = 3;
		g->fy = 0;
		g->fig[0] = rand() % 6;
		g->fig[3] = rand() % 6;
		g->fig[4] = rand() % 6;
		break;
	}
	printf("gen: %d\n", g->ftyp);
	printf("%d %d %d\n", g->fig[0], g->fig[1], g->fig[2]);
	printf("%d %d %d\n", g->fig[3], g->fig[4], g->fig[5]);
	printf("%d %d %d\n", g->fig[6], g->fig[7], g->fig[8]);
}

