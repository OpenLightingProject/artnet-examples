#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(artnet-examples, [0.3.13])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lncurses':
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL(HAVE_NCURSES, test "${have_ncurses}" = "yes")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h sys/ioctl.h sys/time.h sys/timeb.h termios.h unistd.h])

# check for libartnet
PKG_CHECK_MODULES(libartnet, [libartnet >= 1.1.0])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday atexit ftime memset select strdup strerror])

AC_MSG_CHECKING([operating system])
case $host in
*-linux*)
  AC_DEFINE(OS_LINUX, [], [Linux backend])
  AC_SUBST(OS_LINUX)
  AC_MSG_RESULT([Linux])
  backend="linux"
  ;;
*-darwin*)
  AC_DEFINE(OS_DARWIN, [], [Darwin backend])
  AC_SUBST(OS_DARWIN)
  AC_MSG_RESULT([Darwin/MacOS X])
  backend="darwin"
  ;;
*-mingw*)
  AC_DEFINE(OS_WINDOWS, [], [Windows backend])
  AC_SUBST(OS_WINDOWS)
  AC_MSG_RESULT([Windows])
  backend="windows"
  ;;
*-cygwin*)
  AC_DEFINE(OS_WINDOWS, [], [Windows backend])
  AC_SUBST(OS_WINDOWS)
  AC_MSG_RESULT([Windows])
  backend="windows"
  ;;
*)
  AC_MSG_ERROR([unsupported operating system])
esac

AM_CONDITIONAL([OS_LINUX], [test "x$backend" = "xlinux"])
AM_CONDITIONAL([OS_DARWIN], [test "x$backend" = "xdarwin"])
AM_CONDITIONAL([OS_WINDOWS], [test "x$backend" = "xwindows"])

AC_OUTPUT(Makefile src/Makefile)
