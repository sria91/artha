#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])

m4_define([MAJOR_VER], 1)
m4_define([MINOR_VER], 0)
m4_define([BUILD_VER], 2)

AC_INIT([Artha],[MAJOR_VER.MINOR_VER.BUILD_VER],[legends2k@yahoo.com],[artha],[http://artha.sourceforge.net/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD

AM_INIT_AUTOMAKE([-Wall -Werror check-news dist-bzip2])

# Checks for programs.
AM_PROG_CC_C_O
AC_LANG([C])
AC_LANG_CONFTEST(
[AC_LANG_SOURCE([[const char hw[] = "Hello, World\n";]])])

# define the usage of pkg-config
PKG_PROG_PKG_CONFIG


# Checks for libraries.

# Check for wordnet-dev and wordnet (>=3.0)
AC_CHECK_HEADER([wn.h], , [
AC_MSG_ERROR([
You must have wordnet development headers (wordnet-dev) to build.
(http://wordnet.princeton.edu/download).
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
						echo "(http://wordnet.princeton.edu/download)";
						exit 1
					])
				])
			])
		])
	])
AC_SUBST([WORDNET_LIB])


# gtk_widget_get_visible() since 2.18 only, hence check for GTK+-2.0 >= 2.18
# GTK+ 2.18 depends on GLib 2.22, hence check for GLib-2.0 >= 2.22
# check for GTK+ and GLib
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.18 glib-2.0 >= 2.22], , [
AC_MSG_RESULT(no)
AC_MSG_ERROR([
You must have GTK+ 2.0 (version 2.18 or above) and GLib 2.0 (version 2.22 
or above) development headers installed to build.

If you have these installed already you may need to install pkg-config so 
that they can be located.
])])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

LT_PREREQ([2.2])
LT_INIT
case $build_os in
	*cygwin* | *mingw*)
		build_mach=[windows]
		AC_DEFINE([WINVER],[0x0501],[Build machine is Windows XP])
		AC_DEFINE([_WIN32_WINDOWS],[WINVER],[Build machine is Windows XP])
        AC_DEFINE([_WIN32_WINNT],[_WIN32_WINDOWS],[Build machine is Windows XP])
		AC_DEFINE_UNQUOTED([APP_VERSION],[MAJOR_VER, MINOR_VER, BUILD_VER, 0],[app. executable version])
		m4_define([DLL_MAJOR_VER],1)
		m4_define([DLL_MINOR_VER],0)
		m4_define([DLL_BUILD_VER],0)
		AC_DEFINE_UNQUOTED([DLL_VERSION],[DLL_MAJOR_VER, DLL_MINOR_VER, DLL_BUILD_VER, 0],[libnotify dll version as DWORD])
		AC_DEFINE([DLL_VERSION_STR],["DLL_MAJOR_VER.DLL_MINOR_VER.DLL_BUILD_VER.0"],[libnotify dll version string])
		dll_ver=DLL_MAJOR_VER:DLL_MINOR_VER:DLL_BUILD_VER
		AC_SUBST([dll_ver])
		_LT_SET_OPTIONS([LT_INIT],[win32-dll],[disable-static])
		LT_LANG([Windows Resource])
		;;
	*)
		build_mach=[posix]
		;;
esac
AC_SUBST([LIBTOOL_DEPS])

# begin of POSIX specific config
if test "$build_mach" = "posix"; then

# check if dbus-1 alone is enough or dbus-glib-1 alone is enough
PKG_CHECK_MODULES([libdbus], [dbus-1 >= 0.60 dbus-glib-1 >= 0.60], 
[AC_DEFINE([DBUS_AVAILABLE], [], [Dbus dev. headers are present])], [
AC_MSG_RESULT(no)
AC_MSG_ERROR([
You must have the dbus and dbus-glib-1 (version 0.60 or above) 
development headers installed!

If it is installed already, you may need to install pkg-config so 
that they can be located.
])
])

AC_SUBST([libdbus_CFLAGS])
AC_SUBST([libdbus_LIBS])


# Checks for header files.
AC_PATH_X

if test "$no_x" = "yes"; then
	AC_MSG_ERROR([X11 libraries are required to build.])
fi


AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h], [ AC_DEFINE([X11_AVAILABLE], [], [Xlib dev. headers are present]) ], [
AC_MSG_ERROR([
Xlib Development headers not found!
You need X11 development headers to build.])
])

fi
# end of POSIX specific config

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

AM_CONDITIONAL([POSIX],[test "$build_mach" = "posix"])

# get the target directory's path and get it defined in config.h
AM_COND_IF([POSIX],
[
AC_DEFINE_DIR([APP_DIR], [datadir/AC_PACKAGE_TARNAME], [Path where app. files are placed])
# get the target directory's path and get it defined in config.h
icon_dir=pixmaps
AC_SUBST([ICON_SUB_PATH], [$icon_dir])
AC_DEFINE_DIR([ICON_DIR], [datadir/"$icon_dir"], [Path where icon files are placed])
],
[
AC_DEFINE([APP_DIR], ["."], [Path where app. files are placed])
AC_DEFINE([ICON_DIR], ["."], [Path where app. files are placed])
])

AC_CHECK_FUNCS([strcmp strlen memset], , [
AC_MSG_ERROR([
Failed to find function(s) strcmp or strlen!
These standard C library functions are required to build.
])
])


# Enable debugging argument option. Possible values = yes, messages or verbose
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [enable debugging (default = no)]), [ac_debug="$enableval"])
if test "$ac_debug" = "yes"; then
	AC_DEFINE(DEBUG_LEVEL, 1, [Application debugging enabled])
elif test "$ac_debug" = "messages"; then
	AC_DEFINE(DEBUG_LEVEL, 2, [Debugging enabled both in application and WNI])
elif test "$ac_debug" = "verbose"; then
	AC_DEFINE(DEBUG_LEVEL, 3, [Verbose Debugging enabled both in application and WNI])
else
	ac_debug="no"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_C_INLINE
AC_FUNC_MALLOC

# define ssize_t needed by suggestions.h on Windows
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T


AC_CONFIG_SRCDIR([src/wni.c])
AC_CONFIG_HEADERS([config.h])
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

