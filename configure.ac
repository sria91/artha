#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])

AC_INIT([Artha], [0.7.1], [legends2k@yahoo.com], [artha])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror check-news dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_LANG([C])
AC_PROG_RANLIB

# define the usage of pkg-config
PKG_PROG_PKG_CONFIG

# Checks for libraries.

# Check for wordnet-dev and wordnet (>=3.0)
AC_CHECK_HEADERS([wn.h], , [
AC_MSG_ERROR([echo "You must have wordnet development headers to build."; echo "(http://wordnet.princeton.edu/obtain)"])
])
AC_CHECK_LIB([wordnet], [wninit], [WORDNETLIB=-lwordnet], [echo "You must have wordnet development headers to build."; echo "(http://wordnet.princeton.edu/obtain)"; exit 1])
AC_SUBST([WORDNETLIB])


# gtk_builder_add_from_file() since 2.12 only, hence check for GTK+-2.0 >= 2.12
# g_slice_alloc() since 2.10 only, hence check for GLib-2.0 >= 2.10
# check for GTK+ and GLib
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.12 glib-2.0 >= 2.10], , [
AC_MSG_RESULT(no)
AC_MSG_ERROR([
	You must have the GTK+ 2.0 (version 2.12 or above) and GLib 2.0 (version 2.10 or above) development headers installed to build.

	If you have these installed already you may need to install pkg-config so I can find them.
])])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])


# check for gtk-builder-convert command
AC_CHECK_PROGS([GTKBUILDERCONV], [gtk-builder-convert], [no])
if test "$GTKBUILDERCONV" = "no"; then
	AC_MSG_ERROR([This application needs 'gtk-builder-convert' python script! Please install libgtk2.0 (version 2.12 or above) development headers.])
fi


# Check for libnotify, if found, get the details of dbus too
AC_CHECK_LIB([notify], [notify_notification_new_with_status_icon], [NOTIFYLIB=-lnotify; NOTIFY=1], [NOTIFY=0])
if test "$NOTIFY" = 1; then
	# version 0.74 is the one found on a presetine installation of Hardy
	PKG_CHECK_MODULES([dbus], [dbus-glib-1 >= 0.74], , [
	AC_MSG_RESULT(no)
	AC_MSG_WARN([
	You must have the dbus-glib-1 (version 0.74 or above) development headers installed for Notifications feature.

	If you have it installed already you may need to install pkg-config so I can find them.
	])
	dbus_not_found=1
	])
	if test "$dbus_not_found" != 1; then
		AC_SUBST([dbus_CFLAGS])
		#AC_SUBST([dbus_LIBS])

		AC_SUBST([NOTIFYLIB])

		AC_DEFINE([NOTIFIER_SUPPORT], [1], [Notify - Notifications library present])
	fi
else
	AC_MSG_WARN([Notifications feature is disabled since libnotify development headers were not found! If you need Notifications, please install them (libnotify-dev >= 0.4.1) and rebuild.])
fi


AC_DEFUN([AC_DEFINE_DIR], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
dnl In Autoconf 2.60, ${datadir} refers to ${datarootdir}, which in turn
dnl refers to ${prefix}.  Thus we have to use `eval' twice.
  eval ac_define_dir="\"[$]$2\""
  eval ac_define_dir="\"$ac_define_dir\""
  AC_SUBST($1, "$ac_define_dir")
  AC_DEFINE_UNQUOTED($1, "$ac_define_dir", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])


# Commented sine PKG_CHECK_MODULES will take care of checking this
#AC_CHECK_LIB([glib-2.0], [g_key_file_new], , AC_MSG_ERROR([You must have glib-2.0 (version 2.10 or above) development headers installed to build.]), )

#AC_CHECK_LIB([gtk-x11-2.0], [gtk_show_uri], [SHOW_URI=yes], [SHOW_URI=no], )
#if test "$SHOW_URI" = "yes"; then
#	AC_DEFINE([GTK_SHOW_URI], [1], [gtk_show_uri function availability])
#fi

# get the target directory's path and get it defined in config.h
AC_DEFINE_DIR([APP_DIR], [datadir/AC_PACKAGE_TARNAME], [Path where app. files are placed])

# get the target directory's path and get it defined in config.h
icon_dir=pixmaps
AC_SUBST([ICON_SUB_PATH], [$icon_dir])
AC_DEFINE_DIR([ICON_DIR], [datadir/"$icon_dir"], [Path where icon files are placed])

# Checks for header files.
AC_PATH_X

if test "$no_x" = "yes"; then
	AC_MSG_ERROR([X11 libraries are required to build.])
fi

AC_CHECK_HEADERS([X11/Xlib.h X11/keysym.h], , [
AC_MSG_ERROR([X Development headers not found!
You need X11 development headers to build.])
])

# Enable debugging argument option. Possible values = yes, message or verbose
AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug], [enable debugging (default = no)]), [ac_debug="$enableval"])
if test "$ac_debug" = "yes"; then
	AC_DEFINE(DEBUG_LEVEL, 1, [Application debugging enabled])
elif test "$ac_debug" = "messages"; then
	AC_DEFINE(DEBUG_LEVEL, 2, [Debugging enabled both in application and WNI])
elif test "$ac_debug" = "verbose"; then
	AC_DEFINE(DEBUG_LEVEL, 3, [Verbose Debugging enabled both in application and WNI])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL

# Checks for library functions.
#AC_CHECK_FUNCS([wninit])

AC_CONFIG_SRCDIR([src/wni.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile data/Makefile src/Makefile])
AC_OUTPUT

# display message, if all the above was successful
AC_MSG_RESULT([
Artha is now configured!

  Installation directory:	${prefix}

  To build and install Artha:

    make
    make install (root previlages required)

  Once installed, type `artha` to run.

    See INSTALL file for further details.
])

