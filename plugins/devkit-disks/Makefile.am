# DEVICEKIT-DISKS plugin
plugindir = $(libdir)/sensors-applet/plugins

INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""Sensors Applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(DEVICEKIT_CFLAGS)


plugin_LTLIBRARIES = libdevkitdisks.la

libdevkitdisks_la_SOURCES = \
	devkit-plugin.h	\
	devkit-plugin.c

libdevkitdisks_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) $(DEVICEKIT_LIBS)
libdevkitdisks_la_LIBADD = $(top_builddir)/lib/libsensors-applet-plugin.la
