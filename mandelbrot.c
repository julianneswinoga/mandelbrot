#include <X11/Xlib.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCREEN_WIDTH (200)
#define SCREEN_HEIGHT (200)
#define PALETTE_LENGTH (1000)
#define MAX_ITER (1000)
#define NUM_THREADS (8)
#define THREAD_BLOCKSIZE (2000)
#define SCALE_FACTOR (0.5)

typedef struct {
	double x;
	double y;
	double scale;
} GRAPH;

Display *       dpy;
Colormap        screen_colormap;
GC *            gcs;
Window          w;
XColor *        palette;
GRAPH           graph;
unsigned long   pixelNum;
pthread_mutex_t mutex_data, mutex_flush;

double scale(double point, double minFrom, double maxFrom, double minTo, double maxTo) {
	return (((maxTo - minTo) * (point - minFrom)) / (maxFrom - minFrom)) + minTo;
}

double interpolate(double x, int x0, int y0, int x1, int y1) {
	return (y0 * (x1 - x) + y1 * (x - x0)) / (x1 - x0);
}

/**
 * https://krazydad.com/tutorials/makecolors.php
 */
void colorGradient(XColor **pal, double f1, double f2, double f3, double p1, double p2, double p3, double center, double width, int len) {
	for (int i = 0; i < len; i++) {
		(*pal)[i].red   = sin(f1 * i + p1) * width + center;
		(*pal)[i].green = sin(f2 * i + p2) * width + center;
		(*pal)[i].blue  = sin(f3 * i + p3) * width + center;
		(*pal)[i].flags = DoRed | DoGreen | DoBlue;
		XAllocColor(dpy, screen_colormap, &((*pal)[i]));
	}
}

void *mandelThread(void *arg) {
	int threadNumber = (int)(uintptr_t)arg;
	printf("Thread #%i created\n", threadNumber);

	double        x0, y0, x, y, xtemp, iteration, q;
	unsigned long colorIndex;
	unsigned long pixelNumStart, pixelNumCurrent;

	while (1) {
		pthread_mutex_lock(&mutex_data);
		if (pixelNum >= SCREEN_WIDTH * SCREEN_HEIGHT) { // No more work to be done
			pthread_mutex_unlock(&mutex_data);
			pthread_exit(NULL);
			return NULL;
		} else {
			pixelNumStart   = pixelNum;
			pixelNumCurrent = pixelNum;
			pixelNum += THREAD_BLOCKSIZE;
		}
		pthread_mutex_unlock(&mutex_data);

		int  py, px;
		long I = pixelNumStart;
		for (py = 0; py < SCREEN_HEIGHT && I - SCREEN_HEIGHT > 0; py++) {
			I -= SCREEN_HEIGHT;
		}

		for (px = 0; px < SCREEN_WIDTH && I > 0; px++) {
			I--;
		}

		for (; pixelNumCurrent < pixelNumStart + THREAD_BLOCKSIZE && py < SCREEN_HEIGHT; py++) {
			for (; pixelNumCurrent < pixelNumStart + THREAD_BLOCKSIZE && px < SCREEN_WIDTH; px++) {
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
				pixelNumCurrent++;

				pthread_mutex_lock(&mutex_flush);
				XDrawPoint(dpy, w, gcs[colorIndex], px, py);
				XFlush(dpy);
				pthread_mutex_unlock(&mutex_flush);
			}
			px = 0;
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
	pixelNum = 0;

	for (int i = 0; i < NUM_THREADS; i++) { // Create threads
		threadIds[i] = i;
		pthread_create(&threads[i], &attr, mandelThread, (void *)(uintptr_t)threadIds[i]);
	}

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
}

int main() {
	printf("Building X11 window...");

	XEvent xevent;
	int    button, mousex, mousey;
	graph.x     = 0.0;
	graph.y     = 0.0;
	graph.scale = 0.02;

	dpy = XOpenDisplay(NULL);
	assert(dpy);

	int blackColor  = BlackPixel(dpy, DefaultScreen(dpy));
	screen_colormap = DefaultColormap(dpy, DefaultScreen(dpy));

	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, blackColor, blackColor);

	XSelectInput(dpy, w, StructureNotifyMask); // We want to get MapNotify events
	XMapWindow(dpy, w);                        // "Map" the window (that is, make it appear on the screen)

	while (1) { // Wait for the MapNotify event
		XNextEvent(dpy, &xevent);
		if (xevent.type == MapNotify)
			break;
	}
	printf("done!\n");

	printf("Building color palette...");
	palette = malloc(PALETTE_LENGTH * sizeof(XColor));
	colorGradient(&palette, 0.3, 0.3, 0.3, 0, 2, 4, (1 << 15), (1 << 15), PALETTE_LENGTH);
	printf("done!\n");

	gcs = malloc(PALETTE_LENGTH * sizeof(GC) + 1); // Allocate graphics contexts of PALETTE_LENGTH size + 1 for black
	XGCValues xgcv;
	for (int i = 0; i < PALETTE_LENGTH; i++) {
		xgcv.foreground = palette[i].pixel;
		gcs[i]          = XCreateGC(dpy, w, GCForeground, &xgcv);
		XFillRectangle(dpy, w, gcs[i], (int)(i * (1.0 * SCREEN_WIDTH / PALETTE_LENGTH)), 0, (int)(1.0 * SCREEN_WIDTH / PALETTE_LENGTH) + 1, SCREEN_HEIGHT - 1);
	}
	gcs[PALETTE_LENGTH] = XCreateGC(dpy, w, 0, NULL); // Set last color to black
	XFlush(dpy);

	sleep(3);

	startMandel(); // Initial mandelbrot

	XSelectInput(dpy, w, ButtonPressMask);
	while (1) {
		mousex = -1;
		mousey = -1;
		while (mousex <= 0 && mousey <= 0) {
			XNextEvent(dpy, &xevent); // Blocking
			switch (xevent.type) {
				case ButtonPress:
					switch (xevent.xbutton.button) {
						case Button1: // Left click
							mousex = xevent.xbutton.x;
							mousey = xevent.xbutton.y;
							button = Button1;
							break;

						case Button3: // Right click
							mousex = xevent.xbutton.x;
							mousey = xevent.xbutton.y;
							button = Button3;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		}
		graph.x += graph.scale * (mousex - SCREEN_WIDTH / 2);
		graph.y += graph.scale * (mousey - SCREEN_HEIGHT / 2);

		if (button == Button1) { // Zooming
			graph.scale *= 0.5;
		} else if (button == Button3) { // Panning
		}
		//printf("\t%f\n%f\t%f\n\t%f\n\tat(%f,%f)\n", graph.maxy, graph.minx, graph.maxx, graph.miny, graph.x, graph.y);
		startMandel();
	}
	XCloseDisplay(dpy);
	return 0;
}
