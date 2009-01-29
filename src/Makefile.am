AM_CFLAGS = -Wall @GTK_CFLAGS@ @dbus_CFLAGS@
AM_LDFLAGS = -lgtk-x11-2.0 -lgthread-2.0 @dbus_LIBS@ $(WORDNETLIB) $(NOTIFYLIB)


bin_PROGRAMS = artha

noinst_LIBRARIES = libwni.a
libwni_a_SOURCES = wni.c wni.h

artha_DEPENDENCIES = libwni.a
artha_SOURCES = wni.h gui.c gui.h
artha_LDADD = libwni.a

