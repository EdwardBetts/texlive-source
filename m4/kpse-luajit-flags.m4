# Public macros for the TeX Live (TL) tree.
# Copyright (C) 2014 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# KPSE_LUAJIT_FLAGS
# -----------------
# Set the make variables LUAJIT_INCLUDES and LUAJIT_LIBS to
# the CPPFLAGS and LIBS required for the `-lluajit' library in
# libs/luajit/ of the TL tree.
AC_DEFUN([KPSE_LUAJIT_FLAGS], [dnl
_KPSE_LIB_FLAGS([luajit], [luajit], [tree],
                [-IBLD/libs/luajit/include], [BLD/libs/luajit/libluajit.a], [],
                [], [${top_builddir}/../../libs/luajit/include/luajit.h])[]dnl
]) # KPSE_LUAJIT_FLAGS

# KPSE_LUAJIT_DEFINES
# -------------------
# Set the make variable LUAJIT_DEFINES to the CPPFLAGS required when
# compiling or using the `-lluajit' library.
AC_DEFUN([KPSE_LUAJIT_DEFINES], [dnl
AC_SUBST([LUAJIT_DEFINES], [-DLUAJIT_ENABLE_LUA52COMPAT])
]) # KPSE_LUAJIT_DEFINES
