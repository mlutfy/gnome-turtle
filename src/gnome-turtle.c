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
#include <errno.h>
#include <stdio.h>

#include <adwaita.h>
#include <glib/gi18n.h>

#include "turtle-helpers.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef VERSION
#  define VERSION "0.2.0"
#endif

#define CANVAS_SIZE 600

/* From the flex/bison parser. */
extern FILE *yyin;
extern int yyparse (void);

typedef struct {
	AdwApplicationWindow *window;
	GtkWidget *drawing_area;
	GArray *segments;   /* current fractal, owned here */
	char *title;        /* basename of the loaded file, or NULL */
} TurtleApp;

/* ----------------------------------------------------------------- */

static void
turtle_draw_cb (GtkDrawingArea *area,
                cairo_t *cr,
                int width,
                int height,
                gpointer user_data)
{
	TurtleApp *app = user_data;

	/* The fractals were designed for a white background (the old GnomeCanvas
	   default), so we keep one regardless of the desktop theme. */
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_paint (cr);

	turtle_render (cr, app->segments);
}

static void
turtle_set_title (TurtleApp *app, const char *filename)
{
	g_free (app->title);
	app->title = filename ? g_path_get_basename (filename) : NULL;

	if (app->title) {
		char *full = g_strdup_printf ("%s — GNOME Turtle", app->title);
		gtk_window_set_title (GTK_WINDOW (app->window), full);
		g_free (full);
	} else {
		gtk_window_set_title (GTK_WINDOW (app->window), "GNOME Turtle");
	}
}

static gboolean
turtle_load_file (TurtleApp *app, const char *path, GError **error)
{
	yyin = fopen (path, "r");
	if (yyin == NULL) {
		int err = errno;
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (err),
		             "%s: %s", path, g_strerror (err));
		return FALSE;
	}

	/* The L-System files use '.' as the decimal separator, so the parser's
	   strtod() must run under the C numeric locale. We force it here rather
	   than once at startup because GTK resets LC_ALL during initialisation,
	   which would otherwise clobber a locale set in main(). */
	setlocale (LC_NUMERIC, "C");

	turtle_init_parser ();
	yyparse ();
	fclose (yyin);
	yyin = NULL;

	g_clear_pointer (&app->segments, g_array_unref);
	app->segments = turtle_generate ();

	turtle_set_title (app, path);
	gtk_widget_queue_draw (app->drawing_area);
	return TRUE;
}

static void
turtle_report_error (TurtleApp *app, const char *message)
{
	AdwDialog *dialog = adw_alert_dialog_new (_("Could not open file"), message);

	adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "ok", _("_OK"));
	adw_dialog_present (dialog, GTK_WIDGET (app->window));
}

/* ------------------------------ actions --------------------------- */

static void
open_response_cb (GObject *source, GAsyncResult *result, gpointer user_data)
{
	TurtleApp *app = user_data;
	GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
	g_autoptr (GError) error = NULL;
	g_autoptr (GFile) file = gtk_file_dialog_open_finish (dialog, result, &error);

	if (file == NULL) {
		/* User dismissed the dialog: not an error worth reporting. */
		if (error && !g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
			turtle_report_error (app, error->message);
		return;
	}

	{
		g_autofree char *path = g_file_get_path (file);
		g_autoptr (GError) load_error = NULL;

		if (path == NULL)
			turtle_report_error (app, _("This location is not a local file."));
		else if (!turtle_load_file (app, path, &load_error))
			turtle_report_error (app, load_error->message);
	}
}

static void
action_open (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	TurtleApp *app = user_data;
	GtkFileDialog *dialog = gtk_file_dialog_new ();
	GtkFileFilter *filter = gtk_file_filter_new ();
	g_autoptr (GListStore) filters = g_list_store_new (GTK_TYPE_FILE_FILTER);

	gtk_file_filter_set_name (filter, _("L-System files"));
	gtk_file_filter_add_pattern (filter, "t*");
	gtk_file_filter_add_pattern (filter, "*.lsys");
	g_list_store_append (filters, filter);
	gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));

	gtk_file_dialog_set_title (dialog, _("Open L-System"));
	gtk_file_dialog_open (dialog, GTK_WINDOW (app->window), NULL,
	                      open_response_cb, app);
	g_object_unref (dialog);
}

static void
action_clear (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	TurtleApp *app = user_data;

	g_clear_pointer (&app->segments, g_array_unref);
	turtle_set_title (app, NULL);
	gtk_widget_queue_draw (app->drawing_area);
}

static void
print_draw_page_cb (GtkPrintOperation *op,
                    GtkPrintContext *context,
                    int page_nr,
                    gpointer user_data)
{
	TurtleApp *app = user_data;
	cairo_t *cr = gtk_print_context_get_cairo_context (context);

	turtle_render (cr, app->segments);
}

static void
action_print (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	TurtleApp *app = user_data;
	GtkPrintOperation *op;

	if (app->segments == NULL || app->segments->len == 0) {
		turtle_report_error (app, _("There is nothing to print yet. Open an L-System first."));
		return;
	}

	op = gtk_print_operation_new ();
	gtk_print_operation_set_n_pages (op, 1);
	g_signal_connect (op, "draw-page", G_CALLBACK (print_draw_page_cb), app);
	gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
	                         GTK_WINDOW (app->window), NULL);
	g_object_unref (op);
}

static void
action_about (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	TurtleApp *app = user_data;
	const char *developers[] = { "Mathieu Lutfy", NULL };
	AdwDialog *about;

	about = adw_about_dialog_new ();
	adw_about_dialog_set_application_name (ADW_ABOUT_DIALOG (about), _("GNOME Turtle"));
	adw_about_dialog_set_application_icon (ADW_ABOUT_DIALOG (about), "gnome-turtle");
	adw_about_dialog_set_version (ADW_ABOUT_DIALOG (about), VERSION);
	adw_about_dialog_set_developer_name (ADW_ABOUT_DIALOG (about), "Mathieu Lutfy");
	adw_about_dialog_set_developers (ADW_ABOUT_DIALOG (about), developers);
	adw_about_dialog_set_copyright (ADW_ABOUT_DIALOG (about), "© 2004 Mathieu Lutfy");
	adw_about_dialog_set_license_type (ADW_ABOUT_DIALOG (about), GTK_LICENSE_GPL_2_0);
	adw_about_dialog_set_comments (ADW_ABOUT_DIALOG (about),
	                               _("Draws fractals from L-Systems."));
	adw_about_dialog_set_website (ADW_ABOUT_DIALOG (about),
	                              "https://savannah.nongnu.org/projects/gnome-turtle/");

	adw_dialog_present (about, GTK_WIDGET (app->window));
}

static void
action_quit (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GApplication *gapp = user_data;
	g_application_quit (gapp);
}

/* --------------------------- construction ------------------------- */

static const GActionEntry win_actions[] = {
	{ "open",  action_open,  NULL, NULL, NULL },
	{ "clear", action_clear, NULL, NULL, NULL },
	{ "print", action_print, NULL, NULL, NULL },
	{ "about", action_about, NULL, NULL, NULL },
};

static GMenuModel *
build_menu (void)
{
	GMenu *menu = g_menu_new ();
	GMenu *section = g_menu_new ();

	g_menu_append (section, _("_Open…"), "win.open");
	g_menu_append (section, _("_Clear"), "win.clear");
	g_menu_append (section, _("_Print…"), "win.print");
	g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append (section, _("_About GNOME Turtle"), "win.about");
	g_menu_append (section, _("_Quit"), "app.quit");
	g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
	g_object_unref (section);

	return G_MENU_MODEL (menu);
}

static TurtleApp *
turtle_app_get (GtkApplication *gtk_app)
{
	TurtleApp *app = g_object_get_data (G_OBJECT (gtk_app), "turtle-app");

	if (app != NULL)
		return app;

	app = g_new0 (TurtleApp, 1);

	{
		GtkWidget *toolbar = adw_toolbar_view_new ();
		GtkWidget *header = adw_header_bar_new ();
		GtkWidget *open_button = gtk_button_new_with_label (_("Open"));
		GtkWidget *menu_button = gtk_menu_button_new ();
		GtkWidget *scrolled = gtk_scrolled_window_new ();
		g_autoptr (GMenuModel) menu = build_menu ();

		app->window = ADW_APPLICATION_WINDOW (adw_application_window_new (GTK_APPLICATION (gtk_app)));
		gtk_window_set_default_size (GTK_WINDOW (app->window), CANVAS_SIZE + 40, CANVAS_SIZE + 80);
		gtk_window_set_title (GTK_WINDOW (app->window), "GNOME Turtle");

		g_action_map_add_action_entries (G_ACTION_MAP (app->window), win_actions,
		                                 G_N_ELEMENTS (win_actions), app);

		gtk_actionable_set_action_name (GTK_ACTIONABLE (open_button), "win.open");
		adw_header_bar_pack_start (ADW_HEADER_BAR (header), open_button);

		gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (menu_button), "open-menu-symbolic");
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (menu_button), menu);
		adw_header_bar_pack_end (ADW_HEADER_BAR (header), menu_button);

		app->drawing_area = gtk_drawing_area_new ();
		gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (app->drawing_area), CANVAS_SIZE);
		gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (app->drawing_area), CANVAS_SIZE);
		gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (app->drawing_area),
		                                turtle_draw_cb, app, NULL);

		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), app->drawing_area);
		gtk_widget_set_vexpand (scrolled, TRUE);

		adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (toolbar), header);
		adw_toolbar_view_set_content (ADW_TOOLBAR_VIEW (toolbar), scrolled);
		adw_application_window_set_content (app->window, toolbar);
	}

	g_object_set_data_full (G_OBJECT (gtk_app), "turtle-app", app, g_free);
	return app;
}

static void
app_activate (GApplication *gapp, gpointer user_data)
{
	TurtleApp *app = turtle_app_get (GTK_APPLICATION (gapp));
	gtk_window_present (GTK_WINDOW (app->window));
}

static void
app_open (GApplication *gapp, GFile **files, int n_files, const char *hint, gpointer user_data)
{
	TurtleApp *app = turtle_app_get (GTK_APPLICATION (gapp));
	g_autofree char *path = g_file_get_path (files[0]);
	g_autoptr (GError) error = NULL;

	if (path && !turtle_load_file (app, path, &error))
		g_warning ("%s", error->message);

	gtk_window_present (GTK_WINDOW (app->window));
}

int
main (int argc, char **argv)
{
	g_autoptr (AdwApplication) app = NULL;
	const GActionEntry app_actions[] = {
		{ "quit", action_quit, NULL, NULL, NULL },
	};
	int status;

	/* NB: the C numeric locale needed by the parser (strtod) is forced in
	   turtle_load_file(), after GTK has initialised, not here. */

	app = adw_application_new ("ca.bidon.GnomeTurtle", G_APPLICATION_HANDLES_OPEN);
	g_action_map_add_action_entries (G_ACTION_MAP (app), app_actions,
	                                 G_N_ELEMENTS (app_actions), app);
	gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit",
	                                       (const char *[]){ "<Ctrl>q", NULL });
	gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.open",
	                                       (const char *[]){ "<Ctrl>o", NULL });

	g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
	g_signal_connect (app, "open", G_CALLBACK (app_open), NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);
	return status;
}
