/*
 * $Id: invert.c,v 1.11 2000/12/01 08:10:28 hubbe Exp $
 *
 * INVERT crypto module for Pike
 *
 * /precompiled/crypto/invert
 *
 * Henrik Grubbström 1996-11-08
 */

/*
 * Includes
 */

/* From the Pike distribution */
#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "svalue.h"
#include "constants.h"
#include "pike_macros.h"
#include "threads.h"
#include "object.h"
#include "stralloc.h"
#include "builtin_functions.h"

/* THIS MUST BE INCLUDED LAST */
#include "module_magic.h"

/*
 * Globals
 */

struct program *pike_crypto_invert_program;

/*
 * Functions
 */

void init_pike_crypto_invert(struct object *o)
{
}

void exit_pike_crypto_invert(struct object *o)
{
}

/*
 * efuns and the like
 */

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->name()\n");
  }
  push_string(make_shared_string("INVERT"));
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->query_block_size()\n");
  }
  push_int(8);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->query_key_length()\n");
  }
  push_int(0);
}

/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    Pike_error("Wrong number of args to invert->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to invert->set_key()\n");
  }
  pop_n_elems(args);
  push_object(this_object());
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  char *buffer;
  ptrdiff_t i;
  ptrdiff_t len;

  if (args != 1) {
    Pike_error("Wrong number of arguments to invert->crypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to invert->crypt_block()\n");
  }
  if (sp[-1].u.string->len % 8) {
    Pike_error("Bad length of argument 1 to invert->crypt_block()\n");
  }

  if (!(buffer = alloca(len = sp[-1].u.string->len))) {
    Pike_error("invert->crypt_block(): Out of memory\n");
  }

  for (i=0; i<len; i++) {
    buffer[i] = ~sp[-1].u.string->str[i];
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string(buffer, len));

  MEMSET(buffer, 0, len);
}

/*
 * Module linkage
 */

void pike_invert_init(void)
{
  /*
   * start_new_program();
   *
   * add_storage();
   *
   * add_function();
   * add_function();
   * ...
   *
   * set_init_callback();
   * set_exit_callback();
   *
   * program = end_c_program();
   * program->refs++;
   *
   */

  /* /precompiled/crypto/invert */
  start_new_program();

  /* function(void:string) */
  ADD_FUNCTION("name", f_name,tFunc(tVoid,tStr), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_block_size", f_query_block_size,tFunc(tVoid,tInt), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length,tFunc(tVoid,tInt), 0);
  /* function(string:void) */
  ADD_FUNCTION("set_encrypt_key", f_set_key,tFunc(tStr,tVoid), 0);
  /* function(string:void) */
  ADD_FUNCTION("set_decrypt_key", f_set_key,tFunc(tStr,tVoid), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt_block", f_crypt_block,tFunc(tStr,tStr), 0);

  set_init_callback(init_pike_crypto_invert);
  set_exit_callback(exit_pike_crypto_invert);

  end_class("invert", 0);
}

void pike_invert_exit(void)
{
}


