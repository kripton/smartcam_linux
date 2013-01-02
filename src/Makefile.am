AM_CPPFLAGS = -DPACKAGE_DATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\"

bin_PROGRAMS = smartcam

smartcam_SOURCES = \
    smartcam.cpp SmartEngine.cpp SmartEngine.h \
    CommHandler.cpp CommHandler.h \
    UIHandler.cpp UIHandler.h \
    UserSettings.cpp UserSettings.h \
    JpegHandler.cpp JpegHandler.h

smartcam_CXXFLAGS = @GTK_CFLAGS@ @GTHREAD_CFLAGS@ @DBUS_CFLAGS@ @GCONF_CFLAGS@

smartcam_LDADD = @GTK_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @GCONF_LIBS@ -lbluetooth -ljpeg

#dbus
BUILT_SOURCES = smartcam-dbus.h
# We don't want to install this header
noinst_HEADERS = $(BUILT_SOURCES)

# Correctly clean the generated headers, but keep the xml description
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = smartcam-dbus.xml

#Rule to generate the binding headers
smartcam-dbus.h:  smartcam-dbus.xml
	dbus-binding-tool --prefix=smartcam_dbus --mode=glib-server $<> $@

clean-local :
	rm -f *~
