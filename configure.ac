#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])

AC_INIT([Artha], [1.0.0], [legends2k@yahoo.com], [artha])
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
AC_CHECK_HEADER([wn.h], , [
AC_MSG_ERROR([
You must have wordnet development headers (wordnet-dev) to build.
(http://wordnet.princeton.edu/obtain).
Refer INSTALL file for further details.])
])

# check for wordnet (libwordnet.a)
# if not found WN (building WordNet from source gives libWN.a)
# if not found wn (in some distros it's all small case)
# then check other possible combinations; if none was found exit
AC_CHECK_LIB([wordnet], [wninit], [WORDNET_LIB=-lwordnet], [
	AC_CHECK_LIB([WN], [wninit], [WORDNET_LIB=-lWN], [
		AC_CHECK_LIB([wn], [wninit], [WORDNET_LIB=-lwn], [
			AC_CHECK_LIB([WordNet], [wninit], [WORDNET_LIB=-lWordNet], [
				AC_CHECK_LIB([WORDNET], [wninit], [WORDNET_LIB=-lWORDNET], [
						echo "You must have WordNet library installed to build.";
						echo "(http://wordnet.princeton.edu/obtain)";
						exit 1
					])
				])
			])
		])
	])
AC_SUBST([WORDNET_LIB])


# gtk_builder_add_from_file() since 2.12 only, hence check for GTK+-2.0 >= 2.12
# g_regex_new() since 2.14 only, hence check for GLib-2.0 >= 2.14
# check for GTK+ and GLib
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.12 glib-2.0 >= 2.14], , [
AC_MSG_RESULT(no)
AC_MSG_ERROR([
You must have GTK+ 2.0 (version 2.12 or above) and GLib 2.0 (version 2.14 
or above) development headers installed to build.

If you have these installed already you may need to install pkg-config so 
that they can be located.
])])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

# check if dbus-1 alone is enough or dbus-glib-1 alone is enough
PKG_CHECK_MODULES([libdbus], [dbus-1 >= 0.60 dbus-glib-1 >= 0.60], , [
AC_MSG_RESULT(no)
AC_MSG_ERROR([
You must have the dbus and dbus-glib-1 (version 0.60 or above) 
development headers installed!

If it is installed already, you may need to install pkg-config so 
that they can be located.
])])
AC_SUBST([libdbus_CFLAGS])
AC_SUBST([libdbus_LIBS])


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


AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h], , [
AC_MSG_ERROR([
Xlib Development headers not found!
You need X11 development headers to build.])
])

AC_CHECK_FUNC([strcmp], , [
AC_MSG_ERROR([
Failed to find strcmp()!
The standard C library function strcmp is required to build.
])
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

    Refer INSTALL file for further details.
])

