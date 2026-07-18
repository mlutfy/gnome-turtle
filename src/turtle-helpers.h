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

#ifndef _TURTLEHELPERS_H_
#define _TURTLEHELPERS_H_

#include <glib.h>
#include <cairo.h>

/*
 * A single drawn line segment, in canvas coordinates. The turtle engine
 * produces a list of these; a renderer (screen or printer) then draws them
 * with Cairo. This replaces the old direct-to-GnomeCanvas / libgnomeprint
 * coupling.
 */
typedef struct {
	double x1, y1;
	double x2, y2;
	int    color;   /* index into the palette in turtle_render() */
} TurtleSegment;

/* Turtle state saved by '[' and restored by ']'. */
typedef struct {
	double x, y;
	double angle;
} StackItem;

typedef struct {
	double amplify;
	unsigned char rule;
} SpecialRule;

typedef struct {
	unsigned char *axiom;
	unsigned char *rule['Z' - 'A'];
	SpecialRule *special_rule['Z' - 'A'];
	double xinit;
	double yinit;
	double xscale;
	double yscale;
	double start_angle;
	double tetha;
	double sigma; /* 0 < sigma <= 1 */
	int max_depth;
} LsysDef;

/*
 * Walk the currently-parsed L-System and return the fractal as a freshly
 * allocated GArray of TurtleSegment. The caller owns it (g_array_unref).
 */
GArray *turtle_generate (void);

/* Render a segment list produced by turtle_generate() onto a Cairo context. */
void turtle_render (cairo_t *cr, GArray *segments);

void turtle_init_parser (void);
void turtle_add_to_buffer (unsigned char c);

void turtle_set_angle (double angle);
void turtle_set_sigma (double sigma);
void turtle_set_tetha (double tetha);
void turtle_set_depth (int depth);
void turtle_set_xyinit (double x, double y);
void turtle_set_xyscale (double x, double y);
void turtle_axiom_ready (void);
void turtle_rule_ready (unsigned char rulename);
unsigned char turtle_create_special_rule (unsigned char symbol, double amplify);

double degres_to_gradians (double deg);

#endif /* _TURTLEHELPERS_H_ */
