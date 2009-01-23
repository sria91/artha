uidir = $(datadir)/$(PACKAGE_TARNAME)
ui_DATA = gui.ui

icondir = $(datadir)/@ICON_SUB_PATH@
icon_DATA = artha.svg

desktopdir = $(datadir)/applications
desktop_DATA = artha.desktop

man_MANS = artha.1

EXTRA_DIST = artha.1 gui.glade $(desktop_DATA) $(icon_DATA)

gui.ui: gui.glade
	$(GTKBUILDERCONV) $< $@

uninstall-local:
	-rm -rf $(datadir)/$(PACKAGE_TARNAME)

CLEANFILES = $(ui_DATA)

