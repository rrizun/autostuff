AC_INIT(auto, 0.1)
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_RANLIB
#AC_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile])

AC_CHECK_LIB([dl], [main])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([rt], [main])
AC_CHECK_LIB([mysql], [main])

PKG_CHECK_MODULES([LIBCURL], [libcurl])
PKG_CHECK_MODULES([LIBXML2], [libxml-2.0])
PKG_CHECK_MODULES([PROTOBUF], [protobuf])

# nested packages
AC_CONFIG_SUBDIRS([libevent-2.0.16-stable]) # configure.in:3: error: Autoconf version 2.59c or higher is required

#AM_SILENT_RULES([yes]) # does not work on rhel54

AC_OUTPUT
