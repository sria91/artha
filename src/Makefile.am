AM_CFLAGS = -Wall @GTK_CFLAGS@ @libnotify_CFLAGS@
AM_LDFLAGS = -lgtk-x11-2.0 -lgthread-2.0 -lgmodule-2.0 $(WORDNET_LIB) -lnotify

bin_PROGRAMS = artha

noinst_LIBRARIES = libwni.a
libwni_a_SOURCES = wni.c wni.h

artha_DEPENDENCIES = libwni.a
artha_SOURCES = wni.h addons.h gui.c gui.h suggestions.c suggestions.h
artha_LDADD = libwni.a

