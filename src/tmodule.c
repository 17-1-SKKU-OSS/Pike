/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* This variant of module.c is used when modules are dynamic and when
 * building tpike (which only is done when modules are static). It
 * doesn't link in any post-modules. */
#define PRE_PIKE
#include "module.c"
