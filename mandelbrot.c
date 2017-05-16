#include <X11/Xlib.h> // Every Xlib program must include this
#include <assert.h>   // I include this to test return values the lazy way
#include <unistd.h>   // So we got the profile for 10 seconds

int main() {

	Display *dpy = XOpenDisplay(NULL);
	assert(dpy);

	int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

	Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
	                               200, 100, 0, blackColor, blackColor);

	XSelectInput(dpy, w, StructureNotifyMask); // We want to get MapNotify events
	XMapWindow(dpy, w);                        // "Map" the window (that is, make it appear on the screen)

	GC gc = XCreateGC(dpy, w, 0, NULL); // Create a "Graphics Context"

	XSetForeground(dpy, gc, whiteColor); // Tell the GC we draw using the white color

	while (1) { // Wait for the MapNotify event
		XEvent e;
		XNextEvent(dpy, &e);
		if (e.type == MapNotify)
			break;
	}

	XDrawLine(dpy, w, gc, 10, 60, 180, 20);
	XFlush(dpy);

	sleep(5);
	return 0;
}
