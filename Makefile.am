AM_CFLAGS = \
	@LIBGCRYPT_CFLAGS@ @LIBOTR_CFLAGS@ \
	@GLIB_CFLAGS@ @PURPLE_CFLAGS@ \
	-DPURPLE_PLUGINS \
	-DPIDGIN_OTR_VERSION=\"@VERSION@\" \
	-DLOCALEDIR=\"$(datadir)/locale\"

SUBDIRS = po

if FOR_LIBPURPLE3
plugindir = ${libdir}/purple-3
else
# FOR_PIDGIN2
plugindir = ${libdir}/pidgin
endif

plugin_LTLIBRARIES = pidgin-otr.la

pidgin_otr_la_SOURCES = \
	common.c \
	dialogs.c \
	otr-plugin.c \
	ui.c

pidgin_otr_la_LDFLAGS = \
	-module -avoid-version \
	@LIBGCRYPT_LIBS@ @LIBOTR_LIBS@ @PURPLE_LIBS@

EXTRA_DIST = \
	INSTALL.mingw \
	Makefile.mingw \
	Makefile.static \
	common.h \
	dialogs.h \
	fprint-verificator.h \
	gtk-dialog.h \
	otr-icons.h \
	otr-plugin.h \
	purple3-dialog.h \
	purple3-ui.h \
	gtk-ui.h \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	packaging/fedora/pidgin-otr.spec \
	packaging/windows/pidgin-otr.nsi \
	tooltipmenu.h \
	ui.h \
	po/Makefile.mingw \
	po/README

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

if FOR_PIDGIN2
pidgin_otr_la_SOURCES += \
	gtk-dialog.c \
	gtk-ui.c \
	tooltipmenu.c
AM_CFLAGS += \
	-DUSING_GTK \
	@GTK_CFLAGS@
else
EXTRA_DIST += \
	gtk-dialog.c \
	gtk-ui.c \
	tooltipmenu.c
endif

if FOR_LIBPURPLE3
pidgin_otr_la_SOURCES += \
	purple3-dialog.c \
	purple3-ui.c
else
EXTRA_DIST += \
	purple3-dialog.c \
	purple3-ui.c
endif
