/*
   This file is part of gnome-turtle
   Written by: Mathieu Lutfy <mathieu@bidon.ca>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   For further information, please contact at mathieu@bidon.ca
*/

#include <math.h>  /* sin(3), cos(3) */
#include <ctype.h> /* toupper(3) */
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "turtle-helpers.h"

#define MAX_BUFFER 1024
#define MAX_STACK 50

LsysDef lsys;
static unsigned char buffer[MAX_BUFFER];

/* State for the recursive walk. */
static GArray *segments;
static double curx, cury;   /* the pen position */
static StackItem stack[MAX_STACK];
static int top;
static double angle;

/* TODO: Implement a zoom functionality which would adjust this factor. */
#define AMP 5

static void
turtle_emit (double x2, double y2, int color)
{
	TurtleSegment seg = { curx, cury, x2, y2, color };
	g_array_append_val (segments, seg);
}

static void
turtle_step (int depth, double amplify, gboolean draw, int color)
{
	double x2, y2;

	x2 = curx + pow (lsys.sigma, depth) * lsys.xscale * amplify * cos (angle) * AMP;
	y2 = cury + pow (lsys.sigma, depth) * lsys.yscale * amplify * sin (angle) * AMP;

	if (draw)
		turtle_emit (x2, y2, color);

	curx = x2;
	cury = y2;
}

static void
turtle_draw_fractal_recursion (unsigned char c, int depth)
{
	unsigned char *p;
	double amplify = 1;
	int col = 0;

	if (c >= 128) {
		/* extract special rule! */
		if (lsys.special_rule[c - 128] != NULL) {
			/* don't forget to extract the amplify before trashing the 'c' ;) */
			amplify = lsys.special_rule[c - 128]->amplify;
			c = lsys.special_rule[c - 128]->rule;
			col = 2;
		} else {
			g_warning ("Special Rule %d does not exist!", c);
		}
	}

	if ('a' <= c && c <= 'z') {
		col = 1; /* draw in white */
		c = toupper (c);
	}

	if (c == '$') {
		col = 2; /* green */
		c = '|';
	}

	if ('A' <= c && c < 'Z') {
		if (depth <= lsys.max_depth) {
			for (p = lsys.rule[c - 'A']; p && *p; p++)
				turtle_draw_fractal_recursion (*p, depth + 1);
			return;
		}

		turtle_step (depth, amplify, TRUE, col);
	} else if (c == '+') {
		angle += lsys.tetha;
	} else if (c == '-') {
		angle -= lsys.tetha;
	} else if (c == '|') {
		turtle_step (depth, amplify, TRUE, col);
	} else if (c == '[') {
		if (top >= MAX_STACK) {
			g_warning ("Stack full! (too much recursion?)");
			return;
		}
		stack[top].x = curx;
		stack[top].y = cury;
		stack[top].angle = angle;
		top++;
	} else if (c == ']') {
		if (top <= 0) {
			g_warning ("Stack was empty! (badly put ']'?)");
			return;
		}
		top--;
		curx = stack[top].x;
		cury = stack[top].y;
		angle = stack[top].angle;
	} else if (c == '%') {
		/* empty rule */
		return;
	} else {
		g_debug ("hmm? %d - %c", (int) c, c);
	}
}

GArray *
turtle_generate (void)
{
	unsigned char *p;

	segments = g_array_new (FALSE, FALSE, sizeof (TurtleSegment));

	angle = lsys.start_angle;
	top = 0;
	curx = lsys.xinit * AMP;
	cury = lsys.yinit * AMP;

	for (p = lsys.axiom; p && *p; p++)
		turtle_draw_fractal_recursion (*p, 0);

	return segments;
}

void
turtle_render (cairo_t *cr, GArray *segments)
{
	/* palette: 0 black, 1 white, 2 green, 3 blue -- matches the original */
	static const double palette[][3] = {
		{ 0.0, 0.0, 0.0 },
		{ 1.0, 1.0, 1.0 },
		{ 0.0, 1.0, 0.0 },
		{ 0.0, 0.0, 1.0 },
	};
	guint i;

	if (segments == NULL)
		return;

	cairo_set_line_width (cr, 1.0);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	for (i = 0; i < segments->len; i++) {
		TurtleSegment *s = &g_array_index (segments, TurtleSegment, i);
		const double *rgb = palette[s->color & 3];

		cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
		cairo_move_to (cr, s->x1, s->y1);
		cairo_line_to (cr, s->x2, s->y2);
		cairo_stroke (cr);
	}
}

void
turtle_init_parser (void)
{
	int i;

	memset (buffer, 0, sizeof (buffer));

	for (i = 0; i < 'Z' - 'A'; i++) {
		lsys.rule[i] = NULL;
		lsys.special_rule[i] = NULL;
	}

	/* safe defaults */
	lsys.start_angle = M_PI / 2;
	lsys.max_depth = 3;
	lsys.xinit = 10.0;
	lsys.yinit = 10.0;
	lsys.xscale = 1.0;
	lsys.yscale = 1.0;
}

void
turtle_add_to_buffer (unsigned char c)
{
	size_t size = strlen ((char *) buffer);

	if (size < sizeof (buffer) - 1)
		buffer[size] = c;
	else
		g_critical ("turtle_add_to_buffer: buffer full!");
}

static void
turtle_debug_print_symbols (void)
{
	unsigned char *p;
	GString *out = g_string_new (NULL);

	for (p = buffer; *p; p++) {
		if (*p >= 128) {
			if (lsys.special_rule[*p - 128] != NULL) {
				g_string_append_c (out, lsys.special_rule[*p - 128]->rule);
				g_string_append_c (out, lsys.special_rule[*p - 128]->amplify + '0');
			} else {
				g_string_append (out, "[error decoding special rule]");
			}
		} else {
			g_string_append_c (out, *p);
		}
	}

	g_debug ("%s", out->str);
	g_string_free (out, TRUE);
}

void
turtle_axiom_ready (void)
{
	g_debug ("Valid axiom:");
	turtle_debug_print_symbols ();

	lsys.axiom = (unsigned char *) g_strdup ((char *) buffer);
	memset (buffer, 0, sizeof (buffer));
}

void
turtle_rule_ready (unsigned char rulename)
{
	int index;

	g_debug ("Valid rule: [%c]", rulename);
	turtle_debug_print_symbols ();

	index = toupper (rulename) - 'A';

	lsys.rule[index] = (unsigned char *) g_strdup ((char *) buffer);
	memset (buffer, 0, sizeof (buffer));
}

/*
 * The "start_angle" is the initial angle of the turtle. It's
 * best to start it as 90 (M_PI / 2) so that it looks up, as
 * many fractals who require orientation look like trees.
 */
void
turtle_set_angle (double angle)
{
	lsys.start_angle = - degres_to_gradians (angle);
	g_debug ("start_angle = %f", lsys.start_angle);
}

/*
 * Sigma is the size factor between depths.
 * Recommended: 0 < sigma <= 1 (to make each depth smaller).
 */
void
turtle_set_sigma (double sigma)
{
	lsys.sigma = sigma;
	g_debug ("sigma = %f", lsys.sigma);
}

/*
 * Tetha is the angle represented by rotations representated
 * by + / - symbols.
 */
void
turtle_set_tetha (double tetha)
{
	lsys.tetha = degres_to_gradians (tetha);
}

/*
 * Max_depth is the maximum number of iterations made by
 * the turtle.
 */
void
turtle_set_depth (int depth)
{
	lsys.max_depth = depth;
}

void
turtle_set_xyinit (double x, double y)
{
	lsys.xinit = x;
	lsys.yinit = y;
}

void
turtle_set_xyscale (double x, double y)
{
	lsys.xscale = x;
	lsys.yscale = y;
}

double
degres_to_gradians (double deg)
{
	return deg / 90 / 2 * M_PI;
}

unsigned char
turtle_create_special_rule (unsigned char symbol, double amplify)
{
	int i;

	/* NOTE: we return "i + 128" so the result is >= 128, and therefore
	   understood by the parser/renderer as being a special rule. */

	if (!('A' <= symbol && symbol <= 'Z')) {
		g_warning ("Not a valid symbol! (this should never get past the parser)");
		return 0;
	}

	/* i = 1, because if c == 0 it may look like NULL */
	for (i = 1; i < 'Z' - 'A'; i++) {
		if (lsys.special_rule[i] != NULL) {
			/* if it already exists, just return it */
			if (lsys.special_rule[i]->rule == symbol && lsys.special_rule[i]->amplify == amplify)
				return (unsigned char) (i) + 128;
		} else {
			lsys.special_rule[i] = g_new (SpecialRule, 1);
			lsys.special_rule[i]->rule = symbol;
			lsys.special_rule[i]->amplify = amplify;

			g_debug ("Added special rule: %c : %f : [i = %d]", symbol, amplify, i);
			return (unsigned char) (i) + 128;
		}
	}

	g_warning ("No more space for special rules..");
	return 0;
}
