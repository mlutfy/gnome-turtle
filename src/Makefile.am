## Process this file with automake to produce Makefile.in
bin_PROGRAMS = gnome-turtle

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(srcdir)						\
	$(GNOMETURTLE_CFLAGS)					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGNOMETURTLE_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
	
#	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	
#	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		
#	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	
#	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"

YFLAGS = -d 

gnome_turtle_LDADD = $(INTLLIBS) @LEXLIB@ @LIBS@ $(GNOMETURTLE_LIBS) 
# $(GNOMEUI_LIBS)
# $(GNOME_LIBDIR) 

NON_INST_H_FILS =			\
	gnome-turtle.h			\
	turtle-helpers.h
 
gnome_turtle_SOURCES = 			\
	turtle-parser.y			\
	turtle-scanner.l		\
	gnome-turtle.c			\
	turtle-helpers.c		\
	$(NON_INST_H_FILS)

BUILT_SRC =				\
	turtle-parser.c			\
	turtle-parser.h			\
	turtle-scanner.c		\
	turtle-scanner.h
	
CLEANFILES = $(BUILT_SRC)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SRC)

# install-exec-local:
# 	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
# 	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor


