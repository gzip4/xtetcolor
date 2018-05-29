#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "game.h"

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#define CELL_WP		40
#define FIELD_W		7
#define FIELD_H		18
#define FIELD_WP	(CELL_WP * FIELD_W)
#define FIELD_HP	(CELL_WP * FIELD_H)

#define WINDOW_W	(FIELD_WP + 600)
#define WINDOW_H	(FIELD_HP + 20)

Display *dis;
int screen;
Window win, root;
GC gc;
unsigned long black, white;
Atom wmDeleteMessage;
int keyESC, keyEnter;
void (*drawptr)(Drawable dr, int ww, int wh);
game_t *game;


static time_t const_seconds0;
static suseconds_t const_useconds0;

static time_t getuseconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec - const_seconds0)*1000000+tv.tv_usec-const_useconds0;
}


static time_t getmseconds()
{
	return getuseconds() / 1000;
}


static time_t last_time = (time_t) 0;

static int since_last_time(int msec)
{
	time_t now = getmseconds();
	if ((now - last_time) > msec) {
		last_time = now;
		return 1;
	} else {
		return 0;
	}
}

static void size_hints()
{
	XSizeHints* win_size_hints = XAllocSizeHints();
	if (!win_size_hints) {
		fprintf(stderr, "XAllocSizeHints - out of memory\n");
		exit(1);
	}

	win_size_hints->flags = PSize | PMinSize;
	win_size_hints->min_width = WINDOW_W;
	win_size_hints->min_height = WINDOW_H;
	win_size_hints->base_width = WINDOW_W;
	win_size_hints->base_height = WINDOW_H;

	XSetWMNormalHints(dis, win, win_size_hints);

	XFree(win_size_hints);
}

static void get_window_size(int *w, int *h)
{
	XWindowAttributes win_attr;
	XGetWindowAttributes(dis, win, &win_attr);
	*w = win_attr.width;
	*h = win_attr.height;
}


static void default_draw(Drawable dr, int ww, int wh)
{
	int cw = ww / 2, ch = wh / 2;
	int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP, h = FIELD_HP, cellw = CELL_WP;

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);
}


static void draw(Drawable dr, int ww, int wh)
{
	int cw = ww / 2, ch = wh / 2;
	int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP, h = FIELD_HP, cellw = CELL_WP;
	int colors[] = { 0xff0000, 0xff00, 0xff, 0xff00ff, 0xffff00, 0xffff, 0, 0 };
	int i, j, r;
	cell_t cell;
	cell_t cells[FIELD_W * FIELD_H * sizeof(cell_t)];

	game_field(game, &cells[0]);

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);

	for (j = 0; j < FIELD_H; ++j) {
		//printf("%d: ", j);
		for (i = 0; i < FIELD_W; ++i) {
			cell = cells[j * FIELD_W + i];
			if (cell == EMPTY_CELL) continue;
			XSetForeground(dis, gc, colors[cell]);
			XFillRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
			XSetForeground(dis, gc, black);
			XDrawRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
			//printf(" %d(%d)", i, cell);
		}
		//printf("\n");
	}


	return;

	for (i = 0; i < FIELD_W; ++i) {
		for (j = 0; j < FIELD_H; ++j) {
			r = rand() % 7; //printf("%d ", r);
			if (r == 6) continue;
			XSetForeground(dis, gc, colors[r]);
			XFillRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
			XSetForeground(dis, gc, black);
			XDrawRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
		}
	}

}


static void draw_win()
{
	int w, h;
	int depth = DefaultDepth(dis, screen);
	Pixmap pixmap;
	char buff[32];
	int buffsz;

	get_window_size(&w, &h);
	pixmap = XCreatePixmap(dis, root, w, h, depth);

	XSetForeground(dis, gc, 0x303030);
	XSetBackground(dis, gc, black);
	XFillRectangle(dis, pixmap, gc, 0, 0, w, h);

	if (drawptr) {
		drawptr(pixmap, w, h);
	} else {
		default_draw(pixmap, w, h);
	}

	XCopyArea(dis, pixmap, win, gc, 0, 0, w, h, 0, 0);
	XFreePixmap(dis, pixmap);

	if (game && game->game_over) {
		XSetForeground(dis, gc, white);
		XSetBackground(dis, gc, black);
		XDrawImageString(dis, win, gc, 100, 100, " -=GAME OVER=- ", 15);
	}

	if (game) {
		buffsz = snprintf(buff, 32, "LEVEL: %d", game->level + 1);
		XSetForeground(dis, gc, white);
		XSetBackground(dis, gc, black);
		XDrawImageString(dis, win, gc, 100, 80, buff, buffsz);
	}

	XFlush(dis);
}


static int init_win()
{
	XEvent event;

	win = XCreateSimpleWindow(dis, root, 0, 0, WINDOW_W, WINDOW_H, 0, None, None);
	XSetWMProtocols(dis, win, &wmDeleteMessage, 1);
	XSetStandardProperties(dis, win, "X Tetcolor", "HI!", None, NULL, 0, NULL);
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);
	size_hints(dis, win);

	/* transparent window - avoid flickering */
	XSetWindowBackground(dis, win, None);
	XSetWindowBackgroundPixmap(dis, win, None);

	gc = XCreateGC(dis, win, 0, 0);

	XSetBackground(dis, gc, black);
	XSetForeground(dis, gc, white);
	XClearWindow(dis, win);
	XMapRaised(dis, win);
	XFlush(dis);

	while (XPending(dis)) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == wmDeleteMessage) {
			return 0;
		}

		if (event.type==KeyPress && event.xkey.keycode==keyESC) {
			return 0;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
		}
	}

	return 1;
}

// 0 - exit, 1 - key, if keycode=0 - anykey
static int wait_key(int keycode)
{
	XEvent event;
	while (1) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == wmDeleteMessage) {
			return 0;
		}

		if (event.type==KeyPress && event.xkey.keycode==keyESC) {
			return 0;
		}

		if (event.type==KeyPress && keycode==0) {
			return 1;
		}

		if (event.type==KeyPress && event.xkey.keycode==keycode) {
			return 1;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
		}
	}
}

static void check_for_tick()
{
	int msec = 1000 - game->level * 50;
	if (since_last_time(msec < 100 ? 100 : msec)) {
		game_tick(game);
		draw_win();
	}
}


static int x11_loop()
{
	XEvent event;

	check_for_tick();

	while(XPending(dis))
	{
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == wmDeleteMessage) {
			return 0;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
		}

		if (event.type==KeyPress && event.xkey.keycode==keyESC) {
			return 0;
		}

		if (event.type==KeyPress) {
			printf("KeyPress: %x\n", event.xkey.keycode);
		}
	}

	return 1;
}


static int time_loop()
{
	int x11_fd, num_ready_fds;
	fd_set in_fds;
	struct timeval tv;

	x11_fd = ConnectionNumber(dis);
	last_time = getmseconds();

	while(1) {
		XFlush(dis);

		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);
		tv.tv_usec = 100000;	// 0.1 sec
		tv.tv_sec = 0;
		num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds == 0) {
			check_for_tick();
		}

		if (!x11_loop()) {
			return 0;
		}

		if (game->game_over) {
			return 1;
		}
	}
}


static void game_loop()
{
	drawptr = NULL;
	draw_win();

again:
	if (!wait_key(0)) {
		return;
	}

	game = game_create(FIELD_W, FIELD_H);
	if ( !game ) {
		perror("game_create");
		exit(1);
	}

	drawptr = draw;
	draw_win();

	if (!time_loop()) {
		return;
	}

	game_destroy(game); game = NULL;
	goto again;
}


int main(int argc, char *argv[])
{
	struct timeval tv;

	dis	= XOpenDisplay((char *)0);
	if (!dis) {
		fprintf(stderr, "Couldn't open X11 display\n");
		exit(1);
	}

	screen	= DefaultScreen(dis);
	black	= BlackPixel(dis,screen);
	white	= WhitePixel(dis, screen);
	root	= DefaultRootWindow(dis);
	wmDeleteMessage = XInternAtom(dis, "WM_DELETE_WINDOW", False);
	keyESC	= XKeysymToKeycode(dis, XK_Escape);
	keyEnter= XKeysymToKeycode(dis, XK_Return);

	gettimeofday(&tv, NULL);
	const_seconds0 = tv.tv_sec;
	const_useconds0 = tv.tv_usec;
	srand(tv.tv_sec & tv.tv_usec);

	if (!init_win()) {
		goto main_exit;
	}

	game_loop();

main_exit:
	XFreeGC(dis, gc);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);

	return 0;
}
