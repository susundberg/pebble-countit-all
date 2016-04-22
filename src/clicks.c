

#include <pebble.h>
#include <assert.h>
#include "main.h"

#define BUTTON_N_INDEX 3
#define BUFFER_SIZE 32

typedef struct __attribute__ ((__packed__))
{
   uint32_t time;
   uint16_t elapsed;
   
} ButtonRegistryBuffer;

typedef struct
{
  uint8_t  flags;
  ButtonId button_id;
  time_t   time_started;
  
  ButtonRegistryBuffer buffer[ BUFFER_SIZE ];
  unsigned int buffer_loop;
  
} ButtonRegistry;

ButtonRegistry LOCAL_registry[ BUTTON_N_INDEX ];


const ButtonRegistryBuffer* local_get_last_action( const ButtonRegistry* reg )
{
   unsigned int buffer_prev = reg->buffer_loop - 1;
   if ( buffer_prev >= BUFFER_SIZE )
      buffer_prev = BUFFER_SIZE - 1;
   
   const ButtonRegistryBuffer* ret = &reg->buffer[buffer_prev];
   if ( ret->time == 0 )
      return NULL;
   return ret;
}


static void local_new_action( ButtonRegistry* reg, time_t time_now, uint16_t time_passed )
{
   reg->buffer[ reg->buffer_loop ].elapsed = time_passed;
   reg->buffer[ reg->buffer_loop ].time    = (uint32_t) time_now;
   reg->buffer_loop += 1;
   reg->buffer_loop = (reg->buffer_loop % BUFFER_SIZE);
   
}

static void local_click_handler_single_action( ClickRecognizerRef recognizer, void *context) 
{
  unsigned int button_index = (unsigned int) context;
  ButtonRegistry* reg = &LOCAL_registry[button_index];
  time_t time_now = time( NULL );

  local_new_action( reg, time_now, 0xFFFF );
  main_window_update_elapsed( time_now );
}


static void local_click_handler_long_action( ClickRecognizerRef recognizer, void *context) 
{
  unsigned int button_index = (unsigned int) context;
  ButtonRegistry* reg = &LOCAL_registry[button_index];
  time_t time_now = time( NULL );
  
  if ( reg->time_started == 0 )
  {
     reg->time_started = time_now;
     return;
  }
  
  
  time_t time_started = reg->time_started;
  reg->time_started = 0;
  
  uint32_t elapsed_long = time_now - time_started;
  uint16_t elapsed = 0;
  
  if ( elapsed_long > 0xFFFF )
     elapsed = 0xFFFF;
  else
     elapsed = (uint16_t)elapsed_long ;
  
  
  local_new_action( reg, time_now, elapsed );
  main_window_update_elapsed( time_now );
}



static void local_click_config_provider_wrapper(  ButtonId button_id, void* context ) 
{ 
   
  unsigned int button_index = (unsigned int) context;
  
  uint8_t flags = LOCAL_registry[button_index].flags;
  
  APP_LOG( APP_LOG_LEVEL_DEBUG, "Register button %d %d", button_index, flags );
  
  if ( flags & FLAG_SINGLE_ACTION_BUTTON )
  {
     window_single_click_subscribe( button_id, local_click_handler_single_action ); // single click
  }
  if ( flags & FLAG_LONG_ACTION_BUTTON )
  {
     window_single_click_subscribe( button_id, local_click_handler_long_action ); // single click
  }
  
  // long click opens menu 
  // window_long_click_subscribe( button_id, 0, NULL, local_click_handler_long ); // long click, call on up
     
  window_set_click_context( button_id, context);
}



void click_config_provider( )
{
   for (unsigned int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      local_click_config_provider_wrapper( LOCAL_registry[loop].button_id, (void*)loop );
   }
}


void click_registry_init()
{
   memset( LOCAL_registry, 0x00, sizeof(LOCAL_registry));
   assert( sizeof(ButtonRegistryBuffer) == 6 ); // make sure no padding
   
   for (int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      LOCAL_registry[loop].button_id = (ButtonId)(loop + 1);
      LOCAL_registry[loop].flags = config_get( LOCAL_registry[loop].button_id );
   } 
}


bool click_registry_get_elapsed( time_t time_now, unsigned int index, unsigned int* elapsed )
{
   ButtonRegistry* reg = &LOCAL_registry[index];  

   if ( ( reg->flags & FLAG_LONG_ACTION_BUTTON ) == 0x00 )
      return false;
   
   if ( reg->time_started > 0 )
   {
      (*elapsed) = (unsigned int)time_now - (unsigned int)reg->time_started;
      return true;
   }
   return false; 
}


bool click_registry_get_since_last( time_t time_now, unsigned int index, unsigned int* elapsed )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   const ButtonRegistryBuffer* loop = local_get_last_action( reg );
   if ( loop == NULL )
      return false;
   
   *elapsed = (unsigned int)time_now - (unsigned int)loop->time;
   return true;
}


