/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dummy.c,v 1.12 2003/06/23 18:10:49 kiwi Exp $
*/

/*
 * Glue needed on Solaris if libgcc.a isn't compiled with -fpic.
 *
 * Henrik Grubbström 1997-03-06
 */

#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"

#ifdef HAVE_MYSQL

typedef INT64 _ll_t;
typedef unsigned INT64 _ull_t;

_ll_t mysql_dummy_dum_dum(_ull_t a, _ull_t b, _ll_t c, _ll_t d) {
#ifdef HAVE_LDIV
  ldiv(10, 3);
#endif
#ifdef HAVE_OPEN
  open(0, 0);
#endif
#ifdef HAVE_SOPEN
  sopen(0, 0, 0);
#endif
#ifdef HAVE_CLOSE
  close(0);
#endif
#ifdef HAVE_READ
  read(0, 0, 0);
#endif
#ifdef HAVE_FILENO
  fileno(0);
#endif
#ifdef HAVE_PUTS
  puts(0);
#endif
#ifdef HAVE_FGETS
  fgets(0, 0, 0);
#endif

  return(a%b+(c%d)+(c/d)+(a/b));
}

#else
static int place_holder;	/* Keep the compiler happy */
#endif /* HAVE_MYSQL */
