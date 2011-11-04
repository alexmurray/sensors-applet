SUBDIRS = sensors-applet lib plugins pixmaps po help

appletdir = $(LIBPANEL_APPLET_DIR)
applet_in_files = org.gnome.applets.SensorsApplet.panel-applet.in
applet_DATA = $(applet_in_files:.panel-applet.in=.panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
	-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
	-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
	$< > $@

@INTLTOOL_SERVER_RULE@

uidir 		= $(pkgdatadir)/ui
ui_DATA 	= SensorsApplet.xml

%.panel-applet: %.panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.panel.applet.SensorsAppletFactory.service.in
service_DATA = $(service_in_files:.service.in=.service)

org.gnome.panel.applet.SensorsAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
	-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
	$< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(schemas_DATA)

EXTRA_DIST 	= $(ui_DATA) \
		  $(applet_in_files:.panel-applet.in=.panel-applet.in.in) \
		  $(service_in_files) \
		  intltool-merge.in intltool-extract.in intltool-update.in \
		  gnome-doc-utils.make

DISTCLEANFILES	=	gnome-doc-utils.make \
		   	intltool-extract \
		   	intltool-merge \
	   	   	intltool-update

DISTCHECK_CONFIGURE_FLAGS	= --disable-scrollkeeper
