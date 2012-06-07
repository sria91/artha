dist_ui_DATA = gui.glade
dist_icon_DATA = artha.png
dist_man_MANS = artha.1

EXTRA_DIST = artha.desktop artha.ico

if POSIX
desktop_DATA = artha.desktop
desktopdir = $(datadir)/applications
uidir = $(datadir)/$(PACKAGE_TARNAME)
icondir = $(datadir)/@ICON_SUB_PATH@
else
icondir = $(bindir)
uidir = $(bindir)
endif

