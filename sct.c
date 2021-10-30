/*
 * sct - set color temperature
 *
 * Compile the code using the following command:
 * cc -std=c99 -O2 -I /usr/X11R6/include -o sct sct.c -L /usr/X11R6/lib -lm -lX11 -lXrandr
 *
 * Original code published by Ted Unangst:
 * http://www.tedunangst.com/flak/post/sct-set-color-temperature
 *
 * Modified by Fabian Foerg in order to:
 * - compile on Ubuntu 14.04
 * - iterate over all screens of the default display and change the color
 *   temperature
 * - fix memleaks
 * - clean up code
 * - return EXIT_SUCCESS
 *
 * Public domain, do as you wish.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

/* cribbed from redshift, but truncated with 500K steps */
static const struct { float r; float g; float b; } whitepoints[] = {
	{ 1.0, 0.0, 0.0},
	{ 1.00000000,  0.18172716,  0.00000000, }, /* 1000K */
	{ 1.00000000,  0.42322816,  0.00000000, },
	{ 1.00000000,  0.54360078,  0.08679949, },
	{ 1.00000000,  0.64373109,  0.28819679, },
	{ 1.00000000,  0.71976951,  0.42860152, },
	{ 1.00000000,  0.77987699,  0.54642268, },
	{ 1.00000000,  0.82854786,  0.64816570, },
	{ 1.00000000,  0.86860704,  0.73688797, },
	{ 1.00000000,  0.90198230,  0.81465502, },
	{ 1.00000000,  0.93853986,  0.88130458, },
	{ 1.00000000,  0.97107439,  0.94305985, },
	{ 1.00000000,  1.00000000,  1.00000000, }, /* 6500K */
	{ 0.95160805,  0.96983355,  1.00000000, },
	{ 0.91194747,  0.94470005,  1.00000000, },
	{ 0.87906581,  0.92357340,  1.00000000, },
	{ 0.85139976,  0.90559011,  1.00000000, },
	{ 0.82782969,  0.89011714,  1.00000000, },
	{ 0.80753191,  0.87667891,  1.00000000, },
	{ 0.78988728,  0.86491137,  1.00000000, }, /* 10000K */
	{ 0.77442176,  0.85453121,  1.00000000, },
};

static void
sct_for_screen(Display *dpy, int screen, int temp, double brightness)
{
	Window root = RootWindow(dpy, screen);
	XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);

	temp -= 500;
	double ratio = temp % 500 / 500.0;
#define AVG(c) whitepoints[temp / 500].c * (1 - ratio) + whitepoints[temp / 500 + 1].c * ratio
	double gammar = brightness * AVG(r);
	double gammag = brightness * AVG(g);
	double gammab = brightness * AVG(b);

	for (int c = 0; c < res->ncrtc; c++) {
		int crtcxid = res->crtcs[c];
		int size = XRRGetCrtcGammaSize(dpy, crtcxid);

		XRRCrtcGamma *crtc_gamma = XRRAllocGamma(size);

		for (int i = 0; i < size; i++) {
			double g = 65535.0 * i / size;
			crtc_gamma->red[i] = g * gammar;
			crtc_gamma->green[i] = g * gammag;
			crtc_gamma->blue[i] = g * gammab;
		}

		XRRSetCrtcGamma(dpy, crtcxid, crtc_gamma);
		XFree(crtc_gamma);
	}

	XFree(res);
}

int
main(int argc, char **argv)
{
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) {
		perror("XOpenDisplay(NULL) failed");
		fprintf(stderr, "Make sure DISPLAY is set correctly.\n");
		return EXIT_FAILURE;
	}
	int screens = XScreenCount(dpy);

	int temp = 6500;
	if (argc > 1) {
		temp = atoi(argv[1]);
		if (strcmp(argv[1], "-r") == 0)
			temp = 500;
	}
	if (temp < 500 || temp > 10000)
		temp = 6500;

	double brightness = 1.0;
	if (argc > 2)
		brightness = atof(argv[2]);
	if (brightness < 0.1 || brightness > 1.0)
		brightness = 1.0;

	for (int screen = 0; screen < screens; screen++)
		sct_for_screen(dpy, screen, temp, brightness);

	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

