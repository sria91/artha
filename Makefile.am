ACLOCAL_AMFLAGS = -I m4
SUBDIRS = data src

libtool: @LIBTOOL_DEPS@
	cd $(srcdir) && $(SHELL) ./config.status --recheck
