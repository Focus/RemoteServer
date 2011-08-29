#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([RemoteServer], [0.1], [batieatworld@gmail.com])
AC_CONFIG_SRCDIR([server.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(Xtst,XOpenDisplay,[], AC_MSG_ERROR([Unable to find Xtest libraries.]) )
AC_DEFINE([USE_XTEST], [1], [Using Xtest extension])


# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([bzero inet_ntoa memset socket])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
