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
#include <glib.h>
#include <gnome.h>
#include <config.h>

#include "turtle-helpers.h"

#define MAX_BUFFER 1024
#define MAX_STACK 50
#define MAX_ITEMS 16384 /* They stay in memory anyway */

LsysDef lsys;
static unsigned char buffer[MAX_BUFFER];

// for the recursive drawing
static GnomeCanvasPoints *line;
static StackItem stack[MAX_STACK];
static int top;
static double angle;

GnomeCanvasItem *all_items[MAX_ITEMS];
int cpt_items = 0;
gboolean complain = TRUE;

// TODO: Implement a zoom functionality which would adjust this function
#define AMP 5

static void
turtle_add_to_all_items (GnomeCanvasItem *item)
{
	if (cpt_items < MAX_ITEMS)
		all_items[cpt_items++] = item;
	else {
		if (complain) {
			/* complain only once */
			g_print ("turtle_add_to_all_items: list full!\n");
			complain = FALSE;
		}
	}
}

static void
turtle_draw_fractal_recursion (GtkWidget *canvas, GnomePrintContext *gpc, unsigned char c, int depth)
{
	GnomeCanvasItem *item;
	unsigned char *p;
	double amplify;
	char *colours[] = { "black", "white", "green", "blue" };
	int col = 0;

	amplify = 1;

	if (c >= 128) {
		// extract special rule!
		if (lsys.special_rule[c - 128] != NULL) {
			// don't forget to extract the amplify before trashing the 'c' ;)
			amplify = lsys.special_rule[c - 128]->amplify;
			c = lsys.special_rule[c - 128]->rule;
			col = 2;
		} else {
			g_print ("Error: Special Rule %d does not exist!\n", c);
		}
	}

	if ('a' <= c && c <= 'z') {
		col = 1; // draw in white
		c = toupper(c);
	}

	if (c == '$') {
		col = 2; // green
		c = '|';
	}

	if ('A' <= c && c < 'Z') {
		if (depth <= lsys.max_depth) {
			for (p = lsys.rule[c - 'A']; *p; p++) {
				turtle_draw_fractal_recursion (canvas, gpc, *p, depth + 1);
			}

			return;
		}

		line->coords[2] = line->coords[0] + pow (lsys.sigma, depth) * lsys.xscale * amplify * cos(angle) * AMP;
		line->coords[3] = line->coords[1] + pow (lsys.sigma, depth) * lsys.yscale * amplify * sin(angle) * AMP;

		if (gpc) {
			gnome_print_moveto (gpc, line->coords[0], line->coords[1]);
			gnome_print_lineto (gpc, line->coords[2], line->coords[3]);
			gnome_print_stroke (gpc);
		}

		if (canvas) {
			item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (canvas)),
					GNOME_TYPE_CANVAS_LINE,
					"points", line,
					"fill_color", colours[col],
					NULL);

			turtle_add_to_all_items (item);
		}

		line->coords[0] = line->coords[2];
		line->coords[1] = line->coords[3];
	} else if (c == '+') {
		angle += lsys.tetha;
	} else if (c == '-') {
		angle -= lsys.tetha;
	} else if (c == '$') {
		/* Move forward, do not draw */
		line->coords[2] = line->coords[0] + pow (lsys.sigma, depth) * lsys.xscale * amplify * cos(angle) * AMP;
		line->coords[3] = line->coords[1] + pow (lsys.sigma, depth) * lsys.yscale * amplify * sin(angle) * AMP;
		line->coords[0] = line->coords[2];
		line->coords[1] = line->coords[3];
	} else if (c == '|') {
		line->coords[2] = line->coords[0] + pow (lsys.sigma, depth) * lsys.xscale * amplify * cos(angle) * AMP;
		line->coords[3] = line->coords[1] + pow (lsys.sigma, depth) * lsys.yscale * amplify * sin(angle) * AMP;

		item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (canvas)),
				GNOME_TYPE_CANVAS_LINE,
				"points", line,
				"fill_color", colours[col],
				NULL);

		turtle_add_to_all_items (item);
		line->coords[0] = line->coords[2];
		line->coords[1] = line->coords[3];
	} else if (c == '[') {
		if (top >= MAX_STACK) {
			g_print ("WARNING: Stack full! (too much recursion?)\n");
			return;
		}

		stack[top].line->coords[0] = line->coords[0];
		stack[top].line->coords[1] = line->coords[1];
		stack[top].line->coords[2] = line->coords[2];
		stack[top].line->coords[3] = line->coords[3];
		stack[top].angle = angle;
		top++;
	} else if (c == ']') {
		if (top <= 0) {
			g_print ("WARNING: Stack was empty! (badly put ']'?)\n");
			return;
		}

		top--;
		line->coords[0] = stack[top].line->coords[0];
		line->coords[1] = stack[top].line->coords[1];
		line->coords[2] = stack[top].line->coords[2];
		line->coords[3] = stack[top].line->coords[3];
		angle = stack[top].angle;
	} else if (c == '%') {
		/* empty rule */
		return;
	} else {
		g_print("hmm? %d - %c\n", (int) c, c);
	}
}


void
turtle_draw_fractal (GtkWidget *canvas, GnomePrintContext *gpc)
{ 
	unsigned char *p;
	int i;

	if (gpc) {
		gnome_print_beginpage (gpc, "1");
		gnome_print_moveto (gpc, lsys.xinit * AMP, lsys.yinit * AMP);
	}
	
	angle = lsys.start_angle;
	top = 0;
	
	line = gnome_canvas_points_new (2);
	line->coords[0] = lsys.xinit * AMP;
	line->coords[1] = lsys.yinit * AMP;

	for (i = 0; i < MAX_STACK; i++) {
		stack[i].line = gnome_canvas_points_new (2);
		stack[i].angle = 0;
	}

	for (p = lsys.axiom; *p; p++) {
		turtle_draw_fractal_recursion (canvas, gpc, *p, 0);
	}

	if (gpc)
		gnome_print_showpage (gpc);
}

void
turtle_clear_screen (void)
{
	int i;

	for (i = 0; i < cpt_items; i++)
		gtk_object_destroy (GTK_OBJECT(all_items[i]));

	cpt_items = 0;
	gui_set_has_content (FALSE);
}

void
turtle_init_parser (void)
{
	int i;

	memset (buffer, 0, sizeof (buffer));

	// XXX
	// memset (lsys.rule, 0, sizeof (lsys.rule) * ('Z' - 'A'));
	// memset (lsys.special_rule, 0, sizeof (lsys.special_rule) * ('Z' - 'A'));
	for (i = 0; i < 'Z' - 'A'; i++) {
		lsys.rule[i] = NULL;
		lsys.special_rule[i] = NULL;
	}

	// safe defaults
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
	int size;

	size = strlen (buffer);

	if (size < sizeof (buffer))
		buffer[size] = c;
	else
		g_print ("[critical] turtle_add_to_buffer: buffer full!\n");
}

void
turtle_debug_print_symbols (char *input)
{
	char *p;

	for (p = buffer; *p; p++) {
		if (*p >= 128) {
			if (lsys.special_rule[*p - 128] != NULL) {
				putchar (lsys.special_rule[*p - 128]->rule);
				putchar (lsys.special_rule[*p - 128]->amplify + '0');
			} else {
				g_print ("[error decoding special rule]\n");
			}
		} else {
			putchar (*p);
		}
	}
}

void
turtle_axiom_ready (void)
{
	g_print ("Valid axiom: ");
	turtle_debug_print_symbols (buffer);
	putchar('\n');
	
	lsys.axiom = strdup (buffer);
	memset (buffer, 0, sizeof(buffer));
}

void
turtle_rule_ready (unsigned char rulename)
{
	int index;

	g_print ("Valid rule: [%c] ", rulename);
	turtle_debug_print_symbols (buffer);
	putchar('\n');

	index = toupper(rulename) - 'A';

	lsys.rule[index] = strdup (buffer);
	memset (buffer, 0, sizeof(buffer));
}

/*
 * The "start_angle" is the initial angle of the turtle. It's
 * best to start it as 90° (M_PI / 2) so that it looks up, as
 * many fractals who require orientation look like trees.
 */
void
turtle_set_angle (double angle)
{
	lsys.start_angle = - degres_to_gradians(angle);
	g_print ("start_angle = %f\n", lsys.start_angle);
}

/*
 * Sigma is the size factor between depths.
 * Recommended: 0 < sigma <= 1 (to make each depth smaller).
 */
void
turtle_set_sigma (double sigma)
{
	lsys.sigma = sigma;
	g_print ("sigma = %f\n", lsys.sigma);
}

/*
 * Tetha is the angle represented by rotations representated
 * by + / - symbols.
 */
void
turtle_set_tetha (double tetha)
{
	lsys.tetha = degres_to_gradians(tetha);
}

/*
 * Max_depth is the maximum number of iterations made by
 * the turtle. For example, if you have an axiom of "F" and
 * a rule "F -> F[+F]", then F will be examped max_depth times
 * to avoid infinite loops.
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

inline double
degres_to_gradians (double deg)
{
    return deg / 90 / 2 * M_PI;
}

unsigned char
turtle_create_special_rule (unsigned char symbol, double amplify)
{
	int i;

	// NOTE: we return "- i" so that the result is under 0, and therefore,
	// understood by the parser as being a special rule
	// XXX: Instead, let's try i + 128, to stay in unsigned char..
	
	if (!('A' <= symbol && symbol <= 'Z')) {
		g_print ("Not valid symbol! (this should never get passed the parser)\n");
		return 0; // XXX
	}

	// XXX i = 1, because if c == 0, may be NULL
	for (i = 1; i < 'Z' - 'A'; i++) {
		if (lsys.special_rule[i] != NULL) {
			// if it already exists, just return it
			if (lsys.special_rule[i]->rule == symbol && lsys.special_rule[i]->amplify == amplify)
				return (unsigned char) (i) + 128;
		
		} else if (lsys.special_rule[i] == NULL) {
			lsys.special_rule[i] = (SpecialRule *) malloc (sizeof(SpecialRule));

			lsys.special_rule[i]->rule = symbol;
			lsys.special_rule[i]->amplify = amplify;

			g_print ("Added special rule: %c : %f : [i = %d]\n", symbol, amplify, i);

			return (unsigned char) (i) + 128;
		}
	}

	// no space ?!
	g_print ("No more space for special rules..\n");
	return 0;
}

void
turtle_print_draw (GnomePrintContext *gpc)
{
	gnome_print_beginpage (gpc, "1");

	gnome_print_moveto (gpc, 100, 100);
	gnome_print_lineto (gpc, 200, 200);
	gnome_print_stroke (gpc);

	gnome_print_showpage (gpc);
}
