

#include <pebble.h>
#include <assert.h>
#include "main.h"




typedef struct
{
  uint8_t  flags;
  ButtonId button_id;
  time_t   time_started;
  
  ButtonRegistryBuffer buffer[ BUFFER_SIZE ];
  unsigned int buffer_loop;
  
} ButtonRegistry;

ButtonRegistry LOCAL_registry[ BUTTON_N_INDEX ];

const ButtonRegistryBuffer* click_registry_get_action( unsigned int index, unsigned int loop )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   
   unsigned int current_index = (2*BUFFER_SIZE + (reg->buffer_loop - 1) - loop)%BUFFER_SIZE;
   
   if (reg->buffer[ current_index ].time == 0 )
      return NULL;
   return &reg->buffer[ current_index ];
}


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


static void local_click_open_menu(ClickRecognizerRef recognizer, void *context) 
{
   unsigned int button_index = (unsigned int) context;
   main_show_menu_window( button_index );
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
  
  bool enabled = false;
  
  if ( flags & FLAG_SINGLE_ACTION_BUTTON )
  {
     window_single_click_subscribe( button_id, local_click_handler_single_action ); // single click
     enabled = true;
  }
  if ( flags & FLAG_LONG_ACTION_BUTTON )
  {
     window_single_click_subscribe( button_id, local_click_handler_long_action ); // single click
     enabled = true;
  }
  
  
  if ( enabled )
  {
     // long click opens menu 
     window_long_click_subscribe( button_id, 0, NULL, local_click_open_menu ); // long click, call on up
  }
  
  window_set_click_context( button_id, context);
}



void click_config_provider( )
{
   for (unsigned int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      local_click_config_provider_wrapper( LOCAL_registry[loop].button_id, (void*)loop );
   }
}


ButtonId click_get_button_id_from_index(int index)
{
   return (ButtonId)(index + 1);
}



void local_load_buffer_from_storage( ButtonRegistry* reg, uint32_t key_offset )
{
   
   uint32_t loop = 0 ;
   for ( loop = 0; loop < BUFFER_SIZE; loop ++ )
   {
      int ret = persist_read_data( key_offset + loop + 1, &reg->buffer[loop], sizeof(ButtonRegistryBuffer) );
      if ( ret == E_DOES_NOT_EXIST )
         break;
   }
   
   if ( loop > 0 )
   {
     reg->buffer_loop = persist_read_int( key_offset ); 
   }
   
}

     

void click_registry_init()
{
   memset( LOCAL_registry, 0x00, sizeof(LOCAL_registry));
   assert( sizeof(ButtonRegistryBuffer) == 6 ); // make sure no padding
   
   for (int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      LOCAL_registry[loop].button_id = click_get_button_id_from_index( loop );
      LOCAL_registry[loop].flags = config_get( LOCAL_registry[loop].button_id );
   
      local_load_buffer_from_storage( &LOCAL_registry[loop], PERSISTANT_STORAGE_DATA_START + (BUFFER_SIZE + 1)*loop );
      
      
      
   } 
   
   
   
}

bool click_registry_enabled( unsigned int index )
{
   ButtonRegistry* reg = &LOCAL_registry[index];  

   if ( ( reg->flags & FLAG_LONG_ACTION_BUTTON ) != 0x00 )
      return true;
   
   if ( ( reg->flags & FLAG_SINGLE_ACTION_BUTTON ) != 0x00 )
      return true;
   
   return false;
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


