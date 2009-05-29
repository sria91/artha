AM_CFLAGS = -Wall @GTK_CFLAGS@ @libnotify_CFLAGS@

# AM_LDFLAGS is only to pass global linker flags and not for 
# passing libraries; hence per-build _LDADD is used below. 
#AM_LDFLAGS = $(WORDNET_LIB) -lgtk-x11-2.0 -lgthread-2.0 -lgmodule-2.0 -lnotify

bin_PROGRAMS = artha

noinst_LIBRARIES = libwni.a
libwni_a_SOURCES = wni.c wni.h

artha_DEPENDENCIES = libwni.a
artha_SOURCES = wni.h addons.h gui.c gui.h suggestions.c suggestions.h
artha_LDADD = libwni.a $(WORDNET_LIB) -lgtk-x11-2.0 -lgthread-2.0 -lgmodule-2.0 @libnotify_LIBS@

