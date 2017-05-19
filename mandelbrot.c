#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>

#define M_PI 3.14159265358979323846

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (800)
#define BORDER_WIDTH (10)
#define PALETTE_LENGTH (1000)
#define PALETTE_RAINBOWS (6)
#define MAX_ITER (1000)
#define NUM_THREADS (8)
#define THREAD_LINES (2)
#define SCALE_FACTOR (0.1)

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

GRAPH           graph;
int             next_available_line;
pthread_mutex_t mutex_data, mutex_flush;

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

void *mandelThread(void *arg) {
	int threadNumber = (int)(uintptr_t)arg;
	printf("Thread #%i created\n", threadNumber);

	long double   x0, y0, x, y, xtemp, iteration, q;
	unsigned long colorIndex;
	int           lineStart;
	int           py, px, initialpx, initialpy;

	while (1) {
		pthread_mutex_lock(&mutex_data);
		if (next_available_line >= SCREEN_HEIGHT) { // No more work to be done
			pthread_mutex_unlock(&mutex_data);
			pthread_exit(NULL);
			return NULL;
		} else {
			lineStart = next_available_line;
			next_available_line += THREAD_LINES;
		}
		pthread_mutex_unlock(&mutex_data);

		initialpx = 0;
		initialpy = lineStart;

		for (py = initialpy; py < initialpy + THREAD_LINES; py++) {
			for (px = initialpx; px < SCREEN_WIDTH; px++) {
				x0 = graph.x + (px - SCREEN_WIDTH / 2) * graph.scale;
				y0 = graph.y + (py - SCREEN_HEIGHT / 2) * graph.scale;
				x  = x0;
				y  = y0;

				q = (x0 - 0.25) * (x0 - 0.25) + y0 * y0;

				if ((x0 + 1) * (x0 + 1) + y0 * y0 < 1 / 16 || q * (q + x0 - 0.25) < 0.25 * y0 * y0) {
					iteration = MAX_ITER;
				} else {
					for (iteration = 0; x * x + y * y < (1 << 16) && iteration < MAX_ITER; iteration++) {
						xtemp = x * x - y * y + x0;
						y     = 2 * x * y + y0;
						x     = xtemp;
					}
				}

				colorIndex = (unsigned long)(iteration / MAX_ITER * PALETTE_LENGTH);
				pthread_mutex_lock(&mutex_flush);
				xcb_point_t pt = {.x = px, .y = py};
				xcb_change_gc(connection, graphics, XCB_GC_FOREGROUND, &colors[colorIndex]->pixel);
				xcb_poly_point(connection, XCB_COORD_MODE_ORIGIN, window, graphics, 1, &pt);
				pthread_mutex_unlock(&mutex_flush);
			}
		}
	}
}

void startMandel() {
	printf("Building image...");
	int            threadIds[NUM_THREADS];
	pthread_attr_t attr;
	pthread_t      threads[NUM_THREADS];

	pthread_mutex_init(&mutex_data, NULL);  // Init mutex for data
	pthread_mutex_init(&mutex_flush, NULL); // Init mutex for flushing
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	next_available_line = 0;

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
			printf("Thread #%i exited with %i\n", i, (int)(uintptr_t)status);
			exit(1);
		}
	}
	printf("done!\n");
	printf("Zoom level:%Lfe-12\tX:%Lfe-12\tY:%Lfe-12\n", graph.scale * 1E12, graph.x * 1E12, graph.y * 1E12);
}

int main() {
	printf("Building X11 window...");

	graph.x     = -0.8; // Initial conditions
	graph.y     = 0.0;
	graph.scale = 0.015;

	connection = xcb_connect(NULL, NULL);
	xcb_generic_event_t * e;
	const xcb_setup_t *   setup       = xcb_get_setup(connection);
	xcb_screen_iterator_t iter        = xcb_setup_roots_iterator(setup);
	xcb_screen_t *        screen      = iter.data;
	uint32_t              w_mask      = XCB_CW_EVENT_MASK;
	uint32_t              w_values[1] = {XCB_EVENT_MASK_BUTTON_PRESS}; // Generate events when a button is pressed

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
						break;
					case 3: // Zoom out with right click
						graph.scale /= 0.5;
						break;
					default:
						break;
				}
				startMandel();
			} break;
			default:
				printf("Unknown event occured\n");
				break;
		}
	}

	xcb_disconnect(connection);
	return 0;
}
