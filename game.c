#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

static void gen_figure(game_t *g);
static int collision(const game_t *g, int x, int y, cell_t *fig);
static void copy_figure(const game_t *g, cell_t *cp);
static void clear_figure(game_t *g);
static int check_combinations(game_t *g);
static int check_color(game_t *g, cell_t *buff, char *coords, int ncoords);


game_t *game_create(int w, int h)
{
	game_t *g = (game_t *) malloc( sizeof(game_t) );
	if ( !g ) return NULL;

	g->cells = (cell_t *) malloc( w * h );
	if ( !g->cells ) {
		free(g);
		return NULL;
	}

	g->fig = (cell_t *) malloc( 9 );
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
	g->combi_cb = NULL;
	g->ncomb = 0;

	memset(g->cells, EMPTY_CELL, w*h);
	clear_figure(g);

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
	memcpy(cp, g->cells, g->w * g->h);
	copy_figure(g, cp);
}


int game_tick(game_t *g)
{
	if (g->game_over) return 0;

	// level up
	if (g->ticks > 1 && g->ticks % 100 == 0) {
		++g->level;
	}

	// no figure
	if (g->ftyp == -1) {
		gen_figure(g);
		if (collision(g, g->fx, g->fy, NULL)) {
			copy_figure(g, g->cells);
			clear_figure(g);
			g->game_over = 1;
			return 0;
		}
		++g->ticks;
		return 2;
	}

	// have figure
	if (collision(g, g->fx, g->fy + 1, NULL)) {
		// land figure
		copy_figure(g, g->cells);
		clear_figure(g);

		g->ncomb = 0;
		while (check_combinations(g)) {
			++g->ncomb;
		}
		g->ncomb = 0;
		return 1;
	} else {
		// advance figure
		++g->fy;
		return 2;
	}
}


int game_move_r(game_t *g)
{
	if (collision(g, g->fx + 1, g->fy, NULL)) {
		return 0;
	}

	++g->fx;
	return 1;
}


int game_move_l(game_t *g)
{
	if (collision(g, g->fx - 1, g->fy, NULL)) {
		return 0;
	}

	--g->fx;
	return 1;
}

/*  0 1 2  6 3 0  2 5 8
 *  3 4 5  7 4 1  1 4 7
 *  6 7 8  8 5 2  0 3 6
 *  SRC    LEFT   RIGHT
 */
int game_move_rot(game_t *g, int dir)
{
	int i;
	const int *cr;
	const int rr[] = {2, 5, 8, 1, 4, 7, 0, 3, 6};		// right rotation
	const int lr[] = {6, 3, 0, 7, 4, 1, 8, 5, 2};		// left rotation
	cell_t rc[9];

	memset(rc, EMPTY_CELL, 9);

	switch (g->ftyp) {
	case 1:
	case 2:
	case 3:
		if (dir) cr = rr; else cr = lr;
		for (i = 0; i < 9; ++i) {
			rc[cr[i]] = g->fig[i];
		}
		if (collision(g, g->fx, g->fy, rc)) {
			return 0;
		}
		break;
	default:
		return 0;	// do not rotate single cell
	}

	memcpy(g->fig, rc, 9);
	return 1;
}


int game_move_down(game_t *g)
{
	int y = g->fy;
	while (1) {
		if (collision(g, g->fx, y, NULL)) {
			g->fy = y - 1;
			break;
		}
		++y;
	}
	return 1;
}


////////////////////////////////////////////////////////////////////////////

static void clear_figure(game_t *g)
{
	g->ftyp = -1;
	g->fx = -1;
	g->fy = -1;

	memset(g->fig, EMPTY_CELL, 9);
}


static void copy_figure(const game_t *g, cell_t *cp)
{
	const int x = g->fx, y = g->fy, w = g->w;
	cell_t cell;

	if (g->ftyp == -1) {
		return;
	}

	cell = g->fig[0]; if (cell != EMPTY_CELL) cp[(y+0)*w+(x+0)] = cell;
	cell = g->fig[1]; if (cell != EMPTY_CELL) cp[(y+0)*w+(x+1)] = cell;
	cell = g->fig[2]; if (cell != EMPTY_CELL) cp[(y+0)*w+(x+2)] = cell;
	cell = g->fig[3]; if (cell != EMPTY_CELL) cp[(y+1)*w+(x+0)] = cell;
	cell = g->fig[4]; if (cell != EMPTY_CELL) cp[(y+1)*w+(x+1)] = cell;
	cell = g->fig[5]; if (cell != EMPTY_CELL) cp[(y+1)*w+(x+2)] = cell;
	cell = g->fig[6]; if (cell != EMPTY_CELL) cp[(y+2)*w+(x+0)] = cell;
	cell = g->fig[7]; if (cell != EMPTY_CELL) cp[(y+2)*w+(x+1)] = cell;
	cell = g->fig[8]; if (cell != EMPTY_CELL) cp[(y+2)*w+(x+2)] = cell;
}


static int collision(const game_t *g, int x, int y, cell_t *fig)
{
	int i, j, k = 0, n = 4;
	cell_t cell;

	if (y >= g->h) return 1;

	if (!fig) fig = g->fig;

	for (j = 0; j < 3; ++j) {
		for (i = 0; i < 3; ++i) {
			cell = fig[j * 3 + i];
			if (cell == EMPTY_CELL) continue;
			if ((y + j) >= g->h) {
				return 1;
			}
			if (g->cells[(y+j) * g->w + (x+i)] != EMPTY_CELL) {
				return 1;
			}
			k = MAX(k, i);	// max filled i
			n = MIN(n, i);	// min filled i
		}
	}

	if ((x + k) >= g->w) return 1;
	if ((x + n) <  0   ) return 1;

	return 0;
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
	memset(g->fig, EMPTY_CELL, 9);
	g->ftyp = rand() % 4;

	switch (g->ftyp) {
	case 0:
		g->fx = 2;
		g->fy = 0;
		g->fig[1] = rand() % 6;
		break;
	case 1:
		g->fx = 2;
		g->fy = 0;
		g->fig[1] = rand() % 6;
		g->fig[4] = rand() % 6;
		break;
	case 2:
		g->fx = 2;
		g->fy = 0;
		g->fig[1] = rand() % 6;
		g->fig[4] = rand() % 6;
		g->fig[7] = rand() % 6;
		break;
	case 3:
		g->fx = 2;
		g->fy = 0;
		g->fig[1] = rand() % 6;
		g->fig[4] = rand() % 6;
		g->fig[5] = rand() % 6;
		break;
	}
}


static int check_color(game_t *g, cell_t *buff, char *coords, int ncoords)
{
	int x, y, xx, yy, i, count, bn;
	const int w = g->w, h = g->h, bsz = (w + h) * 2;
	char *diag = (char *) malloc(bsz);

	// vertical 3+
	for (x = 0; x < w; ++x) {
		for (y = 0; y < h; ++y) {
			if (!buff[y * w + x]) continue;

			count = 1;
			for (yy = y + 1; yy < h; ++yy) {
				if (!buff[yy * w + x]) break;
				++count;
			}
			if (count > 2) {
				for (i = 0; i < count; ++i) {
					coords[ncoords++] = x;
					coords[ncoords++] = y + i;
				}
			}
			y = yy;
		}
	}

	// horizontal 3+
	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			if (!buff[y * w + x]) continue;

			count = 1;
			for (xx = x + 1; xx < w; ++xx) {
				if (!buff[y * w + xx]) break;
				++count;
			}
			if (count > 2) {
				for (i = 0; i < count; ++i) {
					coords[ncoords++] = x + i;
					coords[ncoords++] = y;
				}
			}
			x = xx;
		}
	}

	// diagonal right 3+
	bn = 0;
	for (x = w - 1, y = 0; x >= 0; --x) {
		diag[bn++] = x;
		diag[bn++] = y;
	}
	for (x = 0, y = 0; y < h; ++y) {
		diag[bn++] = x;
		diag[bn++] = y;
	}
	for (bn = 0; bn < bsz; bn += 2) {
		x = diag[bn];
		y = diag[bn + 1];
		for ( ; x < w && y < h; ++x, ++y) {
			if (!buff[y * w + x]) continue;

			count = 1;
			for (xx = x + 1, yy = y + 1; xx < w && yy < h; ++xx, ++yy) {
				if (!buff[yy * w + xx]) break;
				++count;
			}
			if (count > 2) {
				for (i = 0; i < count; ++i) {
					coords[ncoords++] = x + i;
					coords[ncoords++] = y + i;
				}
			}
			x = xx;
			y = yy;
		}
	}

	// diagonal left 3+
	bn = 0;
	for (x = 0, y = 0; x < w; ++x) {
		diag[bn++] = x;
		diag[bn++] = y;
	}
	for (x = w - 1, y = 0; y < h; ++y) {
		diag[bn++] = x;
		diag[bn++] = y;
	}
	for (bn = 0; bn < bsz; bn += 2) {
		x = diag[bn];
		y = diag[bn + 1];
		for ( ; x >= 0 && y < h; --x, ++y) {
			if (!buff[y * w + x]) continue;

			count = 1;
			for (xx = x - 1, yy = y + 1; xx >= 0 && yy < h; --xx, ++yy) {
				if (!buff[yy * w + xx]) break;
				++count;
			}
			if (count > 2) {
				for (i = 0; i < count; ++i) {
					coords[ncoords++] = x - i;
					coords[ncoords++] = y + i;
				}
			}
			x = xx;
			y = yy;
		}
	}

	free(diag);

	return ncoords;
}

#define COORDS_BUFFSZ	512

static int check_combinations(game_t *g)
{
	int i, x, y, yy, clr, ncoords = 0;
	const int w = g->w, buffsz = g->w * g->h;
	cell_t cell;

	cell_t *buff = (cell_t *) malloc(buffsz);
	char *coords = (char *) malloc(COORDS_BUFFSZ);

	for (clr = 0; clr < 6; ++clr) {
		memset(buff, 0, buffsz);
		for (i = 0; i < buffsz; ++i) {
			if (g->cells[i] == clr) buff[i] = 1;
		}

		ncoords = check_color(g, buff, coords, ncoords);
	}

	// callback if any
	if (ncoords > 0 && g->combi_cb) {
		(*g->combi_cb)(coords, ncoords);
	}

	// delete cell and slide whole column down
	for (i = 0; i < ncoords; i += 2) {
		x = coords[i];
		y = coords[i + 1];

		for (yy = y; yy >= 0; --yy) {
			cell = (yy == 0) ? EMPTY_CELL : g->cells[(yy - 1) * w + x];
			g->cells[yy * w + x] = cell;
		}

		// score count per cell
		g->score += 9 * (g->level + 1) * (g->ncomb + 1);
	}

	if (ncoords >= 12) {
		g->score += 1000 * (g->level + 1) * (g->ncomb + 1);
	}

	free(coords);
	free(buff);

	return ncoords;
}

