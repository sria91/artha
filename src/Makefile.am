AM_CFLAGS = -Wall -Wextra -ansi -pedantic -std=c99 -Wno-unused-parameter \
-fstrict-aliasing -Wno-missing-field-initializers @GTK_CFLAGS@ @libdbus_CFLAGS@

bin_PROGRAMS = artha

noinst_LIBRARIES = libwni.a
libwni_a_SOURCES = wni.c wni.h

artha_DEPENDENCIES = libwni.a
artha_SOURCES = eggaccelerators.c eggaccelerators.h \
		mod_notify.c mod_notify.h \
		suggestions.c suggestions.h \
		hotkey_editor.c hotkey_editor.h \
		gui.c gui.h \
		tomboyutil.c tomboyutil.h \
		instance_handler.c instance_handler.h \
		addons.h


# @GTK_LIBS@ & libdbus_CFLAGS are not used to avoid unwanted 
# libraries (like -latk, -lrt, etc.) passed to the linker
artha_LDADD = libwni.a $(WORDNET_LIB) -lX11 -ldbus-1 \
		-ldbus-glib-1 -lgtk-x11-2.0 -lgdk-x11-2.0 \
		-lgmodule-2.0 -lgobject-2.0 -lglib-2.0

