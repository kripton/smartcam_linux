SUBDIRS = icons phone_installs

desktopdir = $(datadir)/applications
desktop_in_files = smartcam.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

clean-local :
	rm -f *~

DISTCLEANFILES = smartcam.desktop
