#include <X11/Xlib.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#define HEIGHT (600)
#define WIDTH (800)
#define PALETTE_LENGTH (10000)
#define MAX_ITER (1000)

Display *dpy;
Colormap screen_colormap;

double scale(double point, double minFrom, double maxFrom, double minTo, double maxTo) {
	return (((maxTo - minTo) * (point - minFrom)) / (maxFrom - minFrom)) + minTo;
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

int main() {
	printf("Building X11 window...");
	dpy = XOpenDisplay(NULL);
	assert(dpy);

	int blackColor  = BlackPixel(dpy, DefaultScreen(dpy));
	int whiteColor  = WhitePixel(dpy, DefaultScreen(dpy));
	screen_colormap = DefaultColormap(dpy, DefaultScreen(dpy));

	Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
	                               WIDTH, HEIGHT, 0, whiteColor, whiteColor);

	XSelectInput(dpy, w, StructureNotifyMask); // We want to get MapNotify events
	XMapWindow(dpy, w);                        // "Map" the window (that is, make it appear on the screen)

	GC gc = XCreateGC(dpy, w, 0, NULL); // Create a "Graphics Context"

	while (1) { // Wait for the MapNotify event
		XEvent e;
		XNextEvent(dpy, &e);
		if (e.type == MapNotify)
			break;
	}
	printf("done!\n");

	printf("Building color palette...");
	XColor *palette = malloc(PALETTE_LENGTH * sizeof(XColor));
	colorGradient(&palette, 0.3, 0.3, 0.3, 0, 2, 4, 128, 127, PALETTE_LENGTH);
	printf("done!\n");

	printf("Building image...");
	double        x0, y0, x, y, xtemp, log_zn, nu;
	int           iteration;
	unsigned long colorIndex, c1, c2;
	for (int py = 0; py < HEIGHT; py++) {
		for (int px = 0; px < WIDTH; px++) {
			x0 = scale(px, 0, WIDTH, -2.5, 1.0);
			y0 = scale(py, 0, HEIGHT, -1.0, 1.0);

			x = 0.0;
			y = 0.0;
			for (iteration = 0; x * x + y * y < 2 * 2 && iteration < MAX_ITER; iteration++) {
				xtemp = x * x - y * y + x0;
				y     = 2 * x * y + y0;
				x     = xtemp;
			}

			if (iteration < MAX_ITER) {
				log_zn = log(x * x + y * y) / 2.0;
				nu     = log(log_zn / log(2.0)) / 2.0;
				iteration += 1 - nu;
			}
			c1 = (unsigned long)((1.0 * iteration / MAX_ITER) * PALETTE_LENGTH);
			c2 = (unsigned long)((1.0 * iteration / MAX_ITER) * PALETTE_LENGTH) + 100;

			XSetForeground(dpy, gc, palette[].pixel);
			XDrawPoint(dpy, w, gc, px, py);
			XFlush(dpy);
		}
	}
	printf("done!\n");

	sleep(15);
	return 0;
}
