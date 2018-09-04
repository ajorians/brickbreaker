
/*

  A linux clone of a windows game with the same name.
  Written from scratch in 4 days.

  License: GPLv2

  paxed@alt.org

  20080502



  TODO:
  -when hovering mouse over a block in the map, show the targeting bar
   towards all four directions.
  -max. # of moves per level?
  -highscore entries?
  -smooth block movement?
  -configurable keys
  -help screens
  -level editor?

 */


#include "SDL/SDL.h"
#ifndef _TINSPIRE
#include "SDL/SDL_image.h"
#include <SDL/SDL_mixer.h>
#endif

#include <time.h>
#include <stdarg.h>
#include <string.h>

#ifdef _TINSPIRE
#include "gfx/background2.h"
#include "gfx/background3.h"
#include "gfx/background4.h"
#include "gfx/background5.h"
#include "gfx/background.h"
#include "gfx/charset.h"
#include "gfx/cursor.h"
#include "gfx/explosion.h"
#include "gfx/level_bg.h"
#include "gfx/score_bg.h"
#include "gfx/targeting.h"
#include "gfx/tiles.h"
#include "gfx/window_border.h"
#endif

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof(x[0]))

#define TRUE  1
#define FALSE 0

#define boolean int


#define GAME_NAME "brickshooter"
#define GAME_VERSION "v0.04"

#define GKEY_QUIT                 SDLK_ESCAPE
#define GKEY_LEFT SDLK_LEFT
#define GKEY_RIGHT SDLK_RIGHT
#define GKEY_UP SDLK_UP
#define GKEY_DOWN SDLK_DOWN
#define GKEY_FIRE SDLK_SPACE
#define GKEY_TURN_CW 'x'
#define GKEY_TURN_CCW 'z'

SDL_Surface *screen = NULL;

Uint32 button_color;
Uint32 button_hilite_color;
Uint32 window_color;

Uint32 cursor_color;
Uint32 allow_color;
Uint32 bg_color;


#ifdef _TINSPIRE
boolean is_fullscreen = TRUE;
#else
boolean is_fullscreen = FALSE;
#endif
boolean is_paused = FALSE;
boolean debugging_level = 0;
boolean do_quit = FALSE;
#ifdef _TINSPIRE
#define SCREEN_WID (320)
#define SCREEN_HEI (240)
#define SCREEN_BPP (16)
#else
int SCREEN_WID = 800;
int SCREEN_HEI = 600;
int SCREEN_BPP = 24;
#endif

enum {
    DIR_N = 0,
    DIR_E,
    DIR_S,
    DIR_W,
    DIR_NONE
};


#define MAP_WID 10
#define MAP_HEI 10
#define MAP_BORDER_WID 3


#ifdef _TINSPIRE
int TILE_WID = 16;
int TILE_HEI = 16;
int TILE_SPACE = 0;
#else
int TILE_WID = 32;
int TILE_HEI = 32;
int TILE_SPACE = 2;
#endif

#ifdef _TINSPIRE
int SCORE_X = 32+8;
int SCORE_Y = 16+8;

int SCORE_BG_X = 16;
int SCORE_BG_Y = 16;
#else
int SCORE_X = 64+16;
int SCORE_Y = 32+16;

int SCORE_BG_X = 32;
int SCORE_BG_Y = 32;
#endif

#ifdef _TINSPIRE
#define LEVEL_POS_X (SCREEN_WID - 50)
#define LEVEL_POS_Y (16+8)

#define LEVELTXT_BG_X (SCREEN_WID - 98)
#define LEVELTXT_BG_Y (16)
#else
int LEVEL_POS_X = SCREEN_WID - 100;
int LEVEL_POS_Y = 32+16;

int LEVELTXT_BG_X = SCREEN_WID - 196;
int LEVELTXT_BG_Y = 32;
#endif


#define MAX_FLOOD_HEAP 128


int anim_delay = 1;
int block_move_speed = 2; /* 0, 1, 2 */

boolean audio_use = TRUE;
int audio_rate = 44100;
Uint16 audio_format = AUDIO_S16SYS;
int audio_num_channels = 1;
int audio_buffers = 4096;

enum soundtypes {
    SND_EXPLODE = 0,
    SND_GAMEOVER,
    SND_BLOCKHIT,
    SND_NEXTLEVEL,
    SND_BLOCKOUT,
    SND_FINISHSET,
    SND_CANTMOVE,
    SND_SHOOTBLOCK,
    SND_STARTUP,
    NUM_SOUNDS
};

#ifndef _TINSPIRE
Mix_Chunk *audio_sound[NUM_SOUNDS];

int audio_channels[NUM_SOUNDS];

char *audio_sound_files[NUM_SOUNDS] = {
    "snd/explode.wav",
    "snd/gameover.wav",
    "snd/blockhit.wav",
    "snd/nextlevel.wav",
    "snd/blockout.wav",
    "snd/finishset.wav",
    "snd/cantmove.wav",
    "snd/shootblock.wav",
    "snd/startup.wav"
};
#endif


const char *charset_chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,;:'|-!\"#$%&/()=?+\\*_<>@ ";

SDL_Surface *charset_img = NULL;
#ifdef _TINSPIRE
int charset_charwid = 8;
#else
int charset_charwid = 16;
#endif

SDL_Surface *background = NULL;
SDL_Surface *tilegfx = NULL;
SDL_Surface *cursor_img = NULL;
SDL_Surface *target_pointer_img = NULL;
SDL_Surface *score_bg_img = NULL;
SDL_Surface *leveltxt_bg_img = NULL;
SDL_Surface *window_border = NULL;

struct mapcell {
    int block;
    int dir;

    int xofs;
    int yofs;

    int xadj;
    int yadj;

    int steps;
};


struct mapcell map[MAP_WID][MAP_HEI];
struct mapcell border_n[MAP_BORDER_WID][MAP_WID];
struct mapcell border_s[MAP_BORDER_WID][MAP_WID];
struct mapcell border_w[MAP_BORDER_WID][MAP_HEI];
struct mapcell border_e[MAP_BORDER_WID][MAP_HEI];

int cursor_pos = MAP_WID / 2;
int cursor_side = DIR_N;
int cursor_shown = TRUE;

int used_tiletypes[20];
int used_tiletypes_count = 0;

int score = 0;
int level = 1;
int moves = 0;

long level_file_len = -1;
char *level_file = NULL;

char *def_levelset = "aigyptos";

char levelset_fname[64];
char levelset_name[64];

int random_levelset = -1;


int program_state = 0; /* 0 = main menu, 1 = playing */



int EXPLOSION_FRAMES = -1;
struct explosion_anim {
    int x,y;
    int state;
};
SDL_Surface *explosion_img = NULL;

#define MAX_EXPLOSIONS 32
struct explosion_anim explosions[MAX_EXPLOSIONS];


void
play_sound(int snd)
{
#ifndef _TINSPIRE
    if (audio_use)
	audio_channels[snd] = Mix_PlayChannel(-1, audio_sound[snd], 0);
#endif
}


void
explosions_init()
{
    int i;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
	explosions[i].x = explosions[i].y = explosions[i].state = -1;

    if (explosion_img)
	EXPLOSION_FRAMES = explosion_img->h / explosion_img->w;
    else EXPLOSION_FRAMES = -1;
}

int
explosions_active()
{
    int i, count = 0;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
	if (explosions[i].state != -1)
	    count++;
    return count;
}

void
explosions_add(int x, int y)
{
    int i;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
	if (explosions[i].state == -1) {
	    explosions[i].x = x;
	    explosions[i].y = y;
	    explosions[i].state = EXPLOSION_FRAMES-1;
	    return;
	}
}

void
explosions_draw(SDL_Surface *surf)
{
    int i;
    SDL_Rect src, dst;

    src.w = explosion_img->w;
    src.h = src.w;

    src.x = 0;

    for (i = 0; i < MAX_EXPLOSIONS; i++)
	if (explosions[i].state >= 0) {
	    dst.x = explosions[i].x;
	    dst.y = explosions[i].y;
	    src.y = src.h*explosions[i].state;
	    SDL_BlitSurface(explosion_img, &src, surf, &dst);
	}
}

void
explosions_anim()
{
    int i;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
	if (explosions[i].state >= 0) explosions[i].state--;
}


SDL_Surface *
copysurface(SDL_Surface* surf)
{
    return SDL_ConvertSurface(surf, surf->format, surf->flags);
}

/*
void
screen_fadeout()
{
    int blockwid = 10;
    int blockhei = 10;

    int xsize = SCREEN_WID / blockwid;
    int ysize = SCREEN_HEI / blockhei;
    int x,y;

    Uint32 fadeout_color = SDL_MapRGB(screen->format, 0, 0, 0);

    SDL_Rect r;

    r.w = xsize;
    r.h = ysize;

    for (y = 0; y < (blockhei+1)/2; y++) {
	for (x = 0; x < blockwid; x++) {
	    r.y = y*ysize;
	    r.x = x*xsize;
	    r.w = xsize;
	    r.h = ysize;
	    SDL_FillRect(screen, &r, fadeout_color);

	    r.y = (blockhei-y-1)*ysize;
	    r.x = (blockwid-x-1)*xsize;
	    r.w = xsize;
	    r.h = ysize;
	    SDL_FillRect(screen, &r, fadeout_color);

	    SDL_Flip(screen);
	}
    }
}
*/

void
write_string(SDL_Surface *surf, int x, int y, char *str)
{
    int i;

    SDL_Rect src, dst;

    charset_charwid = charset_img->w / strlen(charset_chars);

    if (x < 0)
	x = (SCREEN_WID/2) - ((strlen(str)*charset_charwid)/2);

    for (i = 0; i < strlen(str); i++) {
	src.x = (strchr(charset_chars, str[i]) - charset_chars)*charset_charwid;
	src.y = 0;
	src.h = charset_img->h;
	src.w = charset_charwid;

	dst.x = x + i*charset_charwid;
	dst.y = y;

	SDL_BlitSurface(charset_img, &src, surf, &dst);

    }
}

void
draw_window(SDL_Surface *surf, int x, int y, int w, int h)
{
    SDL_Rect r, src;

    int bw, bh, i, dx,dy;

    if ((w % charset_charwid) > 0)
	w += (charset_charwid - (w % charset_charwid));
    if ((h % charset_img->h) > 0)
	h += (charset_img->h - (h % charset_img->h));

    if (!window_border) {
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	SDL_FillRect(surf, &r, window_color);
	return;
    }

    bw = (window_border->w - charset_charwid) / 2;
    bh = (window_border->h - charset_img->h) / 2;


    for (i = 0; i < w; i += charset_charwid) {
	src.x = bw;
	src.y = 0;
	src.w = charset_charwid;
	src.h = bh;
	r.x = x + i;
	r.y = y - bh;
	SDL_BlitSurface(window_border, &src, surf, &r);

	src.y = window_border->h - bh;
	r.y = y + h;
	SDL_BlitSurface(window_border, &src, surf, &r);

    }

    for (i = 0; i < h; i += charset_img->h) {
	src.x = 0;
	src.y = bh;
	src.w = bw;
	src.h = charset_img->h;
	r.x = x - bw;
	r.y = y + i;
	SDL_BlitSurface(window_border, &src, surf, &r);

	src.x = window_border->w - bw;
	r.x = x + w;
	SDL_BlitSurface(window_border, &src, surf, &r);
    }

    src.x = bw;
    src.y = bh;
    src.w = charset_charwid;
    src.h = charset_img->h;
    for (dx = 0; dx < w; dx += charset_charwid) {
	for (dy = 0; dy < h; dy += charset_img->h) {
	    r.x = x + dx;
	    r.y = y + dy;
	    SDL_BlitSurface(window_border, &src, surf, &r);
	}
    }

    /* top left */
    src.x = 0;
    src.y = 0;
    src.w = bw;
    src.h = bh;
    r.x = x - bw;
    r.y = y - bh;
    SDL_BlitSurface(window_border, &src, surf, &r);

    /* top right */
    src.x = window_border->w - bw;
    src.y = 0;
    r.x = x + w;
    SDL_BlitSurface(window_border, &src, surf, &r);

    /* bottom left */
    src.x = 0;
    src.y = window_border->h - bh;
    r.x = x - bw;
    r.y = y + h;
    SDL_BlitSurface(window_border, &src, surf, &r);

    /* bottom right */
    src.x = window_border->w - bw;
    src.y = window_border->h - bh;
    r.x = x + w;
    r.y = y + h;
    SDL_BlitSurface(window_border, &src, surf, &r);
}


/*
int
get_yn(SDL_Surface *screen, char *msg, int defval)
{
    int ok = 0;

    SDL_Rect dst, yesbtn, nobtn;

    int box_border = charset_charwid / 2;

    SDL_Event event;

    int retval = defval;

    int hilight = 0;

    char yes_str[] = "Yes ";
    char no_str[]  = " No ";

    do {

	if (strlen(msg) < 9)
	    dst.w = (9*charset_charwid) + box_border*2;
	else
	    dst.w = (strlen(msg)*charset_charwid) + box_border*2;

	dst.h = (charset_img->h * 3) + box_border*2;

	dst.x = (SCREEN_WID/2) - (dst.w/2);
	dst.y = (SCREEN_HEI/2) - (dst.h/2);

	draw_window(screen, dst.x, dst.y, dst.w, dst.h);

	write_string(screen, -1, dst.y+box_border, msg);

	yesbtn.x = dst.x + box_border;
	yesbtn.y = dst.y + charset_img->h*2+box_border;
	yesbtn.w = strlen(yes_str)*charset_charwid;
	yesbtn.h = charset_img->h;
	SDL_FillRect(screen, &yesbtn, (hilight & 1) ? button_hilite_color : button_color);

	nobtn.x = dst.x+box_border+5*charset_charwid;
	nobtn.w = strlen(no_str)*charset_charwid;
	nobtn.y = yesbtn.y;
	nobtn.h = yesbtn.h;
	SDL_FillRect(screen, &nobtn, (hilight & 2) ? button_hilite_color : button_color);

	write_string(screen, dst.x+box_border, dst.y+charset_img->h*2+box_border, yes_str);
	write_string(screen, dst.x+box_border+5*charset_charwid, dst.y+charset_img->h*2+box_border, no_str);

	SDL_Flip(screen);

	SDL_Delay(25);

	while (SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT:
		ok = 1;
		break;
	    case SDL_KEYDOWN:
		switch (event.key.keysym.sym) {
		default: break;
		case 'y':
		case 'Y':
		    retval = TRUE;
		    ok = 1;
		    break;
		case 'n':
		case 'N':
		    retval = FALSE;
		    ok = 1;
		    break;
		case SDLK_ESCAPE:
		    ok = 1;
		    break;
		case SDLK_LEFT:
		    hilight = 1;
		    break;
		case SDLK_RIGHT:
		    hilight = 2;
		    break;
		case SDLK_RETURN:
		case SDLK_SPACE:
		case SDLK_KP_ENTER:
		    if (hilight == 1 || hilight == 2) {
			retval = (hilight == 1) ? TRUE : FALSE;
			ok = 1;
		    }
		    break;
		}
		break;
	    case SDL_MOUSEMOTION:
		hilight = 0;
		if (event.motion.x >= yesbtn.x && event.motion.y >= yesbtn.y &&
		    event.motion.x <= yesbtn.x+yesbtn.w && event.motion.y <= yesbtn.y+yesbtn.h)
		    hilight = 1;
		if (event.motion.x >= nobtn.x && event.motion.y >= nobtn.y &&
		    event.motion.x <= nobtn.x+nobtn.w && event.motion.y <= nobtn.y+nobtn.h)
		    hilight = 2;
		break;
	    case SDL_MOUSEBUTTONUP:
	    case SDL_MOUSEBUTTONDOWN:
		hilight = 0;
		if (event.button.x >= yesbtn.x && event.button.y >= yesbtn.y &&
		    event.button.x <= yesbtn.x+yesbtn.w && event.button.y <= yesbtn.y+yesbtn.h) {
		    hilight = 1;
		    retval = TRUE;
		    if (event.type == SDL_MOUSEBUTTONUP) ok = 1;
		}
		if (event.button.x >= nobtn.x && event.button.y >= nobtn.y &&
		    event.button.x <= nobtn.x+nobtn.w && event.button.y <= nobtn.y+nobtn.h) {
		    hilight = 2;
		    retval = FALSE;
		    if (event.type == SDL_MOUSEBUTTONUP) ok = 1;
		}
		break;
	    }
	}

    } while (!ok);

    return retval;
}
*/

void
show_popup(SDL_Surface *screen, char *msg, long timeout)
{
    int ok = 0;

    SDL_Rect dst, btn;

    int box_border = charset_charwid / 2;

    SDL_Event event;

    int hilight = 0;
    int got_input = 0;

    char ok_str[] = " OK ";

    time_t secs = time(NULL);

    SDL_Surface *bg_surf = copysurface(screen);

    do {

	if (strlen(msg) < 4)
	    dst.w = (4*charset_charwid) + box_border*2;
	else
	    dst.w = (strlen(msg)*charset_charwid) + box_border*2;

	dst.h = (charset_img->h * 3) + box_border*2;

	dst.x = (SCREEN_WID/2) - (dst.w/2);
	dst.y = (SCREEN_HEI/2) - (dst.h/2);

	if (bg_surf)
	    SDL_BlitSurface(bg_surf, NULL, screen, NULL);

	draw_window(screen, dst.x, dst.y, dst.w, dst.h);

	write_string(screen, dst.x+box_border, dst.y+box_border, msg);


	btn.x = (SCREEN_WID/2) - ((strlen(ok_str)*charset_charwid)/2);
	btn.y = dst.y + charset_img->h*2+box_border;
	btn.w = strlen(ok_str)*charset_charwid;
	btn.h = charset_img->h;
	SDL_FillRect(screen, &btn, (hilight & 1) ? button_hilite_color : button_color);

	write_string(screen, -1, dst.y+charset_img->h*2+box_border, ok_str);

	SDL_Flip(screen);

	SDL_Delay(25);

	if (timeout > 0) {
	    if (time(NULL) >= secs + timeout) ok = 1;
	}


	while (SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT:
		ok = 1;
		break;
	    case SDL_KEYDOWN:
	    case SDL_KEYUP:
		if (event.key.keysym.sym == SDLK_SPACE ||
		    event.key.keysym.sym == SDLK_KP_ENTER ||
		    event.key.keysym.sym == SDLK_RETURN ||
		    event.key.keysym.sym == SDLK_ESCAPE) {
		    if (event.type == SDL_KEYUP && got_input) ok = 1;
		    else got_input = 1;
		} else got_input = 0;
		break;
	    case SDL_MOUSEMOTION:
		hilight = 0;
		if (event.motion.x >= btn.x && event.motion.y >= btn.y &&
		    event.motion.x <= btn.x+btn.w && event.motion.y <= btn.y+btn.h)
		    hilight = 1;
		break;
	    case SDL_MOUSEBUTTONUP:
	    case SDL_MOUSEBUTTONDOWN:
		hilight = 0;
		if (event.motion.x >= btn.x && event.motion.y >= btn.y &&
		    event.motion.x <= btn.x+btn.w && event.motion.y <= btn.y+btn.h) {
		    hilight = 1;
		    if (event.type == SDL_MOUSEBUTTONUP) ok = 1;
		}
		break;
	    }
	}

    } while (!ok);

    if (bg_surf) {
	SDL_BlitSurface(bg_surf, NULL, screen, NULL);
	SDL_FreeSurface(bg_surf);
    }
}


int
show_menu(SDL_Surface *screen, char *title, int n_options, char *optiontxts[], int default_option)
{
    int ok = 0, i, tmpi;

    SDL_Rect dst, btn;

    int box_border = charset_charwid / 2;

    SDL_Event event;

    int hilight;
    int got_input;
    int min_wid;

    SDL_Surface *bg_surf = copysurface(screen);

    if (default_option < 0) default_option = 0;
    else if (default_option >= n_options) default_option = n_options;

    hilight = default_option;
    got_input = 0;

    min_wid = strlen(title);

    for (i = 0; i < n_options; i++) {
	tmpi = strlen(optiontxts[i]);
	if (tmpi > min_wid) min_wid = tmpi;
    }


    do {

	dst.w = (min_wid*charset_charwid) + box_border*2;

	dst.h = (charset_img->h * (2 + n_options)) + box_border*2;

//#ifdef _TINSPIRE
//	dst.x = 20;
//	dst.y = 20;
//#else
	dst.x = (SCREEN_WID/2) - (dst.w/2);
	dst.y = (SCREEN_HEI/2) - (dst.h/2);
//#endif

	if (bg_surf)
	    SDL_BlitSurface(bg_surf, NULL, screen, NULL);

	draw_window(screen, dst.x, dst.y, dst.w, dst.h);

	write_string(screen, -1, dst.y+box_border, title);

	for (i = 0; i < n_options; i++) {
//#ifdef _TINSPIRE
//#else
	    btn.x = (SCREEN_WID/2) - ((min_wid*charset_charwid)/2);
//#endif
	    btn.y = dst.y + charset_img->h*(2+i)+box_border;
	    btn.w = min_wid*charset_charwid;
	    btn.h = charset_img->h;
	    SDL_FillRect(screen, &btn, (hilight == i) ? button_hilite_color : button_color);

	    write_string(screen, -1, dst.y+charset_img->h*(2+i)+box_border, optiontxts[i]);

	}

	SDL_Flip(screen);

	SDL_Delay(25);

	while (SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT:
		hilight = -1;
		do_quit = TRUE;
		ok = 1;
		break;
	    case SDL_KEYDOWN:
	    case SDL_KEYUP:
		switch (event.key.keysym.sym) {
		default: got_input = 0; break;
		case SDLK_SPACE:
		case SDLK_KP_ENTER:
		case SDLK_RETURN:
		    if (event.type == SDL_KEYUP && got_input &&
			hilight >= 0 && hilight < n_options) ok = 1;
		    else got_input = 1;
		    break;
		case SDLK_ESCAPE:
		    if (event.type == SDL_KEYUP) {
			hilight = -1;
			ok = 1;
		    }
		    break;
		case SDLK_UP:
		    if (event.type == SDL_KEYUP) {
			if (hilight > 0) hilight--;
			else hilight = 0;
		    }
		    got_input = 0;
		    break;
		case SDLK_DOWN:
		    if (event.type == SDL_KEYUP) {
			if (hilight < n_options-1) hilight++;
			else hilight = n_options-1;
		    }
		    got_input = 0;
		    break;
		}
		break;
	    case SDL_MOUSEMOTION:
		for (i = 0; i < n_options; i++) {
		    int minx = ((SCREEN_WID/2) - ((min_wid*charset_charwid)/2));
		    int miny = (((SCREEN_HEI/2) - ((charset_img->h * (2 + n_options))/2 + box_border)) + charset_img->h*(2+i)) + box_border;
		    int wid = (min_wid*charset_charwid);
		    if ((event.motion.x >= minx) &&
			(event.motion.y >= miny) &&
			(event.motion.x <= minx+wid) &&
			(event.motion.y <= miny+charset_img->h)) {
			hilight = i;
			break;
		    }
		}
		break;
	    case SDL_MOUSEBUTTONUP:
	    case SDL_MOUSEBUTTONDOWN:
		for (i = 0; i < n_options; i++) {
		    int minx = ((SCREEN_WID/2) - ((min_wid*charset_charwid)/2));
		    int miny = (((SCREEN_HEI/2) - ((charset_img->h * (2 + n_options))/2 + box_border)) + charset_img->h*(2+i)) + box_border;
		    int wid = (min_wid*charset_charwid);
		    if ((event.motion.x >= minx) &&
			(event.motion.y >= miny) &&
			(event.motion.x <= minx+wid) &&
			(event.motion.y <= miny+charset_img->h)) {
			hilight = i;
			if (event.type == SDL_MOUSEBUTTONDOWN) got_input = 1;
			else if (event.type == SDL_MOUSEBUTTONUP && got_input) ok = 1;
			break;
		    }
		}
		break;
	    }
	}

    } while (!ok);

    if (bg_surf) {
	SDL_BlitSurface(bg_surf, NULL, screen, NULL);
	SDL_FreeSurface(bg_surf);
    }

    return hilight;
}


void
show_gameover(SDL_Surface *screen, int state)
{
    int ok = 0;

    int w = SCREEN_WID*2 / 3;
    int h = SCREEN_HEI*2 / 3;

    int x = (SCREEN_WID/2) - (w/2);
    int y = (SCREEN_HEI/2) - (h/2);

    SDL_Event event;

    char buf[64];

    int mbtn = 0;

    SDL_Surface *bg_surf = copysurface(screen);

    do {

	if (bg_surf)
	    SDL_BlitSurface(bg_surf, NULL, screen, NULL);

	draw_window(screen, x, y, w, h);


	switch (state) {
	default:
	case 0:
	    write_string(screen, -1, x+charset_img->h, "Game Over");
	    break;
	case 1:
	    write_string(screen, -1, x+charset_img->h,   "Congrats!");
	    write_string(screen, -1, x+charset_img->h*2, "You finished the level set!");
	    break;
	case 2:
	    write_string(screen, -1, x+charset_img->h, "Prepare for next level");
	    break;
	}

	sprintf(buf, "Levelset: %s", levelset_name);
	write_string(screen, -1, x+charset_img->h*4, buf);


	sprintf(buf, "Score: %6i", score);
	write_string(screen, -1, x+charset_img->h*6, buf);

	sprintf(buf, "Level: %6i", level);
	write_string(screen, -1, x+charset_img->h*7, buf);

	sprintf(buf, "Moves: %6i", moves);
	write_string(screen, -1, x+charset_img->h*8, buf);


	SDL_Flip(screen);

	SDL_Delay(25);

	while (SDL_PollEvent(&event)) {
	    switch(event.type) {
	    case SDL_QUIT:
		ok = 1;
		break;
	    case SDL_KEYUP:
		ok = 1;
		break;
	    case SDL_MOUSEBUTTONDOWN:
		mbtn = 1;
	    case SDL_MOUSEBUTTONUP:
		if (mbtn == 1)
		    ok = 1;
		else mbtn = 0;
		break;
	    default:
		break;
	    }
	}

    } while (!ok);

    if (bg_surf) {
	SDL_BlitSurface(bg_surf, NULL, screen, NULL);
	SDL_FreeSurface(bg_surf);
    }

}



void
print_end_of_game_info()
{
    printf("Your score: %i\n", score);
    printf("Level: %i\n", level);
    printf("Moves: %i\n", moves);
}


int
random_used_tiletype()
{
    if (used_tiletypes_count)
	return (used_tiletypes[rand() % used_tiletypes_count]);
    else return 0;
}

void
pline(int lvl, char *fmt, ...)
{
    if (lvl < debugging_level) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
    }
}


void
borders_init()
{
    int x,y,z;

    used_tiletypes_count = 0;

    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++) {
	    int block = map[x][y].block;
	    if (block) {
		int exists = 0;
		for (z = 0; z < used_tiletypes_count; z++) {
		    if (used_tiletypes[z] == block) exists = 1;
		}
		if (!exists) {
		    used_tiletypes[used_tiletypes_count] = block;
		    used_tiletypes_count++;
		}
	    }
	}

    memset(border_n, 0, sizeof(border_n));
    memset(border_s, 0, sizeof(border_s));
    memset(border_e, 0, sizeof(border_e));
    memset(border_w, 0, sizeof(border_w));

    for (x = 0; x < MAP_BORDER_WID; x++) {
	for (y = 0; y < MAP_WID; y++) {
	    border_n[x][y].dir = border_s[x][y].dir = DIR_NONE;
	    border_n[x][y].block = border_s[x][y].block = 1;
	}
	for (y = 0; y < MAP_HEI; y++) {
	    border_w[x][y].dir = border_e[x][y].dir = DIR_NONE;
	    border_w[x][y].block = border_e[x][y].block = 1;
	}
    }

    if (used_tiletypes_count) {
	for (x = 0; x < MAP_BORDER_WID; x++) {
	    for (y = 0; y < MAP_WID; y++) {
		border_n[x][y].block = random_used_tiletype();
		border_s[x][y].block = random_used_tiletype();
	    }
	    for (y = 0; y < MAP_HEI; y++) {
		border_w[x][y].block = random_used_tiletype();
		border_e[x][y].block = random_used_tiletype();
	    }
	}
    }
}



void
map_set_block(int x, int y, int tile, int dir)
{
    map[x][y].block = tile;
    map[x][y].dir = dir;
    map[x][y].xofs = map[x][y].yofs = map[x][y].xadj = map[x][y].yadj = map[x][y].steps = 0;
}



void
map_init()
{
    int x, y;

    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++)
	    map_set_block(x,y, 0, DIR_NONE);
}

int
map_corners_full()
{
    if ((map[0][0].block) && (map[0][MAP_HEI-1].block) &&
	(map[MAP_WID-1][MAP_HEI-1].block) && (map[MAP_WID-1][0].block))
	return 1;
    return 0;
}

int
map_is_full()
{
    int x, y;
    for (x = 0; x < MAP_WID; x++) {
	if (!(map[x][0].block)) return 0;
	if (!(map[x][MAP_HEI-1].block)) return 0;
    }
    for (y = 0; y < MAP_HEI; y++) {
	if (!(map[0][y].block)) return 0;
	if (!(map[MAP_WID-1][y].block)) return 0;
    }
    return 1;
}

int
map_is_empty()
{
    int x, y;
    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++)
	    if (map[x][y].block) return 0;
    return 1;
}

void
map_mirror_h()
{
    int x, y;
    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < ((MAP_HEI+1)/2); y++)
	    map[x][y] = map[x][MAP_HEI-1-y];
}

void
map_mirror_v()
{
    int x, y;
    for (x = 0; x < ((MAP_WID+1)/2); x++)
	for (y = 0; y < MAP_HEI; y++)
	    map[x][y] = map[MAP_WID-1-x][y];
}


void
map_generate_random(int level, int maxlevel)
{
    int num_diff_blocks = rand() % (2 + (int)((level*6)/maxlevel));

    int num_blocks = rand() % (3 + (int)((level*15)/maxlevel) + (rand() % 3));

    int i,x,y;
    int block;

    if (num_blocks < 3) num_blocks = 3;
    if (num_diff_blocks < 3) num_diff_blocks = 3;

    do {

	do {
	    for (i = 0; i < num_blocks; i++) {
		do {
		    block = (rand() % num_diff_blocks);
		} while (!block);

		do {
		    x = (rand() % MAP_WID);
		    y = (rand() % MAP_HEI);
		} while (map[x][y].block);

		map_set_block(x,y, block, DIR_NONE);
	    }
	} while (map_is_full() || map_corners_full());

	if (!(rand() % 4)) map_mirror_h();
	if (!(rand() % 4)) map_mirror_v();

	borders_init();

    } while (used_tiletypes_count < 2);

}



void
levelfile_free()
{
    if (level_file) {
	free(level_file);
	level_file = NULL;
	level_file_len = -1;
    }
}

int
levelfile_load(char *fname)
{
    FILE *fin;

    char fnamebuf[128];

#ifdef _TINSPIRE
    snprintf(fnamebuf, 127, "levels/%s.dat.tns", fname);
#else
    snprintf(fnamebuf, 127, "levels/%s.dat", fname);
#endif

    levelfile_free();

    fin = fopen(fnamebuf, "r");

    if (!fin) return 0;

    fseek(fin, 0, SEEK_END);
    level_file_len = ftell(fin);

    level_file = (char *)malloc(level_file_len);

    fseek(fin, 0, SEEK_SET);

    fread(level_file, sizeof(char), level_file_len, fin);

    fclose(fin);

    return 1;
}

char *
levelfile_line(long *pos)
{
    static char buf[256];

    int i = 0;

    buf[0] = 0;

    while ((i < 255) && (((*pos) + i) < level_file_len)) {
	buf[i] = level_file[*pos+i];
	if (buf[i] == '\n' || buf[i] == '\r')
	    break;
	i++;
    }
    buf[i] = 0;
    *pos += (i+2);
    return buf;
}


int
map_load(int level)
{
    static boolean first_load = 1;

    long fpos = 0;

    int lvlcount = 0;

    int x, y;

    char tmpmap[MAP_WID][MAP_HEI];

    int in_map = 0;
    int map_max_x = 0;
    int map_max_y = 0;
    int adj_x = 0;
    int adj_y = 0;

    int tmpint;
    char tmpstr[256];
    char *buf;

    boolean got_level = FALSE;

    if (random_levelset > 0) {
	if (level <= random_levelset) {
	    map_generate_random(level, random_levelset);
	    return 1;
	} else return 0;
    }


    if ((level_file_len < 0) || (level_file == NULL)) return 0;

    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++)
	    tmpmap[x][y] = 0;

    map_init();

    y = 0;

    while (fpos < level_file_len) {

	buf = levelfile_line(&fpos);

	if (buf[0] == '#') continue;
	if (buf[strlen(buf)-1] == 10)
	    buf[strlen(buf)-1] = 0;

	if (first_load) {
	    if (!in_map) {
		if (strstr(buf, "SETNAME:") == buf) {
		    char *parm = strchr(buf, ':');
		    parm++;
		    strcpy(levelset_name, parm);
		}
	    }
	}

	if (lvlcount == level) {

	    if (in_map) {
		if (!strcmp(buf, "ENDMAP")) {
		    if (map_max_y < y) map_max_y = y;
		    in_map = 0;
		    got_level = TRUE;
		} else {
		    if (y < MAP_HEI) {
			for (x = 0; x < strlen(buf); x++) {
			    if (buf[x] >= '0' && buf[x] <= '9')
				tmpmap[x][y] = buf[x]-'0'+1;
			}
			if (map_max_x < strlen(buf)) map_max_x = strlen(buf);
			y++;
		    }
		}
	    } else {
		if (!strcmp(buf, "LEVEL")) break;
		else if (!strcmp(buf, "MAP")) in_map = 1;
		else if (sscanf(buf, "BACKGROUND:%s", tmpstr) == 1) {
#if 0
		    SDL_Surface *tmpsurf = IMG_Load(tmpstr);
		    if (tmpsurf) {
			if (background) SDL_FreeSurface(background);
			background = tmpsurf;
		    }
#endif
		}
		else if (sscanf(buf, "SEED:%d", &tmpint) == 1) {
		    srand(tmpint);
		}
	    }

	} else if (!strcmp(buf, "LEVEL")) lvlcount++;
    }

    if (!got_level) return 0;

    if (map_max_x < MAP_WID) adj_x = ((MAP_WID - map_max_x) / 2);
    if (map_max_y < MAP_HEI) adj_y = ((MAP_HEI - map_max_y) / 2);

    for (x = 0; x < MAP_WID-adj_x; x++)
	for (y = 0; y < MAP_HEI-adj_y; y++) {
	    map_set_block(x+adj_x, y+adj_y, tmpmap[x][y], DIR_NONE);
	}

    first_load = 0;

    return 1;
}


int
can_fire(int *len)
{
    int ok = 0;
    int x;
    int tile;
    int length = 0;

    switch (cursor_side) {
    default:
    case DIR_N:
	for (x = 0; x < MAP_HEI; x++) {
	    tile = (map[cursor_pos][x].block);
	    if (tile) { ok = 1; break; }
	    length++;
	}
	break;
    case DIR_E:
	for (x = MAP_WID-1; x >= 0; x--) {
	    tile = (map[x][cursor_pos].block);
	    if (tile) { ok = 1; break; }
	    length++;
	}
	break;
    case DIR_S:
	for (x = MAP_HEI-1; x >= 0; x--) {
	    tile = (map[cursor_pos][x].block);
	    if (tile) { ok = 1; break; }
	    length++;
	}
	break;
    case DIR_W:
	for (x = 0; x < MAP_WID; x++) {
	    tile = (map[x][cursor_pos].block);
	    if (tile) { ok = 1; break; }
	    length++;
	}
	break;
    }

    if (ok && (length == 0)) ok = 0;

    if (ok && len) *len = length;
    return ok;
}


void
map_draw(int parts)
{
    int x, y;

#ifdef _TINSPIRE
    int map_left = (3*TILE_WID);
    int map_top = (3*TILE_HEI);
#else
    int map_left = (SCREEN_WID / 2) - ((MAP_WID / 2)*TILE_WID) - ((MAP_WID*TILE_SPACE)/2);
    int map_top = (SCREEN_HEI / 2) - ((MAP_HEI / 2)*TILE_HEI) - ((MAP_HEI*TILE_SPACE)/2);
#endif

    SDL_Rect r;
    SDL_Rect src;

    int tile;
    int dir;
    int ok;
    int length;

    char scorebuf[32];

    if (background) {
	SDL_BlitSurface(background, NULL, screen, NULL);
    } else {
	SDL_FillRect(screen, NULL, bg_color);
    }

    ok = 0;
    length = 0;

    if (!(parts & 1) && cursor_shown) {

	int adj_x = 0;
	int adj_y = 0;

	/* the bar that shows where we'll hit */
	switch (cursor_side) {
	default:
	case DIR_N:
	    ok = can_fire(&length);
	    if (ok && length) {
		r.h = length * (TILE_HEI + TILE_SPACE) - TILE_SPACE;
		r.w = TILE_WID;
		r.x = map_left + cursor_pos * (TILE_WID + TILE_SPACE);
		r.y = map_top;
		adj_y = TILE_HEI;
	    }
	    break;
	case DIR_E:
	    ok = can_fire(&length);
	    if (ok && length) {
		r.w = (length) * (TILE_WID + TILE_SPACE) - TILE_SPACE;
		r.h = TILE_HEI;
		r.y = map_top + cursor_pos * (TILE_WID + TILE_SPACE);
		r.x = map_left + (MAP_WID-length) * (TILE_WID + TILE_SPACE);
		adj_x = TILE_WID;
	    }
	    break;
	case DIR_S:
	    ok = can_fire(&length);
	    if (ok && length) {
		r.h = (length) * (TILE_HEI + TILE_SPACE) - TILE_SPACE;
		r.w = TILE_WID;
		r.x = map_left + cursor_pos * (TILE_WID + TILE_SPACE);
		r.y = map_top + (MAP_HEI-length) * (TILE_HEI + TILE_SPACE);
		adj_y = TILE_HEI;
	    }
	    break;
	case DIR_W:
	    ok = can_fire(&length);
	    if (ok && length) {
		r.w = length * (TILE_HEI + TILE_SPACE) - TILE_SPACE;
		r.h = TILE_WID;
		r.y = map_top + cursor_pos * (TILE_WID + TILE_SPACE);
		r.x = map_left;
		adj_x = TILE_WID;
	    }
	    break;
	}

	if (target_pointer_img) {

	    src.w = TILE_WID;
	    src.h = TILE_HEI;
	    src.x = cursor_side*TILE_WID;

	    if (length == 1) {
		src.y = 0;
		SDL_BlitSurface(target_pointer_img, &src, screen, &r);
	    } else if (length == 2) {
		src.y = TILE_HEI;
		SDL_BlitSurface(target_pointer_img, &src, screen, &r);
		src.y = TILE_HEI*3;
		r.x += adj_x;
		r.y += adj_y;
		SDL_BlitSurface(target_pointer_img, &src, screen, &r);
	    } else if (length) {

		int tmpi;

		src.y = TILE_HEI;
		SDL_BlitSurface(target_pointer_img, &src, screen, &r);

		src.y = TILE_HEI*2;
		for (tmpi = 0; tmpi < length - 2; tmpi++) {
		    r.x += adj_x;
		    r.y += adj_y;
		    SDL_BlitSurface(target_pointer_img, &src, screen, &r);
		}

		src.y = TILE_HEI*3;
		r.x += adj_x;
		r.y += adj_y;
		SDL_BlitSurface(target_pointer_img, &src, screen, &r);
	    }
	} else {
	    SDL_FillRect(screen, &r, allow_color);
	}
    }


    r.w = TILE_WID;
    r.h = TILE_HEI;

    src.w = TILE_WID;
    src.h = TILE_HEI;

    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++)
	    if (map[x][y].block) {
		r.x = map_left + (x * (TILE_WID + TILE_SPACE));
		r.y = map_top + (y * (TILE_HEI + TILE_SPACE));

		tile = (map[x][y].block);
		dir = (map[x][y].dir);

		r.x += map[x][y].xofs;
		r.y += map[x][y].yofs;

		src.x = dir * TILE_WID;
		src.y = (tile-1) * TILE_HEI;
		SDL_BlitSurface(tilegfx, &src, screen, &r);
	    }

    /* north border */
    for (x = 0; x < MAP_BORDER_WID; x++) {
	for (y = 0; y < MAP_WID; y++) {
	    r.x = map_left + (y * (TILE_WID + TILE_SPACE));
	    r.y = map_top - ((x+1) * (TILE_HEI + TILE_SPACE));
	    tile = border_n[x][y].block;
	    dir = border_n[x][y].dir;
	    src.x = dir * TILE_WID;
	    src.y = (tile-1) * TILE_HEI;
	    SDL_BlitSurface(tilegfx, &src, screen, &r);
	}
    }

    /* south border */
    for (x = 0; x < MAP_BORDER_WID; x++) {
	for (y = 0; y < MAP_WID; y++) {
	    r.x = map_left + (y * (TILE_WID + TILE_SPACE));
	    r.y = map_top + (MAP_HEI * TILE_HEI) + (TILE_SPACE * MAP_HEI) + (x * (TILE_HEI + TILE_SPACE));
	    tile = (border_s[x][y].block);
	    dir = (border_s[x][y].dir);
	    src.x = dir * TILE_WID;
	    src.y = (tile-1) * TILE_HEI;
	    SDL_BlitSurface(tilegfx, &src, screen, &r);
	}
    }

    /* west border */
    for (x = 0; x < MAP_BORDER_WID; x++) {
	for (y = 0; y < MAP_WID; y++) {
	    r.x = map_left - ((x+1) * (TILE_WID + TILE_SPACE));
	    r.y = map_top + (y * (TILE_HEI + TILE_SPACE));
	    tile = (border_w[x][y].block);
	    dir = (border_w[x][y].dir);
	    src.x = dir * TILE_WID;
	    src.y = (tile-1) * TILE_HEI;
	    SDL_BlitSurface(tilegfx, &src, screen, &r);
	}
    }

    /* east border */
    for (x = 0; x < MAP_BORDER_WID; x++) {
	for (y = 0; y < MAP_WID; y++) {
	    r.x = map_left + (MAP_WID*TILE_WID) + (TILE_SPACE*MAP_WID) +  (x * (TILE_WID + TILE_SPACE));
	    r.y = map_top + (y * (TILE_HEI + TILE_SPACE));
	    tile = (border_e[x][y].block);
	    dir = (border_e[x][y].dir);
	    src.x = dir * TILE_WID;
	    src.y = (tile-1) * TILE_HEI;
	    SDL_BlitSurface(tilegfx, &src, screen, &r);
	}
    }



    /* cursor */
    if (!(parts & 1) && cursor_shown) {

	r.w = TILE_WID + (TILE_SPACE*2);
	r.h = TILE_HEI + (TILE_SPACE*2);

	cursor_side = cursor_side % DIR_NONE;
	switch (cursor_side) {
	default:
	case DIR_N:
	    if (cursor_pos < 0) cursor_pos += MAP_WID;
	    cursor_pos = (cursor_pos % MAP_WID);
	    r.x = map_left + cursor_pos * (TILE_WID + TILE_SPACE);
	    r.y = map_top - (TILE_HEI + TILE_SPACE);
	    break;
	case DIR_E:
	    if (cursor_pos < 0) cursor_pos += MAP_HEI;
	    cursor_pos = (cursor_pos % MAP_HEI);
	    r.x = map_left + (MAP_WID*TILE_WID)+(TILE_SPACE*MAP_WID);
	    r.y = map_top + cursor_pos * (TILE_WID + TILE_SPACE);
	    break;
	case DIR_S:
	    if (cursor_pos < 0) cursor_pos += MAP_WID;
	    cursor_pos = (cursor_pos % MAP_WID);
	    r.x = map_left + cursor_pos * (TILE_WID + TILE_SPACE);
	    r.y = map_top + (MAP_HEI*TILE_HEI) + (MAP_HEI*TILE_SPACE);
	    break;
	case DIR_W:
	    if (cursor_pos < 0) cursor_pos += MAP_HEI;
	    cursor_pos = (cursor_pos % MAP_HEI);
	    r.x = map_left - (TILE_WID+TILE_SPACE);
	    r.y = map_top + cursor_pos * (TILE_WID + TILE_SPACE);
	    break;
	}

	r.x -= TILE_SPACE;
	r.y -= TILE_SPACE;

	if (cursor_img) {
	    r.x += ((TILE_WID+TILE_SPACE-(cursor_img->w/2)+1)/2);
	    r.y += ((TILE_HEI+TILE_SPACE-(cursor_img->h/4)+1)/2);

	    src.w = (cursor_img->w / 2);
	    src.h = (cursor_img->h / 4);
	    src.x = can_fire(NULL)*(src.w);
	    src.y = cursor_side * src.h;
	    SDL_BlitSurface(cursor_img, &src, screen, &r);
	} else {
	    SDL_FillRect(screen, &r, cursor_color);
	}

    }

    explosions_draw(screen);


    /* score */
    if (score_bg_img) {
	r.x = SCORE_BG_X;
	r.y = SCORE_BG_Y;
	SDL_BlitSurface(score_bg_img, NULL, screen, &r);
    }
    snprintf(scorebuf, 30, "%6i", score);
    write_string(screen, SCORE_X, SCORE_Y, scorebuf);

    /* level */
    if (leveltxt_bg_img) {
	r.x = LEVELTXT_BG_X;
	r.y = LEVELTXT_BG_Y;
	SDL_BlitSurface(leveltxt_bg_img, NULL, screen, &r);
    }
    snprintf(scorebuf, 30, "%3i", level);
    write_string(screen, LEVEL_POS_X, LEVEL_POS_Y, scorebuf);
}


int
map_check_flood(int x, int y, int block)
{
    int coords_x[MAX_FLOOD_HEAP];
    int coords_y[MAX_FLOOD_HEAP];
    int coords_c = 0;

    int mapmask[MAP_WID][MAP_HEI];

    int dx,dy;
    int mask_count = 0;
    int checked = 0;

    for (dx = 0; dx < MAP_WID; dx++)
	for (dy = 0; dy < MAP_HEI; dy++)
	    mapmask[dx][dy] = 0;

    coords_x[coords_c] = x; coords_y[coords_c] = y; coords_c++;

    do {
	coords_c--;
	x = coords_x[coords_c]; y = coords_y[coords_c];

	if (x >= 0 && y >= 0 && x < MAP_WID && y < MAP_HEI) {

	    mapmask[x][y] = 1;
	    mask_count++;

	    if ((x+1 < MAP_WID) && ((map[x+1][y].block) == block) && (mapmask[x+1][y] == 0)) {
		coords_x[coords_c] = (x+1); coords_y[coords_c] = y; coords_c++;
	    }
	    if ((x-1 >= 0) && ((map[x-1][y].block) == block) && (mapmask[x-1][y] == 0)) {
		coords_x[coords_c] = (x-1); coords_y[coords_c] = y; coords_c++;
	    }
	    if ((y+1 < MAP_HEI) && ((map[x][y+1].block) == block) && (mapmask[x][y+1] == 0)) {
		coords_x[coords_c] = x; coords_y[coords_c] = (y+1); coords_c++;
	    }
	    if ((y-1 >= 0) && ((map[x][y-1].block) == block) && (mapmask[x][y-1] == 0)) {
		coords_x[coords_c] = x; coords_y[coords_c] = (y-1); coords_c++;
	    }
	}
    } while (coords_c && (coords_c < MAX_FLOOD_HEAP-4));

    if (mask_count >= 3) {
	int i;
	int tmp = 1;
	if (mask_count > 3) {
	    /* bonuses for destroying 3+ blocks: 3 -> 0pts, 4 -> 3pts, 5 -> 12pts, 6 -> 60pts, 7 -> 360pts */
	    for (i = 3; i < mask_count; i++)
		tmp = tmp * i;
	    score += tmp;
	}

	for (dx = 0; dx < MAP_WID; dx++)
	    for (dy = 0; dy < MAP_HEI; dy++)
		if (mapmask[dx][dy]) {
		    int map_left = (SCREEN_WID / 2) - ((MAP_WID / 2)*TILE_WID) - ((MAP_WID*TILE_SPACE)/2);
		    int map_top = (SCREEN_HEI / 2) - ((MAP_HEI / 2)*TILE_HEI) - ((MAP_HEI*TILE_SPACE)/2);

		    score += 10; /* 10pts per destroyed block */
		    map_set_block(dx, dy, 0, DIR_NONE);
		    checked = 1;

		    explosions_add(map_left + dx * (TILE_WID+TILE_SPACE), map_top + dy * (TILE_HEI+TILE_SPACE));
		}

	for (i = 2; i < mask_count; i++)
	    play_sound(SND_EXPLODE);
    }

    return checked;
}


int
map_check()
{
    int x, y, block;

    int checked = 0;

    for (x = 0; x < MAP_WID; x++)
	for (y = 0; y < MAP_HEI; y++) {
	    block = (map[x][y].block);
	    if (block)
		checked |= map_check_flood(x,y, block);
	}
    return checked;
}

void
map_shift_blocks()
{
    int x,y;

    int done;
    int show_frame = 0;

    int frame_skips[3] = {9, 18, 36};


    do {
	done = 1;
	for (x = 0; x < MAP_WID; x++)
	    for (y = 0; y < MAP_HEI; y++) {
		if (map[x][y].steps > 0) {
		    map[x][y].xofs += map[x][y].xadj;
		    map[x][y].yofs += map[x][y].yadj;
		    map[x][y].steps--;
		    done = 0;
		}
	    }

	if (anim_delay && !((++show_frame) % frame_skips[block_move_speed % 3])) {
	    map_draw(1);
	    explosions_anim();
	    SDL_Flip(screen);
	}

    } while (!done);

    if (anim_delay) {
	map_draw(1);
	explosions_anim();
	SDL_Flip(screen);
    }

}

int
map_run(int *sounds)
{
    int x,y, block, dir, moved;
    int i;

    int checked = 0;
    boolean blockhit = 0;
    boolean blockout = 0;

    struct mapcell tmpmap[MAP_WID][MAP_HEI];

    do {

	for (x = 0; x < MAP_WID; x++)
	    for (y = 0; y < MAP_HEI; y++) {
		tmpmap[x][y].block = map[x][y].block;
		tmpmap[x][y].dir = map[x][y].dir;
	    }

	moved = 0;

	for (x = 0; x < MAP_WID; x++)
	    for (y = 0; y < MAP_HEI; y++)
	    if (tmpmap[x][y].block) {
		block = tmpmap[x][y].block;
		dir = tmpmap[x][y].dir;
		switch (dir) {
		default:
		case DIR_NONE: break;
		case DIR_N:
		    if (y-1 >= 0) {
			if (!(map[x][y-1].block)) {
			    map_set_block(x,y-1, block, dir);
			    map[x][y-1].steps = (TILE_HEI+TILE_SPACE);
			    map[x][y-1].yadj = -1;
			    map[x][y-1].yofs = (TILE_HEI+TILE_SPACE);
			    map_set_block(x, y, 0, DIR_NONE);
			    moved = 1;
			} else {
			    blockhit = 1;
			}
		    } else {
			for (i = MAP_BORDER_WID-2; i >= 0; i--)
			    border_n[i+1][x] = border_n[i][x];
			border_n[0][x].block = block;
			border_n[0][x].dir = DIR_NONE;
			map_set_block(x, y, 0, DIR_NONE);
			moved = 1;
			if (score >= 10) score -= 10;
			blockout = 1;
		    }
		    break;
		case DIR_S:
		    if (y+1 < MAP_HEI) {
			if (!(map[x][y+1].block)) {
			    map_set_block(x,y+1, block, dir);
			    map[x][y+1].steps = (TILE_HEI+TILE_SPACE);
			    map[x][y+1].yadj = 1;
			    map[x][y+1].yofs = -(TILE_HEI+TILE_SPACE);
			    map_set_block(x, y, 0, DIR_NONE);
			    moved = 1;
			} else {
			    blockhit = 1;
			}
		    } else {
			for (i = MAP_BORDER_WID-2; i >= 0; i--)
			    border_s[i+1][x] = border_s[i][x];
			border_s[0][x].block = block;
			border_s[0][x].dir = DIR_NONE;
			map_set_block(x, y, 0, DIR_NONE);
			moved = 1;
			if (score >= 10) score -= 10;
			blockout = 1;
		    }
		    break;
		case DIR_W:
		    if (x-1 >= 0) {
			if (!(map[x-1][y].block)) {
			    map_set_block(x-1,y, block, dir);
			    map[x-1][y].steps = (TILE_WID+TILE_SPACE);
			    map[x-1][y].xadj = -1;
			    map[x-1][y].xofs = (TILE_WID+TILE_SPACE);
			    map_set_block(x, y, 0, DIR_NONE);
			    moved = 1;
			} else {
			    blockhit = 1;
			}
		    } else {
			for (i = MAP_BORDER_WID-2; i >= 0; i--)
			    border_w[i+1][y] = border_w[i][y];
			border_w[0][y].block = block;
			border_w[0][y].dir = DIR_NONE;
			map_set_block(x, y, 0, DIR_NONE);
			moved = 1;
			if (score >= 10) score -= 10;
			blockout = 1;
		    }
		    break;
		case DIR_E:
		    if (x+1 < MAP_WID) {
			if (!(map[x+1][y].block)) {
			    map_set_block(x+1,y, block, dir);
			    map[x+1][y].steps = (TILE_WID+TILE_SPACE);
			    map[x+1][y].xadj = 1;
			    map[x+1][y].xofs = -(TILE_WID+TILE_SPACE);
			    map_set_block(x, y, 0, DIR_NONE);
			    moved = 1;
			} else {
			    blockhit = 1;
			}
		    } else {
			for (i = MAP_BORDER_WID-2; i >= 0; i--)
			    border_e[i+1][y] = border_e[i][y];
			border_e[0][y].block = block;
			border_e[0][y].dir = DIR_NONE;
			map_set_block(x, y, 0, DIR_NONE);
			moved = 1;
			if (score >= 10) score -= 10;
			blockout = 1;
		    }
		    break;
		}
	    }

	map_shift_blocks();

	/*
	if (anim_delay) {
	    map_draw(1);
	    explosions_anim();
	    SDL_Flip(screen);
	    SDL_Delay(anim_delay);
	    }
	*/

	checked |= moved;
    } while (moved);


    if (sounds) {
	*sounds = 0;
	if (blockhit) *sounds = (*sounds) + 1;
	if (blockout) *sounds = (*sounds) + 2;
    }

    return checked;
}




void
exit_game()
{
    pline(2, "Shutting down SDL.\n");

    SDL_FreeSurface(screen);
    SDL_FreeSurface(background);
    SDL_FreeSurface(window_border);
    SDL_FreeSurface(cursor_img);
    SDL_FreeSurface(target_pointer_img);
    SDL_FreeSurface(score_bg_img);
    SDL_FreeSurface(leveltxt_bg_img);
    SDL_FreeSurface(charset_img);
    SDL_FreeSurface(tilegfx);

    SDL_Quit();
    printf("Bye!\n");
    print_end_of_game_info();

    levelfile_free();
}

#ifdef _TINSPIRE
#define LoadImg(path, var) nSDL_LoadImage(var)
#else
#define LoadImg(path, var) IMG_Load(path)
#endif

void
init_game()
{
    int modeflags = SDL_HWSURFACE|SDL_ANYFORMAT;

    pline(2, "Initializing SDL.\n");

#ifdef _TINSPIRE
    if ((SDL_Init(SDL_INIT_VIDEO) == -1))
#else
    if ((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) == -1))
#endif
    {
	printf("Could not initialize SDL: %s.\n", SDL_GetError());
	exit(-1);
    }
    pline(2, "SDL initialized.\n");

    atexit(SDL_Quit);

    if (is_fullscreen) {
	modeflags |= SDL_FULLSCREEN;
    }

#ifdef _TINSPIRE
    screen = SDL_SetVideoMode(SCREEN_WID, SCREEN_HEI, SCREEN_BPP, SDL_SWSURFACE|SDL_FULLSCREEN|SDL_HWPALETTE);
#else
    screen = SDL_SetVideoMode(SCREEN_WID,SCREEN_HEI, SCREEN_BPP, modeflags);
#endif
    if (screen == NULL) {
	fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s.\n",
		SCREEN_WID, SCREEN_HEI, SCREEN_BPP,
		SDL_GetError());
	exit(1);
    }

    pline(2, "Set %dx%dx%d %s mode.\n", screen->w, screen->h,
	  screen->format->BitsPerPixel,
	  is_fullscreen ? "fullscreen" : "windowed" );

    SDL_WM_SetCaption(GAME_NAME, NULL);

#ifndef _TINSPIRE
    if (audio_use) {

	if(Mix_OpenAudio(audio_rate, audio_format, audio_num_channels, audio_buffers) != 0) {
	    fprintf(stderr, "Unable to initialize audio: %s\n", Mix_GetError());
	    audio_use = FALSE;
	} else {
	    int tmpi;
	    for (tmpi = 0; tmpi < NUM_SOUNDS; tmpi++) {
		audio_sound[tmpi] = Mix_LoadWAV(audio_sound_files[tmpi]);
	    }
	}
    }
#endif



    button_color = SDL_MapRGB(screen->format, 120, 150, 170);
    button_hilite_color = SDL_MapRGB(screen->format, 170,200,220);
    window_color = SDL_MapRGB(screen->format, 128,100,169);


    cursor_color = SDL_MapRGB(screen->format, 255,255,255);
    allow_color = SDL_MapRGB(screen->format, 100,100,100);
    bg_color = SDL_MapRGB(screen->format, 0, 0, 0);

    background = LoadImg("gfx/background.png", img_background);

    window_border = LoadImg("gfx/window_border.png", img_window_border);

    cursor_img = LoadImg("gfx/cursor.png", cursor);

    target_pointer_img = LoadImg("gfx/targeting.png", targeting);

    score_bg_img = LoadImg("gfx/score_bg.png", score_bg);
    leveltxt_bg_img = LoadImg("gfx/level_bg.png", level_bg);

    charset_img = LoadImg("gfx/charset.png", charset);
    if (charset_img == NULL) {
	fprintf(stderr, "Couldn't load charset: %s.\n",
		SDL_GetError());
	exit(1);
    }

    explosion_img = LoadImg("gfx/explosion.png", explosion);

    tilegfx = LoadImg("gfx/tiles.png", tiles);

    if (tilegfx == NULL) {
	fprintf(stderr, "Couldn't load tiles: %s.\n",
		SDL_GetError());
	exit(1);
    }


    if (!levelfile_load(levelset_fname)) {
	fprintf(stderr, "Couldn't load level file %s.\n", levelset_fname);
	show_popup(screen, "Couldn't load level set.", 7);
    }

    if (!map_load(level)) {
	map_generate_random(1,10);
    }

    borders_init();

    explosions_init();
}


void
do_block_anim()
{
    SDL_Event event;
    int fini = 0;
    boolean skip_delays = 0;
    int sounds = 0;

    do {
	if (!fini && anim_delay && !skip_delays) {
	    map_draw(1);
	    SDL_Flip(screen);
	    SDL_Delay(anim_delay);

	    while (SDL_PollEvent(&event)) {
		switch(event.type) {
		default:
		    break;
		case SDL_QUIT:
		    do_quit = TRUE;
		    return;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_KEYDOWN:
		    skip_delays = 1;
		    break;
		}
	    }

	}
	fini = map_run(&sounds);

	if (!fini) {
	    fini = map_check();
	} else {
	    if (sounds & 1)
		play_sound(SND_BLOCKHIT);
	    if (sounds & 2)
		play_sound(SND_BLOCKOUT);
	}

    } while (fini);
}

void
wait_for_explosions()
{
    while (explosions_active()) {
	explosions_anim();
	explosions_draw(screen);
	map_draw(2);
	SDL_Flip(screen);
    }
}

void
next_level()
{
    wait_for_explosions();
    level++;
    explosions_init();
    if (map_load(level)) {
	play_sound(SND_NEXTLEVEL);
	show_gameover(screen, 2);
	borders_init();
    } else {
	play_sound(SND_FINISHSET);
	show_gameover(screen, 1);
	printf("Congratulations, you finished the level set!\n");
	print_end_of_game_info();
	program_state = 0;
    }
}

void
pressed_fire()
{
    int length;

    if (can_fire(&length) && (length > 0)) {
	int x;
	moves++;
	switch (cursor_side) {
	case DIR_N:
	    length--;
	    map_set_block(cursor_pos, 0, border_n[0][cursor_pos].block, DIR_S);
	    for (x = 0; x < MAP_BORDER_WID-1; x++)
		border_n[x][cursor_pos] = border_n[x+1][cursor_pos];
	    border_n[MAP_BORDER_WID-1][cursor_pos].block = random_used_tiletype();
	    break;
	case DIR_E:
	    length--;
	    map_set_block(MAP_WID-1, cursor_pos, border_e[0][cursor_pos].block, DIR_W);
	    for (x = 0; x < MAP_BORDER_WID-1; x++)
		border_e[x][cursor_pos] = border_e[x+1][cursor_pos];
	    border_e[MAP_BORDER_WID-1][cursor_pos].block = random_used_tiletype();
	    break;
	case DIR_S:
	    length--;
	    map_set_block(cursor_pos, MAP_HEI-1, border_s[0][cursor_pos].block, DIR_N);
	    for (x = 0; x < MAP_BORDER_WID-1; x++)
		border_s[x][cursor_pos] = border_s[x+1][cursor_pos];
	    border_s[MAP_BORDER_WID-1][cursor_pos].block = random_used_tiletype();
	    break;
	case DIR_W:
	    length--;
	    map_set_block(0, cursor_pos, border_w[0][cursor_pos].block, DIR_E);
	    for (x = 0; x < MAP_BORDER_WID-1; x++)
		border_w[x][cursor_pos] = border_w[x+1][cursor_pos];
	    border_w[MAP_BORDER_WID-1][cursor_pos].block = random_used_tiletype();
	    break;
	}

	play_sound(SND_SHOOTBLOCK);

	do_block_anim();

	if (map_is_empty()) {
	    next_level();
	} else if (map_is_full()) {
	    play_sound(SND_GAMEOVER);
	    wait_for_explosions();
	    show_gameover(screen, 0);
	    program_state = 0;
	}
    } else {
	play_sound(SND_CANTMOVE);
    }
}

void
options_menu_loop()
{
#define OPTIONS_OPTS 4
    int return_from_options = 0;
    int i, def_opt = OPTIONS_OPTS-1;
    int selected_opt;
    char *optionsmenu[OPTIONS_OPTS];

    char *blockmovespeed_str[3] = {"slow", "normal", "fast"};

    for (i = 0; i < OPTIONS_OPTS; i++)
	optionsmenu[i] = (char *)malloc(64);

    do {
	cursor_shown = FALSE;
	map_draw(1);
	cursor_shown = TRUE;
	sprintf(optionsmenu[0], "%s", (!is_fullscreen) ? "Fullscreen" : " Windowed ");
	sprintf(optionsmenu[1], "Sounds: %3s", (audio_use) ? "on" : "off");
	sprintf(optionsmenu[2], "Block speed: %6s", blockmovespeed_str[block_move_speed % 3]);
	sprintf(optionsmenu[3], "Back");
	selected_opt = show_menu(screen, "Options", OPTIONS_OPTS, optionsmenu, def_opt);
	def_opt = selected_opt;
	switch (selected_opt) {
	case 0:
	    if (SDL_WM_ToggleFullScreen(screen)) {
		if (is_fullscreen)
		    is_fullscreen = FALSE;
		else
		    is_fullscreen = TRUE;
	    };
	    break;
	case 1:
	    audio_use = !audio_use;
	    break;
	case 2:
	    block_move_speed = (block_move_speed + 1) % 3;
	    break;
	default: return_from_options = 1; break;
	}
    } while (!return_from_options);

    for (i = 0; i < OPTIONS_OPTS; i++)
	free(optionsmenu[i]);

}



void
handle_key_down(SDL_keysym *keysym)
{
    switch (keysym->sym) {
    case GKEY_FIRE:
	pressed_fire();
	break;
    case GKEY_TURN_CW:
	{
	    int length;
	    if (can_fire(&length)) {
		cursor_pos = length;
		if (cursor_side == DIR_S)
		    cursor_pos = MAP_WID - 1 - length;
		else if (cursor_side == DIR_E)
		    cursor_pos = MAP_HEI - 1 - length;
	    }
	    cursor_side = (cursor_side + 1) % 4;
	}
	break;
    case GKEY_TURN_CCW:
	{
	    int length;
	    if (can_fire(&length)) {
		cursor_pos = length;
		if (cursor_side == DIR_S)
		    cursor_pos = MAP_WID - 1 - length;
		else if (cursor_side == DIR_E)
		    cursor_pos = MAP_HEI - 1 - length;
	    }
	    cursor_side = (cursor_side + 3) % 4;
	}
	break;
    case GKEY_UP:
	switch (cursor_side) {
	case DIR_N: break;
	case DIR_E:
	    cursor_pos--;
	    if (cursor_pos < 0) {
		cursor_pos = MAP_WID-1;
		cursor_side = 0;
	    }
	    break;
	case DIR_S:
	    cursor_side = 0;
	    break;
	case DIR_W:
	    cursor_pos--;
	    if (cursor_pos < 0) {
		cursor_pos = 0;
		cursor_side = 0;
	    }
	    break;
	}
	break;
    case GKEY_DOWN:
	switch (cursor_side) {
	case DIR_N:
	    cursor_side = 2;
	    break;
	case DIR_E:
	    cursor_pos++;
	    if (cursor_pos >= MAP_HEI) {
		cursor_pos = MAP_WID-1;
		cursor_side = 2;
	    }
	    break;
	case DIR_S: break;
	case DIR_W:
	    cursor_pos++;
	    if (cursor_pos >= MAP_HEI) {
		cursor_pos = 0;
		cursor_side = 2;
	    }
	    break;
	}
	break;
    case GKEY_RIGHT:
	switch (cursor_side) {
	case DIR_N:
	    cursor_pos++;
	    if (cursor_pos >= MAP_WID) {
		cursor_pos = 0;
		cursor_side = 1;
	    }
	    break;
	case DIR_E: break;
	case DIR_S:
	    cursor_pos++;
	    if (cursor_pos >= MAP_WID) {
		cursor_pos = MAP_HEI-1;
		cursor_side = 1;
	    }
	    break;
	case DIR_W:
	    cursor_side = 1;
	    break;
	}
	break;
    case GKEY_LEFT:
	switch (cursor_side) {
	case DIR_N:
	    cursor_pos--;
	    if (cursor_pos < 0) {
		cursor_pos = 0;
		cursor_side = 3;
	    }
	    break;
	case DIR_E:
	    cursor_side = 3;
	    break;
	case DIR_S:
	    cursor_pos--;
	    if (cursor_pos < 0) {
		cursor_pos = MAP_HEI-1;
		cursor_side = 3;
	    }
	    break;
	case DIR_W: break;
	}
	break;
    case GKEY_QUIT:
	{
	    int quit_game_menu = 0;
	    char *quitgamemenu[] = {
		"Resume game",
		"Options",
		"Quit"
	    };
	    do {
		cursor_shown = FALSE;
		map_draw(1);
		cursor_shown = TRUE;
		switch (show_menu(screen, GAME_NAME, ARRAY_SIZE(quitgamemenu), quitgamemenu, 0)) {
		default:
		case 0: quit_game_menu = 1; break;
		case 1: options_menu_loop(); break;
		case 2:
		    play_sound(SND_GAMEOVER);
		    show_gameover(screen, 0);
		    program_state = 0;
		    quit_game_menu = 1;
		    break;
		}
	    } while (!quit_game_menu);

	}
	break;
    default:
	if (debugging_level && keysym->sym == 'n')
	    next_level();
	break;
    }
}

int
handle_mouse_hover(int mx, int my)
{
#ifdef _TINSPIRE
    int map_left = (3*TILE_WID);
    int map_top = (3*TILE_HEI);
#else
    int map_left = (SCREEN_WID / 2) - ((MAP_WID / 2)*TILE_WID) - ((MAP_WID*TILE_SPACE)/2);
    int map_top = (SCREEN_HEI / 2) - ((MAP_HEI / 2)*TILE_HEI) - ((MAP_HEI*TILE_SPACE)/2);
#endif

    int x, y;

    int min_x, min_y, max_x, max_y;

    for (x = 0; x < MAP_WID; x++) {
	min_x = map_left + x *(TILE_WID + TILE_SPACE);
	min_y = map_top - (TILE_HEI+TILE_SPACE) * MAP_BORDER_WID;
	max_x = min_x + TILE_WID;
	max_y = min_y + ((TILE_HEI+TILE_SPACE) * MAP_BORDER_WID);

	if (mx >= min_x && mx <= max_x && my >= min_y && my <= max_y) {
	    cursor_side = DIR_N;
	    cursor_pos = x;
	    return 1;
	}

	min_y = map_top + MAP_HEI * (TILE_HEI+TILE_SPACE);
	max_y = min_y + (TILE_HEI+TILE_SPACE)*MAP_BORDER_WID;

	if (mx >= min_x && mx <= max_x && my >= min_y && my <= max_y) {
	    cursor_side = DIR_S;
	    cursor_pos = x;
	    return 1;
	}

    }

    for (y = 0; y < MAP_HEI; y++) {
	min_y = map_top + y * (TILE_HEI + TILE_SPACE);
	min_x = map_left - (TILE_WID + TILE_SPACE)*MAP_BORDER_WID;
	max_x = min_x + (TILE_WID+TILE_SPACE)*MAP_BORDER_WID;
	max_y = min_y + TILE_HEI;

	if (mx >= min_x && mx <= max_x && my >= min_y && my <= max_y) {
	    cursor_side = DIR_W;
	    cursor_pos = y;
	    return 1;
	}

	min_x = map_left + MAP_WID * (TILE_WID + TILE_SPACE);
	max_x = min_x + (TILE_WID+TILE_SPACE)*MAP_BORDER_WID;

	if (mx >= min_x && mx <= max_x && my >= min_y && my <= max_y) {
	    cursor_side = DIR_E;
	    cursor_pos = y;
	    return 1;
	}

    }
    return 0;
}


void
process_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	switch(event.type) {
	case SDL_KEYUP:
	    is_paused = FALSE;
	    cursor_shown = TRUE;
	    handle_key_down(&event.key.keysym);
	    break;
	case SDL_KEYDOWN:
	    break;
	case SDL_ACTIVEEVENT:
	    if (event.active.state & (SDL_APPINPUTFOCUS|SDL_APPACTIVE)) {
		is_paused = TRUE;
	    }
	    break;
	case SDL_QUIT:
	    fprintf(stdout, "Whee!\n");
	    print_end_of_game_info();
	    do_quit = TRUE;
	    break;
	case SDL_MOUSEMOTION:
	    is_paused = FALSE;
	    if (!handle_mouse_hover(event.motion.x, event.motion.y)) {
		cursor_shown = FALSE;
	    } else {
		cursor_shown = TRUE;
	    }
	    break;
	case SDL_MOUSEBUTTONDOWN:
	    is_paused = FALSE;
	    if (handle_mouse_hover(event.button.x, event.button.y)) {
		cursor_shown = TRUE;
		pressed_fire();
	    }
	case SDL_MOUSEBUTTONUP:
	    is_paused = FALSE;
	    if (handle_mouse_hover(event.button.x, event.button.y)) {
		cursor_shown = TRUE;
	    }
	    break;
	default:
	    break;
	}
    }
}



void
game_drawscreen_main()
{
    map_draw(0);
    SDL_Flip(screen);
    explosions_anim();
}


void
game_drawscreen_paused()
{
    SDL_Flip(screen);
    SDL_Delay(200);
}

void
main_menu_loop()
{
    char *mainmenuoptions[] = {
	"Play",
	"Options",
	"Quit"
    };
    int selected_opt = 0;
    static int def_opt = 0;

    cursor_shown = FALSE;
    map_draw(1);
    cursor_shown = TRUE;
    selected_opt = show_menu(screen, GAME_NAME, ARRAY_SIZE(mainmenuoptions), mainmenuoptions, def_opt);
    def_opt = selected_opt;
    switch (selected_opt) {
    case 0:
	program_state = 1;
	score = 0;
	level = 1;
	moves = 0;
	play_sound(SND_STARTUP);
	if (!map_load(level)) {
	    map_generate_random(1,10);
	}
	break;
    case 1:
	options_menu_loop();
	break;
    default:
	do_quit = TRUE;
	break;
    }
}

void
main_game_loop()
{
    while (!do_quit) {
	if (is_paused) {
	    game_drawscreen_paused();
	    process_events();
	} else {

	    switch (program_state) {
	    default:
	    case 0:
		main_menu_loop();
		break;
	    case 1:
		/* playing */
		game_drawscreen_main();
		process_events();
		break;
	    }
	}
    }
}



int
handle_params(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++) {
    char *str = argv[i];
    char *parm = strchr(str,'=');

    if (parm) parm++;

    if (strstr(str,"--help") == str || strstr(str, "-h") == str) {
	printf("--help (or -h)\tthis help.\n");
	printf("--level=X\tstart playing from level number X.\n");
	printf("--levelset=name\tload levelset called 'name'.\n");
	printf("--debug=X\tset debugging output level to X.\n");
	printf("--fullscreen\tstart game in fullscreen mode.\n");
	printf("--random=X\tplay a randomly generated levelset of X levels.\n");
	printf("--nosound\tdisable sound effects.\n");
	return 0;
    }
    else if (strstr(str,"--levelset") == str && parm) strncpy(levelset_fname, parm, 63);
    else if (strstr(str,"--level") == str && parm) {
	level = atoi(parm);
	debugging_level = 1;
    }
    else if (strstr(str,"--debug") == str) debugging_level = (parm ? atoi(parm) : 1);
    else if (strstr(str,"--nosound") == str) audio_use = FALSE;
    else if (strstr(str,"--fullscreen") == str) is_fullscreen = TRUE;
    else if (strstr(str,"--random") == str) {
	if (parm) random_levelset = atoi(parm);
	if (random_levelset < 1) random_levelset = 1;
	strcpy(levelset_name, "Random Set");
    }
  }

  return 1;
}



int
main(int argc, char *argv[])
{
    printf("%s, %s.\n\n", GAME_NAME, GAME_VERSION);

    srand(time(NULL));

    strcpy(levelset_fname, def_levelset);

    if (handle_params(argc, argv)) {
	init_game();
	pline(2, "Running.\n");
	main_game_loop();
	exit_game();
    }
    return 0;
}
