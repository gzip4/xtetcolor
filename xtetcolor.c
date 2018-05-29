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
time_t seconds0;
void (*drawptr)(Drawable dr, int ww, int wh);
game_t *game;


static time_t getuseconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec - seconds0)*1000000+tv.tv_usec;
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
	const cell_t * cells = game_field(game);
	cell_t cell;

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);

	for (j = 0; j < FIELD_H; ++j) {
		for (i = 0; i < FIELD_W; ++i) {
			cell = cells[j * FIELD_W + i];
			if (cell == EMPTY_CELL) continue;
			XSetForeground(dis, gc, colors[cell]);
			XFillRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
			XSetForeground(dis, gc, black);
			XDrawRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
		}
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

	//XDrawImageString(dis, win, gc, 10, 10, "pixmap", 6);

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

// 0 - exit, 1 - key
static int wait_key(int keycode)
{
	XEvent event;
	while (1) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == wmDeleteMessage) {
			return 0;
		}

		if (event.type==KeyPress && event.xkey.keycode==keycode) {
			return 1;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
		}
	}
}

static int test_loop()
{
	XEvent event;

	while(XPending(dis)) {
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

	while(1) {
		XFlush(dis);

		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);
		tv.tv_usec = 500000;
		tv.tv_sec = 0;
		num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds == 0) {
			//gettimeofday(&tv, NULL);
			//printf("%d %d\n", tv.tv_sec, tv.tv_usec);
			printf("%d\n", getuseconds());
			//printf("."); fflush(stdout);
		}

		if (!test_loop()) {
			return 0;
		}
	}
}


static void game_loop()
{
	XEvent event;

	if (!wait_key(keyEnter)) {
		return;
	}

	drawptr = draw;
	draw_win();

	printf("main loop\n");

	time_loop();
	return;

	while(1) {
		XNextEvent(dis, &event);
		//printf("event.type=%x\n", event.type);

		if (event.type==ClientMessage && event.xclient.data.l[0] == wmDeleteMessage) {
			break;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
			//printf("Expose\n");
		}

		if (event.type==KeyPress && event.xkey.keycode==keyESC) {
			break;
			//printf("KeyPress\n");
		}

// l:64 r:66 u:62 d:68

		if (event.type==KeyPress) {
			printf("KeyPress: %x\n", event.xkey.keycode);
		}

		//if (event.type==KeyRelease && event.xkey.keycode==keycode) {
			//printf("KeyRelease\n");
		//}

		if (event.type==ButtonPress) {
			draw_win();
			//break;
		}
	}
}


int main(int argc, char *argv[])
{
	KeySym key;
	char text[255];
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
	seconds0 = tv.tv_sec;
	srand(tv.tv_sec & tv.tv_usec);

	game = game_create(FIELD_W, FIELD_H);
	if ( !game ) {
		fprintf(stderr, "game_create - out of memory\n");
		exit(1);
	}

	if (!init_win()) {
		goto main_exit;
	}

	game_loop();

	//printf("%x, %x\n", game, game->cells);
	game_destroy(game);

main_exit:
	XFreeGC(dis, gc);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);

	return 0;
}
