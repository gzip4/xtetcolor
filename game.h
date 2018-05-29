#ifndef GAME_H
#define GAME_H

typedef char cell_t;
#define EMPTY_CELL ((cell_t)(-1))

typedef struct _game_s
{
	cell_t *cells;
	int ticks;
	int w, h;
	int level;
	int score;
	int game_over;

	int ftyp;
	int fx, fy;
	cell_t *fig;
} game_t;


game_t *game_create(int w, int h);
void game_destroy(game_t *g);

void game_field(const game_t *g, cell_t *cp);
void game_tick(game_t *g);

int game_move_r(game_t *g);
int game_move_l(game_t *g);
int game_move_rot(game_t *g);
int game_move_down(game_t *g);

#endif /* GAME_H */
