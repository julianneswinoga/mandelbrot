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
#define SCALE_FACTOR (0.1)
#define INITIAL_BLOCKSIZE (SCREEN_WIDTH / 4)

typedef struct {
	long double x;
	long double y;
	long double scale;
} GRAPH;

xcb_connection_t *        connection;
xcb_window_t              window;
xcb_gcontext_t            graphics;
xcb_colormap_t            colormapId;
xcb_alloc_color_reply_t **colors;
xcb_generic_event_t *     e;

GRAPH           graph;
int             next_available_line, SCREEN_WIDTH, SCREEN_HEIGHT, blocksize;
pthread_mutex_t mutex_nextline, mutex_draw, mutex_phaseComplete;
pthread_cond_t  condition_phaseComplete;
bool            thread_exit_premature;
bool            threadWorkDone[NUM_THREADS] = {
    0,
};

void event_action(xcb_generic_event_t *);

/**
 * https://krazydad.com/tutorials/makecolors.php
 */
void colorGradient(xcb_alloc_color_reply_t ***crs, double f1, double f2, double f3, double p1, double p2, double p3, double center, double width, int len) {
	for (int i = 0; i < len; i++) {
		(*crs)[i] = xcb_alloc_color_reply(connection,
		                                  xcb_alloc_color(connection, colormapId,
		                                                  sin(f1 * i + p1) * width + center,
		                                                  sin(f2 * i + p2) * width + center,
		                                                  sin(f3 * i + p3) * width + center),
		                                  NULL);
	}
}

bool allThreadsComplete() {
	for (int i = 0; i < NUM_THREADS; i++) {
		if (!threadWorkDone[i])
			return false;
	}
	return true;
}

void *mandelThread(void *arg) {
	int threadNumber = (int)(uintptr_t)arg;
	printf("Thread #%i created\n", threadNumber);

	long double   x0, y0, x, y, xtemp, iteration, q;
	unsigned long colorIndex;
	int           lineStart;
	int           py, px, initialpx, initialpy;

	while (1) {
		if (next_available_line >= SCREEN_HEIGHT) { // No more work to be done
			if (blocksize > 1) {
				threadWorkDone[threadNumber] = true;
				if (!allThreadsComplete()) { // Threads are still working, so wait
					pthread_mutex_lock(&mutex_phaseComplete);
					pthread_cond_wait(&condition_phaseComplete,
					                  &mutex_phaseComplete); // Wait for phases to complete
					pthread_mutex_unlock(&mutex_phaseComplete);
				} else {
					for (size_t i = 0; i < NUM_THREADS; i++) threadWorkDone[i] = false; // Reset thread work

					blocksize           = (blocksize / 2 > 1 ? blocksize / 2 : 1);
					next_available_line = 0;
					pthread_cond_broadcast(&condition_phaseComplete); // Broadcast that we can proceed
				}
			} else {                   // Blocksize is 1
				xcb_flush(connection); // Make sure a flush is called
				pthread_exit(NULL);
				return NULL;
			}
		}
		pthread_mutex_lock(&mutex_nextline);
		lineStart = next_available_line;
		next_available_line += THREAD_LINES * blocksize;
		pthread_mutex_unlock(&mutex_nextline);

		initialpx = 0;
		initialpy = lineStart;

		for (py = initialpy; py < initialpy + THREAD_LINES * blocksize; py += blocksize) {
			for (px = initialpx; px < SCREEN_WIDTH; px += blocksize) {
				x0 = graph.x + (px - SCREEN_WIDTH / 2) * graph.scale;
				y0 = graph.y + (py - SCREEN_HEIGHT / 2) * graph.scale;
				x  = x0;
				y  = y0;

				q = (x0 - 0.25) * (x0 - 0.25) + y0 * y0;

				if ((x0 + 1) * (x0 + 1) + y0 * y0 < 1 / 16 || q * (q + x0 - 0.25) < 0.25 * y0 * y0) {
					iteration = MAX_ITER;
				} else {
					for (iteration = 0; x * x + y * y < (1 << 10) && iteration < MAX_ITER; iteration++) {
						xtemp = x * x - y * y + x0;
						y     = 2 * x * y + y0;
						x     = xtemp;
					}
				}

				colorIndex = (unsigned long)(iteration / MAX_ITER * PALETTE_LENGTH);

				xcb_rectangle_t rect = {.x = px, .y = py, .width = blocksize, .height = blocksize};

				pthread_mutex_lock(&mutex_draw);
				xcb_change_gc(connection, graphics, XCB_GC_FOREGROUND, &colors[colorIndex]->pixel); // Change the color
				xcb_poly_fill_rectangle(connection, window, graphics, 1, &rect);
				pthread_mutex_unlock(&mutex_draw);
			}
			if (thread_exit_premature) {
				pthread_exit(NULL); // Say that we have an event to handle
				return NULL;
			}
			if ((e = xcb_poll_for_event(connection)) != NULL) { // Check every row for an event
				thread_exit_premature = true;
				pthread_exit(1); // Say that we have an event to handle
				return NULL;
			}
		}
	}
}

void startMandel() {

	xcb_rectangle_t rect = {.x = 0, .y = 0, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT};
	xcb_change_gc(connection, graphics, XCB_GC_FOREGROUND, &colors[PALETTE_LENGTH]->pixel);

	xcb_poly_fill_rectangle(connection, window, graphics, 1, &rect);
	xcb_flush(connection); // Make sure a flush is called

	printf("Building image...");
	int            threadIds[NUM_THREADS];
	pthread_attr_t attr;
	pthread_t      threads[NUM_THREADS];

	pthread_mutex_init(&mutex_nextline, NULL);         // Init mutex for data
	pthread_mutex_init(&mutex_draw, NULL);             // Init mutex for flushing
	pthread_mutex_init(&mutex_phaseComplete, NULL);    // Init mutex for phase complete
	pthread_cond_init(&condition_phaseComplete, NULL); // Init condition for phase complete
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	next_available_line   = 0;
	blocksize             = INITIAL_BLOCKSIZE; // Reset the blocksize
	thread_exit_premature = false;

	for (int i = 0; i < NUM_THREADS; i++) { // Create threads
		threadIds[i] = i;
		pthread_create(&threads[i], &attr, mandelThread, (void *)(uintptr_t)threadIds[i]);
	}
	xcb_flush(connection); // Make sure a flush is called

	pthread_attr_destroy(&attr);

	for (int i = 0; i < NUM_THREADS; i++) { // Join all threads
		void *status;
		pthread_join(threads[i], &status);

		if (status != 0) {
			event_action(e); // Handle the pending event
			printf("Thread #%i exited with %i\n", i, (int)(uintptr_t)status);
		}
	}
	printf("done!\n");
	printf("Zoom level:%Lfe-12\tX:%Lfe-12\tY:%Lfe-12\n", graph.scale * 1E12, graph.x * 1E12, graph.y * 1E12);
}

void event_action(xcb_generic_event_t *e) {
	switch (e->response_type & ~0x80) {
		case XCB_BUTTON_PRESS: {
			xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
			graph.x += graph.scale * (ev->event_x - SCREEN_WIDTH / 2);
			graph.y += graph.scale * (ev->event_y - SCREEN_HEIGHT / 2);
			switch (ev->detail) {
				case 1: // Zoom in on left click
					graph.scale *= 0.5;
					break;
				case 2: // Pan with middle click
					//blocksize /= blocksize == 1 ? 1 : 2;
					printf("BLK: %i\n", blocksize);
					//startMandel();
					break;
				case 3: // Zoom out with right click
					graph.scale /= 0.5;
					break;
				default:
					break;
			}
			startMandel();
		} break;
		case XCB_RESIZE_REQUEST: { // Window resized
			xcb_resize_request_event_t *ev    = (xcb_resize_request_event_t *)e;
			if (ev->width > 0) SCREEN_WIDTH   = ev->width;
			if (ev->height > 0) SCREEN_HEIGHT = ev->height;

			printf("EXPOSE EVENT%ld, %ix%i\n", ev->window, SCREEN_WIDTH, SCREEN_HEIGHT);
			uint32_t values[] = {SCREEN_WIDTH, SCREEN_HEIGHT};
			xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
		} break;
		default:
			printf("Unknown event occured\n");
			break;
	}
}

int main() {
	printf("Building X11 window...");

	SCREEN_WIDTH  = 200;
	SCREEN_HEIGHT = 200;
	graph.x       = -0.8; // Initial conditions
	graph.y       = 0.0;
	graph.scale   = 0.015;
	blocksize     = INITIAL_BLOCKSIZE;

	connection                        = xcb_connect(NULL, NULL);
	const xcb_setup_t *   setup       = xcb_get_setup(connection);
	xcb_screen_iterator_t iter        = xcb_setup_roots_iterator(setup);
	xcb_screen_t *        screen      = iter.data;
	uint32_t              w_mask      = XCB_CW_EVENT_MASK;
	uint32_t              w_values[1] = {XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_RESIZE_REDIRECT}; // Generate events when a button is pressed

	window = xcb_generate_id(connection);
	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0,
	                  SCREEN_WIDTH, SCREEN_HEIGHT, BORDER_WIDTH, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, w_mask, w_values);
	xcb_map_window(connection, window);

	graphics             = xcb_generate_id(connection);
	uint32_t g_mask      = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t g_values[2] = {screen->black_pixel, 0};
	xcb_create_gc(connection, graphics, window, g_mask, g_values);

	colormapId = xcb_generate_id(connection);
	xcb_create_colormap(connection,
	                    XCB_COLORMAP_ALLOC_NONE,
	                    colormapId,
	                    window,
	                    screen->root_visual);

	printf("Building color palette...");
	colors = malloc(PALETTE_LENGTH * sizeof(xcb_alloc_color_reply_t) + 1); // Allocate memory +1 for black
	colorGradient(&colors, PALETTE_RAINBOWS * 2.0 * M_PI / PALETTE_LENGTH,
	              PALETTE_RAINBOWS * 2.0 * M_PI / PALETTE_LENGTH,
	              PALETTE_RAINBOWS * 2.0 * M_PI / PALETTE_LENGTH, 0, 2, 4, (1 << 15), (1 << 15), PALETTE_LENGTH);

	colors[PALETTE_LENGTH] = xcb_alloc_color_reply(connection, xcb_alloc_color(connection, colormapId, 0, 0, 0), NULL);

	printf("done!\n");

	xcb_rectangle_t rect;
	for (int i = 0; i < PALETTE_LENGTH; i++) {
		xcb_change_gc(connection, graphics, XCB_GC_FOREGROUND, &colors[i]->pixel);
		rect.x      = (int16_t)(i * (1.0 * SCREEN_WIDTH / PALETTE_LENGTH));
		rect.y      = 0;
		rect.width  = (uint16_t)(1.0 * SCREEN_WIDTH / PALETTE_LENGTH) + 1;
		rect.height = SCREEN_HEIGHT;
		xcb_poly_fill_rectangle(connection, window, graphics, 1, &rect);
		xcb_flush(connection);
	}

	sleep(3);

	startMandel(); // Initial mandelbrot

	while ((e = xcb_wait_for_event(connection))) {
		event_action(e);
	}

	xcb_disconnect(connection);
	return 0;
}
