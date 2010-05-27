ui_DATA = gui.glade
icon_DATA = artha.png

EXTRA_DIST = $(ui_DATA) $(icon_DATA)

if POSIX
uidir = $(datadir)/$(PACKAGE_TARNAME)
icondir = $(datadir)/@ICON_SUB_PATH@
desktopdir = $(datadir)/applications
desktop_DATA = artha.desktop
man_MANS = artha.1
EXTRA_DIST += artha.1 $(desktop_DATA)
else
icondir = $(bindir)
uidir = $(bindir)
endif
