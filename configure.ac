dnl Process this file with autoconf to produce configure.

AC_INIT(pidgin-otr, 4.0.2)

AC_CONFIG_MACRO_DIRS([m4])

AM_CONFIG_HEADER(config.h)

# Silent compilation so warnings can be spotted.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE([-Wall -Werror])
AM_SILENT_RULES([yes])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl Cause "make dist" to create a reproducible tarball (coupled with
dnl setting GZIP_ENV in Makefile.am).  Note that this relies on the
dnl file source_date_epoch existing, which is created by a rule in
dnl Makefile.am.  That file is included in the tarball (so that we
dnl have a timestamp on which to base reproducible binary builds *from*
dnl the tarball), so it will be created before this command is run.
AC_SUBST([am__tar],['( export SOURCE_DATE_EPOCH=`tr -cd 0-9 < source_date_epoch`; chmod -R u=rwX,go=rX "$$tardir"; find "$$tardir" -exec touch -d @$${SOURCE_DATE_EPOCH} {} \; -print0 | LC_ALL=C sort -z | $${TAR-tar} --owner=root --group=root --numeric-owner --no-recursion --null -T - -chf - )'])

dnl We do not want to create a .a for the plugin, so disable by default.
LT_INIT([disable-static])

AM_PATH_LIBGCRYPT(1:1.2.0,,AC_MSG_ERROR(libgcrypt 1.2.0 or newer is required.))

AM_PATH_LIBOTR(4.0.0,,AC_MSG_ERROR(libotr 4.x >= 4.0.0 is required.))

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.6])

AC_ARG_WITH([purple],
    AS_HELP_STRING([--with-purple=<version>],
        [compile with libpurple 2 or 3 (default: auto)]),
    with_purple=$withval, with_purple="auto")

if test "x$with_purple" = "xauto"; then
    PKG_CHECK_MODULES([PURPLE], [purple-3 >= 3.0], [with_purple=3], [
        PKG_CHECK_MODULES([PURPLE], [pidgin >= 2.0 purple >= 2.0])
        with_purple=2
    ])
elif test "x$with_purple" = "x2"; then
    PKG_CHECK_MODULES([PURPLE], [pidgin >= 2.0 purple >= 2.0])
elif test "x$with_purple" = "x3"; then
    PKG_CHECK_MODULES([PURPLE], [purple-3 >= 3.0])
else
    AC_MSG_ERROR([--with-purple must specify one of 2, 3 or auto.])
fi

if test "x$with_purple" = "x3"; then
    dnl TODO remove it when libpurple3 API becomes stable
    AC_MSG_WARN([libpurple 3.0.0 support is experimental at the moment, please don't expect compatibility])
fi
if test "x$with_purple" = "x2"; then
    PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.6])
fi

AM_CONDITIONAL(FOR_LIBPURPLE3, test "x$with_purple" = "x3")
AM_CONDITIONAL(FOR_PIDGIN2, test "x$with_purple" = "x2")

AC_PROG_INTLTOOL

GETTEXT_PACKAGE=pidgin-otr
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define the gettext package to be used])

ALL_LINGUAS="ar cs da de el es fa fi fr hu it my_MM nb_NO nl nn pl pt_BR ru sk sv vi zh_CN"
AM_GLIB_GNU_GETTEXT

dnl 1:flags
dnl Taken from Tor's autoconf magic repository
AC_DEFUN([OTR_CHECK_CFLAGS], [
  AS_VAR_PUSHDEF([VAR],[otr_cv_cflags_$1])
  AC_CACHE_CHECK([whether the compiler accepts $1], VAR, [
    otr_saved_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -pedantic -Werror $1"
    AC_TRY_COMPILE([], [return 0;],
                   [AS_VAR_SET(VAR,yes)],
                   [AS_VAR_SET(VAR,no)])
    CFLAGS="$otr_saved_CFLAGS"
  ])
  if test x$VAR = xyes; then
    CFLAGS="$CFLAGS $1"
  fi
  AS_VAR_POPDEF([VAR])
])

dnl 1:flags
dnl 2:extra ldflags
dnl 3:extra libraries
AC_DEFUN([OTR_CHECK_LDFLAGS], [
  AS_VAR_PUSHDEF([VAR],[otr_cv_ldflags_$1])
  AC_CACHE_CHECK([whether the linker accepts $1], VAR, [
    otr_saved_CFLAGS="$CFLAGS"
    otr_saved_LDFLAGS="$LDFLAGS"
    otr_saved_LIBS="$LIBS"
    CFLAGS="$CFLAGS -pedantic -Werror"
    LDFLAGS="$LDFLAGS $2 $1"
    LIBS="$LIBS $3"
    AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [fputs("", stdout)])],
                  [AS_VAR_SET(VAR,yes)],
                  [AS_VAR_SET(VAR,no)],
           [AC_TRY_LINK([], [return 0;],
                                   [AS_VAR_SET(VAR,yes)],
                                   [AS_VAR_SET(VAR,no)])])
    CFLAGS="$otr_saved_CFLAGS"
    LDFLAGS="$otr_saved_LDFLAGS"
    LIBS="$otr_saved_LIBS"
  ])
  if test x$VAR = xyes; then
    LDFLAGS="$LDFLAGS $1"
  fi
  AS_VAR_POPDEF([VAR])
])


dnl If _WIN32 is defined and non-zero, we are building for win32
AC_MSG_CHECKING([for win32])
AC_RUN_IFELSE([AC_LANG_SOURCE([
int main(int c, char **v) {
#ifdef _WIN32
#if _WIN32
  return 0;
#else
  return 1;
#endif
#else
  return 2;
#endif
}])],
bwin32=true; AC_MSG_RESULT([yes]),
bwin32=false; AC_MSG_RESULT([no]),
bwin32=cross; AC_MSG_RESULT([cross])
)

if test "$bwin32" = cross; then
AC_MSG_CHECKING([for win32 (cross)])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifdef _WIN32
int main(int c, char **v) {return 0;}
#else
#error
int main(int c, char **v) {return x(y);}
#endif
])],
bwin32=true; AC_MSG_RESULT([yes]),
bwin32=false; AC_MSG_RESULT([no]))
fi

AM_CONDITIONAL(BUILD_NT_SERVICES, test x$bwin32 = xtrue)

dnl Adam Shostack suggests the following for Windows:
dnl -D_FORTIFY_SOURCE=2 -fstack-protector-all
dnl Others suggest '/gs /safeseh /nxcompat /dynamicbase' for non-gcc on Windows
dnl This requires that we use gcc and that we add -O2 to the CFLAGS.
AC_ARG_ENABLE(gcc-hardening,
    AS_HELP_STRING(--disable-gcc-hardening, disable compiler security checks))

dnl Linker hardening options
dnl Currently these options are ELF specific - you can't use this with MacOSX
AC_ARG_ENABLE(linker-hardening,
    AS_HELP_STRING(--disable-linker-hardening, disable linker security fixups))

dnl ---------------------------------------------------------------------
dnl Now that we know about our major libraries, we can check for compiler
dnl and linker hardening options.  We need to do this with the libraries known,
dnl since sometimes the linker will like an option but not be willing to
dnl use it with a build of a library.

all_ldflags_for_check="$LDFLAGS"
all_libs_for_check="$LIBGCRYPT_LIBS"

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [
#if !defined(__clang__)
#error
#endif
])], have_clang=yes, have_clang=no)

enable_clang_address_sanitizer=no

if test "x$have_clang" = "xyes" -a "x$enable_clang_address_sanitizer" = "xyes"; then
    dnl Enable address sanitizer.
    OTR_CHECK_CFLAGS(-faddress-sanitizer)
    OTR_CHECK_CFLAGS(-g)
    OTR_CHECK_CFLAGS(-fno-omit-frame-pointer)
    OTR_CHECK_CFLAGS(-fno-inline)
    OTR_CHECK_CFLAGS(-fno-optimize-sibling-calls)
fi

if test x$enable_gcc_hardening != xno; then
    CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2"
    if test x$have_clang = xyes; then
       OTR_CHECK_CFLAGS(-Qunused-arguments)
    fi
    OTR_CHECK_CFLAGS(-fstack-protector-all)
    OTR_CHECK_CFLAGS(-Wstack-protector)
    OTR_CHECK_CFLAGS(-fwrapv)
    OTR_CHECK_CFLAGS(--param ssp-buffer-size=1)

    dnl Ian added the next four:
    OTR_CHECK_CFLAGS(-fno-strict-overflow)
    OTR_CHECK_CFLAGS(-Wall)
    OTR_CHECK_CFLAGS(-Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
    OTR_CHECK_CFLAGS(-Wformat-security)

    if test "$bwin32" = "false"; then
       OTR_CHECK_CFLAGS(-fPIE)
       OTR_CHECK_LDFLAGS(-pie, "$all_ldflags_for_check", "$all_libs_for_check")
    fi
fi

if test x$enable_linker_hardening != xno; then
    OTR_CHECK_LDFLAGS(-z relro -z now, "$all_ldflags_for_check", "$all_libs_for_check")
fi

AC_OUTPUT([Makefile po/Makefile.in])

dnl vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
