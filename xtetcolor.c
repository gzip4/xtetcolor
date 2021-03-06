#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "game.h"

#define VERSION "0.97"

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
GC gc, gc2;
unsigned long black, white;
Atom wmDeleteMessage;
Font font1;
int font1cw;
unsigned int keyESC, keyEnter, keyLeft, keyRight, keySpace, keyUp, keyDown;
void (*drawptr)(Drawable dr, int ww, int wh);
game_t *game;
char *user;
char *score_filename;

struct _hiscores {
	unsigned int score;
	char name[16];
} hiscores[30];

static void read_scores();
static void write_scores();
static void fix_scores();


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


static void draw_static(Drawable dr, int ww, int wh)
{
	const int cw = ww / 2, ch = wh / 2;
	const int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP;
	int topn = top + 180;
	int i;
	char *str[] = {
		"ENTER      - Start / reset",
		"Left/Right - Move figure",
		"Up/Down    - Rotate figure",
		"Space      - Drop figure",
		"P          - Pause / resume",
		"ESC        - Leave game",
		"xtetcolor v" VERSION ", MIT (c) gzip4, 2018",
		"https://github.com/gzip4/xtetcolor",
		"XTETCOLOR"
	};
	char buff[256], name[17];

	XSetForeground(dis, gc, white);
	XDrawString(dis, dr, gc, left-260, topn,     str[0], strlen(str[0]));
	XDrawString(dis, dr, gc, left-260, topn+=20, str[1], strlen(str[1]));
	XDrawString(dis, dr, gc, left-260, topn+=20, str[2], strlen(str[2]));
	XDrawString(dis, dr, gc, left-260, topn+=20, str[3], strlen(str[3]));
	XDrawString(dis, dr, gc, left-260, topn+=20, str[4], strlen(str[4]));
	XDrawString(dis, dr, gc, left-260, topn+=20, str[5], strlen(str[5]));

	XSetForeground(dis, gc2, white);
	XSetBackground(dis, gc2, black);
	XDrawString(dis, dr, gc2, left-260, topn+=60, str[6], strlen(str[6]));
	XDrawString(dis, dr, gc2, left-260, topn+=20, str[7], strlen(str[7]));

	XSetForeground(dis, gc, black);
	XDrawString(dis, dr, gc, left-262, top+16, str[8], strlen(str[8]));
	XSetForeground(dis, gc, 0xd0d000);
	XDrawString(dis, dr, gc, left-260, top+18, str[8], strlen(str[8]));
	XDrawString(dis, dr, gc, left-259, top+18, str[8], strlen(str[8]));

	// scores
	XSetForeground(dis, gc2, 0xff2000);
	for (i = 0; i < 30; ++i) {
		if (!hiscores[i].name[0]) break;
		memset(name, 0, sizeof(name));
		memcpy(name, hiscores[i].name, 16);
		snprintf(buff, sizeof(buff), "%2d. [ %-16s ] _ _ _ _ _ %d", i+1, name, hiscores[i].score);
		XDrawString(dis, dr, gc2, left+w+32, (top+18)+i*16, buff, strlen(buff));
	}
}


static void default_draw(Drawable dr, int ww, int wh)
{
	const int cw = ww / 2, ch = wh / 2;
	const int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP, h = FIELD_HP;

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);

	XSetForeground(dis, gc, black);
	XSetBackground(dis, gc, 0xd0d0d0);
	XDrawImageString(dis, dr, gc, cw - font1cw*10, ch, "Press ENTER to start", 20);

	draw_static(dr, ww, wh);
}


static void draw_paused(Drawable dr, int ww, int wh)
{
	const int cw = ww / 2, ch = wh / 2;
	const int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP, h = FIELD_HP;

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);

	XSetForeground(dis, gc, black);
	XSetBackground(dis, gc, 0xd0d0d0);
	XDrawImageString(dis, dr, gc, left+20, top+30, "|| PAUSE", 8);

	draw_static(dr, ww, wh);
}


static void draw(Drawable dr, int ww, int wh)
{
	const int cw = ww / 2, ch = wh / 2;
	const int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, w = FIELD_WP, h = FIELD_HP, cellw = CELL_WP;
	const int colors[] = { 0xff0000, 0xff00, 0xff, 0xff00ff, 0xffff00, 0xffff, 0, 0 };
	int i, j;
	cell_t cell;
	cell_t cells[FIELD_W * FIELD_H];
	char buff[32];
	int buffsz;

	game_field(game, &cells[0]);

	XSetForeground(dis, gc, 0xffff);
	XDrawRectangle(dis, dr, gc, left-1, top-1, w+2, h+2);
	XSetForeground(dis, gc, 0xd0d0d0);
	XFillRectangle(dis, dr, gc, left, top, w+1, h+1);

	for (j = 0; j < FIELD_H; ++j) {
		for (i = 0; i < FIELD_W; ++i) {
			cell = cells[j * FIELD_W + i];
			if (cell == EMPTY_CELL) continue;
			XSetForeground(dis, gc, colors[(int)cell]);
			XFillRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
			XSetForeground(dis, gc, black);
			XDrawRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
		}
	}

	if (game && game->game_over) {
		XSetForeground(dis, gc, 0xff5050);
		XSetBackground(dis, gc, 0x303030);
		XDrawImageString(dis, dr, gc, left-260, top+118, "GAME OVER (Press ENTER)", 23);
	}

	if (game) {
		XSetForeground(dis, gc, white);
		XSetBackground(dis, gc, 0x303030);
		buffsz = snprintf(buff, 32, "LEVEL: %d", game->level + 1);
		XDrawImageString(dis, dr, gc, left-260, top+58, buff, buffsz);
		buffsz = snprintf(buff, 32, "SCORE: %d", game->score);
		XDrawImageString(dis, dr, gc, left-260, top+88, buff, buffsz);
	}

	draw_static(dr, ww, wh);
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

	XFlush(dis);
}


static void draw_combinations(Drawable dr, int ww, int wh, const char *coords, int n)
{
	const int cw = ww / 2, ch = wh / 2;
	const int left = cw - FIELD_WP/2, top = ch - FIELD_HP/2, cellw = CELL_WP;
	int i, j, k;

	for (k = 0; k < n; k += 2) {
		i = coords[k];
		j = coords[k + 1];
		XSetForeground(dis, gc, black);
		XFillRectangle(dis, dr, gc, left+i*cellw, top+j*cellw, cellw, cellw);
	}

	XFlush(dis);
}


static void draw_win_cb(const char *coords, int n)
{
	int w, h;

	get_window_size(&w, &h);
	draw_combinations(win, w, h, coords, n);

	usleep(80000);	// XXX: blink pause

	draw_win();
}


// 0 - exit, 1 - key, if keycode=0 - anykey
static int wait_key(unsigned int keycode)
{
	XEvent event;
	while (1) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == (long) wmDeleteMessage) {
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
	int rc, msec = 1000 - game->level * 50;
	if (since_last_time(msec < 100 ? 100 : msec)) {
		rc = game_tick(game);
		draw_win();
		if (rc == 1) {		// land figure
			// wait and gen new one
			while (!since_last_time(50)) {
				usleep(1000);
			}
			game_tick(game);
			draw_win();
		}
	}
}


static int x11_loop()
{
	XEvent event;

	check_for_tick();

	while (XPending(dis)) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == (long) wmDeleteMessage) {
			return 0;
		}

		if (event.type==Expose && event.xexpose.count==0) {
			draw_win();
		}

		if (event.type==KeyPress && event.xkey.keycode==keyESC) {
			return 0;
		}

		if (event.type==KeyPress && event.xkey.keycode==keyEnter) {
			game->game_over = 1;
			draw_win();
		}

		if (event.type==KeyPress && event.xkey.keycode==keyLeft) {
			if (game_move_l(game)) {
				draw_win();
			}
		}

		if (event.type==KeyPress && event.xkey.keycode==keyRight) {
			if (game_move_r(game)) {
				draw_win();
			}
		}

		if (event.type==KeyPress && event.xkey.keycode==keyUp) {
			if (game_move_rot(game, 1)) {
				draw_win();
			}
		}

		if (event.type==KeyPress && event.xkey.keycode==keyDown) {
			if (game_move_rot(game, 0)) {
				draw_win();
			}
		}

		if (event.type==KeyPress && event.xkey.keycode==keySpace) {
			if (game_move_down(game)) {
				draw_win();
			}
		}

		// XXX: key P
		if (event.type==KeyPress && event.xkey.keycode==0x21) {
			drawptr = draw_paused;
			draw_win();
			if (!wait_key(0x21)) {
				return 0;
			}
			drawptr = draw;
			draw_win();
			last_time = getmseconds();
		}

		if (event.type==KeyPress) {
			//printf("KeyPress: %x\n", event.xkey.keycode);
		}

		check_for_tick();
	}

	return 1;
}


static int time_loop()
{
	int x11_fd;
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
		select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

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

	if (!wait_key(keyEnter)) {
		return;
	}

again:
	game = game_create(FIELD_W, FIELD_H);
	if ( !game ) {
		perror("game_create");
		exit(1);
	}

	game->combi_cb = draw_win_cb;

	drawptr = draw;
	draw_win();

	if (!time_loop()) {
		return;
	}

	read_scores();
	fix_scores();
	write_scores();
	draw_win();

	// game needed for draw function, destroy after keypress
	if (!wait_key(keyEnter)) {
		return;
	}

	game_destroy(game); game = NULL;
	goto again;
}


static int init_win()
{
	XEvent event;

	win = XCreateSimpleWindow(dis, root, 0, 0, WINDOW_W, WINDOW_H, 0, None, None);
	XSetWMProtocols(dis, win, &wmDeleteMessage, 1);
	XSetStandardProperties(dis, win, "X Tetcolor", "HI!", None, NULL, 0, NULL);
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);
	size_hints();

	/* transparent window - avoid flickering */
	XSetWindowBackground(dis, win, None);
	XSetWindowBackgroundPixmap(dis, win, None);

	gc  = XCreateGC(dis, win, 0, 0);
	gc2 = XCreateGC(dis, win, 0, 0);

	XSetBackground(dis, gc, black);
	XSetForeground(dis, gc, white);
	XClearWindow(dis, win);
	XMapRaised(dis, win);
	XFlush(dis);

	while (XPending(dis)) {
		XNextEvent(dis, &event);

		if (event.type==ClientMessage && event.xclient.data.l[0] == (long) wmDeleteMessage) {
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


static int init_fonts()
{
	int i = 0;
	XFontStruct *fs;
	char *fname;
	char *fnames[] = {
		"-xos4-terminus-medium-*-*-*-16-*-*-*-*-*-iso10646-1",
		"-xos4-terminus-medium-*-*-*-16-*-*-*-*-*-*-*",
		"-misc-fixed-medium-*-*-*-18-*-*-*-*-*-iso10646-1",
		"-misc-fixed-medium-*-*-*-18-*-*-*-*-*-*-*",
		"9x18",
		"fixed",
		NULL };

	while ( (fname = fnames[i++]) ) {
		fs = XLoadQueryFont(dis, fname);
		if (!fs) continue;
		font1 = fs->fid;
		font1cw = fs->max_bounds.rbearing - fs->min_bounds.lbearing;	// symbol width
		XSetFont(dis, gc,  font1);
		XSetFont(dis, gc2, XLoadFont(dis, "fixed"));
		XFreeFont(dis, fs);
		return 1;
	}

	return 0;
}


static void usage()
{
	printf("XTetColor v" VERSION ", gzip4ever@gmail.com (https://github.com/gzip4/xtetcolor)\n");
	printf("MIT License. Copyright (c) gzip4, 2018\n");
	printf("\t-u user\t\tset user name\n");
	printf("\t-f file\t\tset hi-scores file name\n");
	printf("\t-h\t\thelp\n");
}


static void read_scores()
{
	int fd, rv;
	char tag[4];

	memset(&hiscores[0], 0, sizeof(hiscores));
	if (!score_filename) return;
	if ((fd = open(score_filename, O_RDONLY)) == -1) return;

	rv = read(fd, &tag[0], 4);
	if ((rv != 4) || (memcmp(tag, "XTC", 4) != 0)) {
		fprintf(stderr, "corrupted score file\n");
		goto read_score_exit;
	}

	rv = read(fd, &hiscores[0], sizeof(hiscores));
	if (rv != sizeof(hiscores)) {
		memset(&hiscores[0], 0, sizeof(hiscores));
		fprintf(stderr, "corrupted score file\n");
		goto read_score_exit;
	}

read_score_exit:
	close(fd);
}


static void write_scores()
{
	int fd, rv, i;

	if (!score_filename) return;
	if ((fd = open(score_filename, O_WRONLY | O_TRUNC | O_CREAT, 0664)) == -1) return;

	write(fd, "XTC", 4);
	write(fd, &hiscores[0], sizeof(hiscores));
	close(fd);
}


static void fix_scores()
{
	int i;

	if (!user || !game) return;

	for (i = 0; i < 30; ++i) {
		if (game->score > hiscores[i].score) {
			memmove(&hiscores[i+1], &hiscores[i], sizeof(struct _hiscores)*(30-i-1));
			hiscores[i].score = game->score;
			memset(hiscores[i].name, 0, 16);
			strncpy(hiscores[i].name, user, 16);
			break;
		}
	}
}


static int parse_cmdline(int *argc, char **argv[])
{
	int ch, rc = 0;
	while ((ch = getopt(*argc, *argv, "hu:f:")) != -1) {
		switch (ch) {
		case 'u':
			user = optarg;
			break;
		case 'f':
			score_filename = optarg;
			break;
		case 'h':
		case '?':
		default:
			usage();
			rc = 1;
		}
	}
	*argc -= optind;
	*argv += optind;

	return rc;
}


int main(int argc, char *argv[])
{
	struct timeval tv;
	char buff[PATH_MAX];
	int rc = parse_cmdline(&argc, &argv);
	if (rc) return rc;

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
	keyLeft	= XKeysymToKeycode(dis, XK_Left);
	keyRight= XKeysymToKeycode(dis, XK_Right);
	keySpace= XKeysymToKeycode(dis, XK_space);
	keyUp	= XKeysymToKeycode(dis, XK_Up);
	keyDown	= XKeysymToKeycode(dis, XK_Down);

	gettimeofday(&tv, NULL);
	const_seconds0 = tv.tv_sec;
	const_useconds0 = tv.tv_usec;
#if defined(__NetBSD__)
	srandom(tv.tv_sec & tv.tv_usec);
#else
	srand(tv.tv_sec & tv.tv_usec);
#endif

	if (!user) user = getenv("XTET_USER");
	if (!user) user = getenv("USER");

	if (!score_filename) {
		snprintf(buff, PATH_MAX, "%s/.xtetcolor.score", getenv("HOME"));
		score_filename = buff;
	}

	read_scores();

	if (!init_win()) {
		goto main_exit;
	}
	if (!init_fonts()) {
		goto main_exit;
	}

	game_loop();

main_exit:
	XFreeGC(dis, gc);
	XFreeGC(dis, gc2);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);

	return 0;
}
