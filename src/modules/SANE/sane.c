#include "config.h"

#if defined(HAVE_SANE_SANE_H) || defined(HAVE_SANE_H)
#ifdef HAVE_SANE_SANE_H
#include <sane/sane.h>
#else
#ifdef HAVE_SANE_H
#include <sane.h>
#endif
#endif

#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "mapping.h"
#include "multiset.h"
#include "operators.h"
#include "module_support.h"
#include "builtin_functions.h"

#include "../Image/image.h"

static int sane_is_inited;

struct scanner
{
  SANE_Handle h;
};

static void init_sane()
{
  if( sane_init( NULL, NULL ) )
    error( "Sane init failed.\n" );
  sane_is_inited =  1;
}

static void push_device( SANE_Device *d )
{
  push_text( "name" );    push_text( d->name );
  push_text( "vendor" );  push_text( d->vendor );
  push_text( "model" );   push_text( d->model );
  push_text( "type" );    push_text( d->type );
  f_aggregate_mapping( 8 );
}


static void f_list_scanners( INT32 args )
{
  SANE_Device **devices;
  int i = 0;

  if( !sane_is_inited ) init_sane();
  switch( sane_get_devices( (void *)&devices, 0 ) )
  {
   case 0:
     while( devices[i] ) push_device( devices[i++] );
     f_aggregate( i );
     break;
   default:
     error("Failed to get device list\n");
  }
}

#define THIS ((struct scanner *)fp->current_storage)

static void push_option_descriptor( const SANE_Option_Descriptor *o )
{
  int i;
  struct svalue *osp = sp;
  push_text( "name" );
  if( o->name )
    push_text( o->name );
  else
    push_int( 0 );
  push_text( "title" );
  if( o->title )
    push_text( o->title );
  else
    push_int( 0 );
  push_text( "desc" );
  if( o->desc )
    push_text( o->desc );
  else
    push_int( 0 );
  push_text( "type" );
  switch( o->type )
  {
   case SANE_TYPE_BOOL:   push_text( "boolean" ); break;
   case SANE_TYPE_INT:    push_text( "int" );     break;
   case SANE_TYPE_FIXED:  push_text( "float" );   break;
   case SANE_TYPE_STRING: push_text( "string" );  break;
   case SANE_TYPE_BUTTON: push_text( "button" );  break;
   case SANE_TYPE_GROUP:  push_text( "group" );   break;
  }


  push_text( "unit" );
  switch( o->unit )
  {
   case SANE_UNIT_NONE:  	push_text( "none" ); 		break;
   case SANE_UNIT_PIXEL:  	push_text( "pixel" ); 		break;
   case SANE_UNIT_BIT:  	push_text( "bit" ); 		break;
   case SANE_UNIT_MM:  		push_text( "mm" ); 		break;
   case SANE_UNIT_DPI:          push_text( "dpi" ); 		break;
   case SANE_UNIT_PERCENT:      push_text( "percent" );   	break;
   case SANE_UNIT_MICROSECOND:  push_text( "microsecond" ); 	break;
  }

  push_text( "size" );   push_int( o->size );

  push_text( "cap" );
  {
    struct svalue *osp = sp;
    if( o->cap & SANE_CAP_SOFT_SELECT )  push_text( "soft_select" );
    if( o->cap & SANE_CAP_HARD_SELECT )  push_text( "hard_select" );
    if( o->cap & SANE_CAP_EMULATED )     push_text( "emulated" );
    if( o->cap & SANE_CAP_AUTOMATIC )    push_text( "automatic" );
    if( o->cap & SANE_CAP_INACTIVE )     push_text( "inactive" );
    if( o->cap & SANE_CAP_ADVANCED )     push_text( "advanced" );
    f_aggregate_multiset( sp - osp );
  }

  push_text( "constaint" );
  switch( o->constraint_type )
  {
   case SANE_CONSTRAINT_NONE:  push_int( 0 ); break;
   case SANE_CONSTRAINT_RANGE:
     push_text( "type" );  push_text( "range" );
     push_text( "min" );   push_int( o->constraint.range->min );
     push_text( "max" );   push_int( o->constraint.range->max );
     push_text( "quant" ); push_int( o->constraint.range->quant );
     f_aggregate_mapping( 8 );
     break;
   case SANE_CONSTRAINT_WORD_LIST:
     push_text( "type" );
     push_text( "list" );
     push_text( "list" );
     for( i = 0; i<o->constraint.word_list[0]; i++ )
       if( o->type == SANE_TYPE_FIXED )
         push_float( SANE_UNFIX(o->constraint.word_list[i+1]) );
       else
         push_int( o->constraint.word_list[i+1] );
     f_aggregate( o->constraint.word_list[0] );
     f_aggregate_mapping( 4 );
     break;
   case SANE_CONSTRAINT_STRING_LIST:
     push_text( "type" );
     push_text( "list" );
     push_text( "list" );
     for( i = 0; o->constraint.string_list[i]; i++ )
       push_text( o->constraint.string_list[i] );
     f_aggregate( i );
     f_aggregate_mapping( 4 );
     break;
  }
  f_aggregate_mapping( sp - osp );
}

static void f_scanner_create( INT32 args )
{
  char *name;
  if(!sane_is_inited) init_sane();
  get_all_args( "create", args, "%s", &name );

  if( sane_open( name, &THIS->h ) )
    error("Failed to open scanner \"%s\"\n", name );
}

static void f_scanner_list_options( INT32 args )
{
  int i, n;
  const SANE_Option_Descriptor *d;
  pop_n_elems( args );

  for( i = 1; (d = sane_get_option_descriptor( THIS->h, i) ); i++ )
    push_option_descriptor( d );
  f_aggregate( i-1 );
}

static int find_option( char *name, const SANE_Option_Descriptor **p )
{
  int i;
  const SANE_Option_Descriptor *d;
  for( i = 1; (d = sane_get_option_descriptor( THIS->h, i ) ); i++ )
    if(d->name && !strcmp( d->name, name ) )
    {
      *p = d;
      return i;
    }
  error("No such option: %s\n", name );
}

static void f_scanner_set_option( INT32 args )
{
  char *name;
  int no;
  SANE_Int int_value;
  float float_value;
  SANE_Int tmp;
  const SANE_Option_Descriptor *d;
  get_all_args( "set_option", args, "%s", &name );

  no = find_option( name, &d );
  if( args > 1 )
  {
    switch( d->type )
    {
     case SANE_TYPE_BOOL:
     case SANE_TYPE_INT:
     case SANE_TYPE_BUTTON:
       sp++;get_all_args( "set_option", args, "%D", &int_value );sp--;
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &int_value, &tmp );
       break;
     case SANE_TYPE_FIXED:
       sp++;get_all_args( "set_option", args, "%F", &float_value );sp--;
       int_value = SANE_FIX(((double)float_value));
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &int_value, &tmp );
       break;
     case SANE_TYPE_STRING:
       sp++;get_all_args( "set_option", args, "%s", &name );sp--;
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &name, &tmp );
     case SANE_TYPE_GROUP:
       break;
    }
  } else {
    int_value = 1;
    sane_control_option( THIS->h, no, SANE_ACTION_SET_AUTO, &int_value, &tmp );
  }
  pop_n_elems( args );
  push_int( 0 );
}

static void f_scanner_get_option( INT32 args )
{
  char *name;
  int no;
  SANE_Int int_value;
  float f;
  SANE_Int tmp;
  const SANE_Option_Descriptor *d;
  get_all_args( "get_option", args, "%s", &name );

  no = find_option( name, &d );

  switch( d->type )
  {
   case SANE_TYPE_BOOL:
   case SANE_TYPE_INT:
   case SANE_TYPE_BUTTON:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &int_value, &tmp );
     pop_n_elems( args );
     push_int( int_value );
     return;
   case SANE_TYPE_FIXED:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &int_value, &tmp );
     pop_n_elems( args );
     push_float( SANE_UNFIX( int_value ) );
     break;
   case SANE_TYPE_STRING:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &name, &tmp );
     pop_n_elems( args );
     push_text( name );
   case SANE_TYPE_GROUP:
     break;
  }
}

static void f_scanner_get_parameters( INT32 args )
{
  SANE_Parameters p;
  pop_n_elems( args );
  sane_get_parameters( THIS->h, &p );
  push_text( "format" );          push_int( p.format );
  push_text( "last_frame" );      push_int( p.last_frame );
  push_text( "lines" );           push_int( p.lines );
  push_text( "depth" );           push_int( p.depth );
  push_text( "pixels_per_line" ); push_int( p.pixels_per_line );
  push_text( "bytes_per_line" );  push_int( p.bytes_per_line );
  f_aggregate_mapping( 12 );
}


static struct program *image_program;

static void get_grey_frame( SANE_Handle h, SANE_Parameters *p, char *data )
{
  char buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    char *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
    {
      *(data++) = *(pp);
      *(data++) = *(pp);
      *(data++) = *(pp++);
    }
  }
}

static void get_rgb_frame( SANE_Handle h, SANE_Parameters *p, char *data )
{
  char buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    char *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
      *(data++) = *(pp++);
  }
}

static void get_comp_frame( SANE_Handle h, SANE_Parameters *p, char *data )
{
  char buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    char *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
    {
      data[0] = *(pp++);
      data += 3;
    }
  }
}

static void assert_image_program()
{
  if( !image_program )
  {
    push_text( "Image.Image" );
    APPLY_MASTER( "resolv", 1 );
    image_program = program_from_svalue( sp - 1  );
    pop_stack();
  }
}

static void f_scanner_simple_scan( INT32 args )
{
  SANE_Parameters p;
  SANE_Handle h = THIS->h;
  struct object *o;
  rgb_group *r;


  pop_n_elems( args );
  if( sane_start( THIS->h ) )   error("Start failed\n");
  if( sane_get_parameters( THIS->h, &p ) )  error("Get parameters failed\n");

  if( p.depth != 8 )
    error("Sorry, only depth 8 supported right now.\n");

  push_int( p.pixels_per_line );
  push_int( p.lines );
  o = clone_object( image_program, 2 );
  r = ((struct image *)o->storage)->img;

  THREADS_ALLOW();
  do
  {
    switch( p.format )
    {
     case SANE_FRAME_GRAY:
       get_grey_frame( h, &p, (char *)r );
       p.last_frame = 1;
       break;
     case SANE_FRAME_RGB:
       get_rgb_frame(  h, &p, (char *)r );
       p.last_frame = 1;
       break;
     case SANE_FRAME_RED:
       get_comp_frame( h, &p, ((char *)r) );
       break;
     case SANE_FRAME_GREEN:
       get_comp_frame( h, &p, ((char *)r)+1 );
       break;
     case SANE_FRAME_BLUE:
       get_comp_frame( h, &p, ((char *)r)+2 );
       break;
    }
  }
  while( !p.last_frame );

  THREADS_DISALLOW();
  push_object( o );
}

static void f_scanner_row_scan( INT32 args )
{
  SANE_Parameters p;
  SANE_Handle h = THIS->h;
  struct svalue *s;
  struct object *o;
  rgb_group *r, or;
  int i, nr;

  if( sane_start( THIS->h ) )               error("Start failed\n");
  if( sane_get_parameters( THIS->h, &p ) )  error("Get parameters failed\n");
  if( p.depth != 8 )  error("Sorry, only depth 8 supported right now.\n");

  assert_image_program();
  switch( p.format )
  {
   case SANE_FRAME_GRAY:
   case SANE_FRAME_RGB:
     break;
   case SANE_FRAME_RED:
   case SANE_FRAME_GREEN:
   case SANE_FRAME_BLUE:
     error("Composite frame mode not supported for row_scan\n");
     break;
  }
  push_int( p.pixels_per_line );
  push_int( 1 );
  o = clone_object( image_program, 2 );
  r = ((struct image *)o->storage)->img;

  nr = p.lines;
  p.lines=1;

  for( i = 0; i<nr; i++ )
  {
    THREADS_ALLOW();
    switch( p.format )
    {
     case SANE_FRAME_GRAY:
       get_grey_frame( h, &p, (char *)r );
       break;
     case SANE_FRAME_RGB:
       get_rgb_frame(  h, &p, (char *)r );
       break;
     case SANE_FRAME_RED:
     case SANE_FRAME_GREEN:
     case SANE_FRAME_BLUE:
       break;
    }
    THREADS_DISALLOW();
    ref_push_object( o );
    push_int( i );
    ref_push_object( fp->current_object );
    apply_svalue( sp-args-3, 3 );
    pop_stack();
  }
  free_object( o );
  pop_n_elems( args );
  push_int( 0 );
}

static void f_scanner_cancel_scan( INT32 args )
{
  sane_cancel( THIS->h );
}

static void init_scanner_struct( struct object *p )
{
  THIS->h = 0;
}

static void exit_scanner_struct( struct object *p )
{
  if( THIS->h )
    sane_close( THIS->h );
}



void pike_module_init()
{
  struct program *p;
  add_function( "list_scanners", f_list_scanners,
                "function(void:array(mapping))", 0 );

  start_new_program();
  ADD_STORAGE( struct scanner );
  add_function( "get_option", f_scanner_get_option,
                "function(string:mixed)", 0 );
  add_function( "set_option", f_scanner_set_option,
                "function(string,void|mixed:void)", 0 );
  add_function( "list_options", f_scanner_list_options,
                    "function(void:array(mapping(string:mixed)))", 0 );

  add_function( "simple_scan", f_scanner_simple_scan,
                "function(void:object)", 0 );

  add_function( "row_scan", f_scanner_row_scan,
                "function(function(object,int,object:void):void)", 0 );

  add_function( "cancel_scan", f_scanner_cancel_scan,
                "function(void:object)", 0 );

  add_function( "get_parameters", f_scanner_get_parameters,
                "function(void:mapping)", 0 );

  add_function( "create", f_scanner_create,
                "function(string:void)", 0 );

   set_init_callback(init_scanner_struct);
   set_exit_callback(exit_scanner_struct);

  add_program_constant( "Scanner", (p=end_program( ) ), 0 );
  free_program( p );
}

void pike_module_exit()
{
  if( sane_is_inited )
    sane_exit();
}

#else
void pike_module_init() {}
void pike_module_exit() {}
#endif
