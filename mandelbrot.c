#include <X11/Xlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (600)
#define PALETTE_LENGTH (1 << 16)
#define MAX_ITER (1000)

Display *dpy;
Colormap screen_colormap;
GC       gc;
Window   w;
XColor * palette;

typedef struct {
	double x;
	double y;
	double minx;
	double maxx;
	double miny;
	double maxy;
} GRAPH;

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
		(*pal)[i].red   = (1 << 16) * sin(f1 * i + p1) * width + center;
		(*pal)[i].green = (1 << 16) * sin(f2 * i + p2) * width + center;
		(*pal)[i].blue  = (1 << 16) * sin(f3 * i + p3) * width + center;
		(*pal)[i].flags = DoRed | DoGreen | DoBlue;
		XAllocColor(dpy, screen_colormap, &((*pal)[i]));
	}
}

void mandelbrot(double center_x, double center_y, double minx, double maxx, double miny, double maxy) {
	printf("Building image...");
	double        x0, y0, x, y, xtemp, log_zn, nu, iteration;
	unsigned long colorIndex, c1, c2;
	for (int py = 0; py < SCREEN_HEIGHT; py++) {
		for (int px = 0; px < SCREEN_WIDTH; px++) {
			x0 = scale(px, 0, SCREEN_WIDTH, minx, maxx);
			y0 = scale(py, 0, SCREEN_HEIGHT, miny, maxy);
			x  = center_x;
			y  = center_y;
			for (iteration = 0; x * x + y * y < (1 << 16) && iteration < MAX_ITER; iteration++) {
				xtemp = x * x - y * y + x0;
				y     = 2 * x * y + y0;
				x     = xtemp;
			}

			/*if (iteration < MAX_ITER) {
				log_zn = log(x * x + y * y) / 2.0;
				nu     = log(log_zn / log(2.0)) / 2.0;
				iteration += 1 - nu;
			}*/
			//c1 = (unsigned long)(((int)(iteration) / MAX_ITER) * PALETTE_LENGTH);
			//c2 = (unsigned long)(((int)(iteration + 10) / MAX_ITER) * PALETTE_LENGTH);
			colorIndex = (unsigned long)(iteration / MAX_ITER * PALETTE_LENGTH);
			XSetForeground(dpy, gc, palette[colorIndex].pixel);
			XDrawPoint(dpy, w, gc, px, py);
			XFlush(dpy);
		}
	}
	printf("done!\n");
}

int main() {
	printf("Building X11 window...");
	XEvent xevent;
	int    button, mousex, mousey;
	GRAPH  graph;
	graph.x    = 0.0;
	graph.y    = 0.0;
	graph.minx = -2.5;
	graph.maxx = 1.0;
	graph.miny = -1.0;
	graph.maxy = 1.0;

	dpy = XOpenDisplay(NULL);
	assert(dpy);

	int blackColor  = BlackPixel(dpy, DefaultScreen(dpy));
	screen_colormap = DefaultColormap(dpy, DefaultScreen(dpy));

	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, blackColor, blackColor);

	XSelectInput(dpy, w, StructureNotifyMask); // We want to get MapNotify events
	XMapWindow(dpy, w);                        // "Map" the window (that is, make it appear on the screen)

	gc = XCreateGC(dpy, w, 0, NULL); // Create a "Graphics Context"

	while (1) { // Wait for the MapNotify event
		XNextEvent(dpy, &xevent);
		if (xevent.type == MapNotify)
			break;
	}
	printf("done!\n");

	printf("Building color palette...");
	palette = malloc(PALETTE_LENGTH * sizeof(XColor));
	colorGradient(&palette, 0.3, 0.3, 0.3, 0, 2, 4, 128, 127, PALETTE_LENGTH);
	printf("done!\n");

	mandelbrot(graph.x, graph.y, graph.minx, graph.maxx, graph.miny, graph.maxy); // Initial mandelbrot

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
		graph.x = scale((double)mousex, 0.0, (double)SCREEN_WIDTH, graph.minx, graph.maxx);
		graph.y = scale((double)mousey, 0.0, (double)SCREEN_HEIGHT, graph.miny, graph.maxy);
		if (button == Button1) {
			printf("left click at %i,%i => %f,%f\n", mousex, mousey, graph.x, graph.y);
			graph.minx = 0.9 * (graph.x + graph.minx);
			graph.maxx = 0.9 * (graph.x + graph.maxx);
			graph.miny = 0.9 * (graph.y + graph.miny);
			graph.maxy = 0.9 * (graph.y + graph.maxy);
		} else if (button == Button3) {
			printf("right click at %i,%i => %f,%f\n", mousex, mousey, graph.x, graph.y);
			graph.minx += graph.x;
			graph.maxx += graph.x;
			graph.miny += graph.y;
			graph.maxy += graph.y;
		}
		printf("minx: %f maxx: %f miny: %f maxy: %f\n", graph.minx, graph.maxx, graph.miny, graph.maxy);
		mandelbrot(graph.x, graph.y, graph.minx, graph.maxx, graph.miny, graph.maxy);
	}
	XCloseDisplay(dpy);
	return 0;
}
