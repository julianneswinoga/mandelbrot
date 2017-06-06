#ifndef __MANDELBROT_H__
#define __MANDELBROT_H__

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>

#define M_PI 3.14159265358979323846

#define BORDER_WIDTH (10)
#define PALETTE_LENGTH (1000)
#define PALETTE_RAINBOWS (6)
#define MAX_ITER (1000)
#define NUM_THREADS (8)
#define THREAD_LINES (2)
#define SCALE_FACTOR (0.5)
#define INITIAL_BLOCKSIZE (SCREEN_WIDTH / 4)
#define MINIMUM_BLOCKSIZE (10)

typedef struct {
	long double x;
	long double y;
	long double scale;
} GRAPH;

typedef struct {
	int r;
	int g;
	int b;
} PIXEL;

xcb_connection_t *        connection;
xcb_screen_t *            screen;
xcb_window_t              window;
xcb_gcontext_t            graphics;
xcb_colormap_t            colormapId;
xcb_alloc_color_reply_t **colors;
xcb_generic_event_t *     e;

GRAPH           graph;
PIXEL **        pixscreen;
int             next_available_line, SCREEN_WIDTH, SCREEN_HEIGHT, blocksize;
pthread_mutex_t mutex_nextline, mutex_draw, mutex_phaseComplete;
pthread_cond_t  condition_phaseComplete;
bool            thread_exit_premature;
bool            threadWorkDone[NUM_THREADS] = {
    0,
};

void  colorGradient(xcb_alloc_color_reply_t ***, double, double, double, double, double, double, double, double, int);
bool  allThreadsComplete();
void *mandelThread(void *);
void  startMandel();
void  event_action(xcb_generic_event_t *);

#endif //__MANDELBROT_H__
