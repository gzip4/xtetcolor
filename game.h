#ifndef GAME_H
#define GAME_H

#define EMPTY_CELL (-1)

typedef char cell_t;

typedef struct _game_s
{
	cell_t *cells;
	int ticks;
	int w, h;

} game_t;


game_t *game_create(int w, int h);
void game_destroy(game_t *g);

const cell_t *game_field(const game_t *g);


#endif /* GAME_H */
