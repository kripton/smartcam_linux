AC_INIT([SmartCam], [1.4.0], [deionut@yahoo.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_SRCDIR([src/smartcam.cpp])
AC_CONFIG_HEADERS([config.h])

#*******************************************************************************
# Internationalization
#*******************************************************************************
IT_PROG_INTLTOOL([0.40.0])

# Check for dependencies...
AC_PROG_CXX

PKG_CHECK_MODULES(GLIB, glib-2.0, dummy="yes", AC_MSG_ERROR(Cannot find glib-2.0 or later, please install it and rerun ./configure.))

PKG_CHECK_MODULES(GTHREAD, [gthread-2.0], dummy="yes", AC_MSG_ERROR(Cannot find gthread-2.0 or later, please install it and rerun ./configure.))
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)

PKG_CHECK_MODULES(GTK, gdk-2.0 gtk+-2.0, dummy="yes", AC_MSG_ERROR(Cannot find gtk+-2.0/gdk-2.0 or later, please install it and rerun ./configure.))
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(DBUS, dbus-1 dbus-glib-1, dummy="yes", AC_MSG_ERROR(Cannot find dbus-1/dbus-glib-1, please install it and rerun ./configure.))
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0, dummy="yes", AC_MSG_ERROR(Cannot find gconf-2.0, please install it and rerun ./configure.))
AC_SUBST(GCONF_LIBS)
AC_SUBST(GCONF_CFLAGS)

AC_CHECK_LIB(bluetooth, hci_open_dev, dummy="yes", AC_MSG_ERROR(Bluetooth library not found))

AC_CHECK_HEADER(jpeglib.h,
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,	dummy="yes", AC_MSG_ERROR(IJG JPEG codec library not found))
	JPEG_LIBS="-ljpeg")

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/smartcam.desktop.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/phone_installs/Makefile
po/Makefile.in
])
AC_OUTPUT
