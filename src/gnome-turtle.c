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

#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <glib.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "turtle-helpers.h"

#define CANVAS_SIZE 600

extern FILE *yyin;
extern int cpt_items; /* items in canvas */

GtkWidget *window;
GtkWidget *appbar;
GtkWidget *canvas_area;

/* Utility functions */
extern void yyparse (void);
void gui_init (int argc, char **argv);
void gui_set_has_content (gboolean state);

/* Prototypes for the menus */
static void menu_open_cb (GtkWidget *widget, gpointer data);
static void menu_open_ok_cb (GtkWidget *widget, gpointer data);
static void menu_save_as_cb (GtkWidget *widget, gpointer data);
static void menu_print_setup_cb (GtkWidget *widget, gpointer data);
static void menu_print_cb (GtkWidget *widget, gpointer data);
static void menu_about_cb (GtkWidget *widget, gpointer data);
static void menu_quit_cb (GtkWidget *widget, gpointer data);
static void menu_clear_cb (GtkWidget *widget, gpointer data);
static void menu_preferences_cb (GtkWidget *widget, gpointer data);

/* Prototypes for the signals and other events */
// none..

/* ----------- menus start ------------ */
static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_OPEN_ITEM (menu_open_cb, NULL),
	GNOMEUIINFO_MENU_SAVE_AS_ITEM (menu_save_as_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_PRINT_SETUP_ITEM (menu_print_setup_cb, NULL),
	GNOMEUIINFO_MENU_PRINT_ITEM (menu_print_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_QUIT_ITEM (menu_quit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo edit_menu[] = {
	GNOMEUIINFO_MENU_CLEAR_ITEM (menu_clear_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
    GNOMEUIINFO_MENU_PREFERENCES_ITEM (menu_preferences_cb, NULL),
    GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP ("gnome-turtle"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (menu_about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu),
	GNOMEUIINFO_MENU_EDIT_TREE (edit_menu),
	GNOMEUIINFO_MENU_SETTINGS_TREE (settings_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
	GNOMEUIINFO_END
};
/* ----------- menus end ------------ */


/* ------------ menu callbacks start ----------- */
static void
menu_open_cb (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fsel;

	fsel = GTK_FILE_SELECTION(gtk_file_selection_new("Open"));

	/* Connect signals for ok/cancel (cancel kills the widget) */
	gtk_signal_connect(GTK_OBJECT(fsel->ok_button), "clicked",
			GTK_SIGNAL_FUNC(menu_open_ok_cb), fsel);
	gtk_signal_connect_object
		(GTK_OBJECT(fsel->cancel_button), "clicked",
		 GTK_SIGNAL_FUNC(gtk_widget_destroy), 
		 GTK_OBJECT(fsel));

	/* position this dialog where the mouse is, this is normaly
	   set according to preferences for gnome dialogs, but there
	   is yet no file selection dialog in gnome */
	// gtk_window_position(GTK_WINDOW(fsel), GTK_WIN_POS_MOUSE);

	// set parent and show
	gtk_window_set_transient_for (GTK_WINDOW (fsel), GTK_WINDOW (window));
	gtk_widget_show (GTK_WIDGET (fsel));
}

static void
menu_open_ok_cb (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fsel;

	fsel = (GtkFileSelection *) data;
	g_return_if_fail(GTK_IS_FILE_SELECTION(fsel));

	yyin = fopen (gtk_file_selection_get_filename(fsel), "r");

	if (yyin) {
		// all semms ok, close the dialog
		gtk_widget_destroy(GTK_WIDGET(fsel));

		turtle_init_parser ();
		turtle_clear_screen ();
		yyparse ();
		turtle_draw_fractal (canvas_area, NULL);

		gtk_widget_set_sensitive (file_menu[1].widget, TRUE);
		gtk_widget_set_sensitive (file_menu[4].widget, TRUE);
	} else {
		GtkWidget *dlg;

		/* make a new dialog with the file selection as parent */
		dlg = gnome_error_dialog_parented(strerror(errno),
				GTK_WINDOW(fsel));
		gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
	}
}

static void
menu_save_as_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg;

	dlg = gnome_error_dialog_parented (
			"This function is not implemented yet",
			GTK_WINDOW(window));
	gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
	g_print ("menu_save_as clicked\n");
}

static void
menu_print_setup_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg;

	dlg = gnome_error_dialog_parented (
			"This function is not implemented yet",
			GTK_WINDOW(window));
	gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
	g_print ("menu_print_setup_cb clicked\n");
}

/* Most of this is copied from the libgnomeprintui2.2-dev examples */
static void
menu_print_cb (GtkWidget *widget, gpointer data)
{
	GnomePrintContext *gpc;
	GnomePrintJob *job;
	GtkWidget *dialog;
	gint response;

	/* Create the objects */
	job    = gnome_print_job_new (NULL);
	dialog = gnome_print_dialog_new (job, "Gnome Turtle - Print Dialog", 0);
	gpc    = gnome_print_job_get_context (job);

	/* Run the dialog */
	response = gnome_print_dialog_run (GNOME_PRINT_DIALOG (dialog));
	if (response == GNOME_PRINT_DIALOG_RESPONSE_CANCEL) {
		g_print ("Printing was canceled\n");
		return;
	}

	/* Draw & print */
	g_print ("Printing: Generating fractal on the GnomePrintContext\n");
	turtle_draw_fractal (NULL, gpc);
	gnome_print_job_close (job);
	gnome_print_job_print (job);
	g_print ("Printing: Finished\n");

	g_object_unref (G_OBJECT (gpc));
	g_object_unref (G_OBJECT (job));
}

static void
menu_quit_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void
menu_clear_cb (GtkWidget *widget, gpointer data)
{
	turtle_clear_screen ();
}

static void
menu_preferences_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg;

	dlg = gnome_error_dialog_parented (
			"This function is not implemented yet",
			GTK_WINDOW(window));
	gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
	g_print ("turtle_preferences_cb clicked\n");
}

static void
menu_about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf *pixbuf = NULL;

	const gchar *authors[] = {"Mathieu Lutfy", NULL};
	gchar *documenters[] = {
		NULL
	};
	
	/* Translator credits */
	gchar *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW(about));
		return;
	}

	{
		char *filename = NULL;

		filename = gnome_program_locate_file (NULL,
				GNOME_FILE_DOMAIN_APP_PIXMAP, "gnome-turtle.png",
				TRUE, NULL);
		if (filename != NULL) {
			pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
			g_free (filename);
		}
	}

	about = gnome_about_new (_("Gnome-Turtle"), VERSION,
				 "Copyright \xc2\xa9 2004 Mathieu Lutfy",
				 _("A Turtle and Lsys drawing program for GNOME."),
				 (const char **)authors,
				 (const char **)documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 pixbuf);
   
	if (pixbuf != NULL)
		gdk_pixbuf_unref (pixbuf);
	
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (window));
	g_signal_connect (G_OBJECT (about), "destroy", G_CALLBACK
			(gtk_widget_destroyed), &about);
	gtk_widget_show (about);
}
/* ------------ menu callbacks end ----------- */

void
gui_init (int argc, char **argv)
{
	GdkColor white = { 0, 0xffff, 0xffff, 0xffff };

	window = gnome_app_new (PACKAGE, "GNOME Turtle");
	gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
	gtk_widget_realize (window);
	g_signal_connect (G_OBJECT (window), "delete_event",
		G_CALLBACK (menu_quit_cb), NULL);
	
	canvas_area = gnome_canvas_new ();
	gtk_widget_set_usize (canvas_area, CANVAS_SIZE, CANVAS_SIZE);
	gtk_widget_modify_bg (canvas_area, GTK_STATE_NORMAL, &white);
	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (canvas_area), 1);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas_area), 0.0, 0.0, CANVAS_SIZE, CANVAS_SIZE);

	gnome_app_set_contents (GNOME_APP (window), canvas_area);
	gtk_widget_show (canvas_area);

	gnome_app_create_menus (GNOME_APP (window), main_menu);

	appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (window), appbar);
	gnome_app_install_menu_hints (GNOME_APP (window), main_menu);

	if (argc > 1) {
		turtle_init_parser ();
		yyin = fopen (argv[1], "r");
		if (!yyin) {
			g_print ("%s: %s\n", argv[1], strerror(errno));
			exit (1);
		}
		yyparse ();
		turtle_draw_fractal (canvas_area, NULL);

		gui_set_has_content (TRUE);
	} else {
		gui_set_has_content (FALSE);
	}
}

void
gui_set_has_content (gboolean state)
{
	gtk_widget_set_sensitive (file_menu[1].widget, state);
	gtk_widget_set_sensitive (file_menu[4].widget, state);
}

int
main (int argc, char **argv)
{
	GnomeProgram *program;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMETURTLE_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (PACKAGE);
#endif

	/* Initialize gnome program */
	program = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
				argc, argv, 
				GNOME_PARAM_POPT_TABLE, NULL, /* options, */
				GNOME_PARAM_HUMAN_READABLE_NAME, _("Turtle Drawing"),
				GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-turtle.png");

	/* Important: for strtod(3) and decimal number */
	// TODO: Maybe make this a preference, or autodetect it
	setlocale (LC_NUMERIC, "C");

	gui_init (argc, argv);
	gtk_widget_show (window);

	gtk_main ();

	return 0;
}

