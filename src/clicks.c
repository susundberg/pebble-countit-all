

#include <pebble.h>
#include <assert.h>
#include "main.h"



typedef struct
{
  ButtonRegistryBuffer buffer[ HISTORY_SIZE ];
  uint32_t             buffer_loop;
  
} ButtonRegistryHistory;

typedef struct
{
  time_t   time_started;
  ButtonRegistryHistory history;
  ButtonId button_id;
  uint8_t  flags; 
} ButtonRegistry;

ButtonRegistry LOCAL_registry[ BUTTON_N_INDEX ];

   


const ButtonRegistryBuffer* click_registry_get_action( unsigned int index, unsigned int loop )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   
   unsigned int current_index = (2*HISTORY_SIZE + (reg->history.buffer_loop - 1) - loop)%HISTORY_SIZE;
   
   if (reg->history.buffer[ current_index ].time == 0 )
      return NULL;
   return &reg->history.buffer[ current_index ];
}


void click_registry_clear( unsigned int index, unsigned int loop )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   unsigned int buffer_index = (2*HISTORY_SIZE + (reg->history.buffer_loop - 1) - loop)%HISTORY_SIZE;
   
   memset( &reg->history.buffer[buffer_index], 0x00, sizeof( ButtonRegistryBuffer ) );
//    APP_LOG( APP_LOG_LEVEL_INFO, "Clearing entry %d (ind: %d)", (int)loop, (int)buffer_index );   
}

void click_registry_clear_finish( unsigned int index)
{
   ButtonRegistryHistory new_history;
   ButtonRegistry* reg = &LOCAL_registry[index];
   
   memset( &new_history, 0x00, sizeof(ButtonRegistryHistory));
   
   unsigned int last_index = reg->history.buffer_loop;
   APP_LOG( APP_LOG_LEVEL_INFO, "Clearing index from %d", (int)last_index);   
      
   for ( unsigned int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      unsigned int current_index = (last_index + loop) % HISTORY_SIZE ;
      
      if ( reg->history.buffer[ current_index ].time == 0 )
         continue;
      
      new_history.buffer[ new_history.buffer_loop ] = reg->history.buffer[ current_index ];
      new_history.buffer_loop = (new_history.buffer_loop + 1) % HISTORY_SIZE ;
   }
    
   reg->history = new_history; // shallow copy 
}


const ButtonRegistryBuffer* local_get_last_action( const ButtonRegistry* reg )
{
   unsigned int buffer_prev = reg->history.buffer_loop - 1;
   if ( buffer_prev >= HISTORY_SIZE )
      buffer_prev = HISTORY_SIZE - 1;
   
   const ButtonRegistryBuffer* ret = &reg->history.buffer[buffer_prev];
   if ( ret->time == 0 )
      return NULL;
   return ret;
}


static void local_new_action( ButtonRegistryHistory* reg, time_t time_now, uint16_t time_passed, ButtonId button )
{
   reg->buffer_loop = (reg->buffer_loop % HISTORY_SIZE); // Just to be sure there is no overflow, if the history size has changed in versions.
   reg->buffer[ reg->buffer_loop ].elapsed = time_passed;
   reg->buffer[ reg->buffer_loop ].time    = (uint32_t) time_now;
   reg->buffer_loop += 1;
   reg->buffer_loop = (reg->buffer_loop % HISTORY_SIZE);
   
   uint32_t storage_key = PERSISTANT_STORAGE_DATA_START + button ;
   
   int ret = persist_write_data( storage_key, reg, sizeof(ButtonRegistryHistory) );
   if ( ret != sizeof(ButtonRegistryHistory))
   {
      APP_LOG( APP_LOG_LEVEL_ERROR, "Writing registry %d failed: %d", (int)storage_key, ret);   
   }
//    APP_LOG( APP_LOG_LEVEL_INFO, "Added new action ind: %u", (unsigned int)reg->buffer_loop);
}

static void local_click_handler_single_action( ClickRecognizerRef recognizer, void *context) 
{
  unsigned int button_index = (unsigned int) context;
  
  ButtonRegistry* reg = &LOCAL_registry[button_index];
  time_t time_now = time( NULL );

  local_new_action( &reg->history, time_now, 0xFFFF, reg->button_id );
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
  
  
  local_new_action( &reg->history, time_now, elapsed, reg->button_id );
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



void local_load_buffer_from_storage( ButtonRegistryHistory* reg, uint32_t key_offset )
{
   
   int ret = persist_read_data( key_offset, reg, sizeof(ButtonRegistryHistory) );

   if ( ret == E_DOES_NOT_EXIST )
      return;
   
   if ( ret == sizeof(ButtonRegistryHistory))
      return;
   
  APP_LOG( APP_LOG_LEVEL_ERROR, "Reading registry %d failed: %d", (int)key_offset, ret);   
}

     

void click_registry_init()
{
   memset( LOCAL_registry, 0x00, sizeof(LOCAL_registry));
   assert( sizeof(ButtonRegistryBuffer) == 6 ); // make sure no padding
   
   for (int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      LOCAL_registry[loop].button_id = click_get_button_id_from_index( loop );
      LOCAL_registry[loop].flags = config_get( LOCAL_registry[loop].button_id );
      local_load_buffer_from_storage( &LOCAL_registry[loop].history, PERSISTANT_STORAGE_DATA_START + LOCAL_registry[loop].button_id );
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

bool click_registry_get_elapsed( time_t time_now, unsigned int index, unsigned int* elapsed, unsigned int* action_time )
{
   ButtonRegistry* reg = &LOCAL_registry[index];  

   if ( ( reg->flags & FLAG_LONG_ACTION_BUTTON ) == 0x00 )
      return false;
   
   if ( reg->time_started > 0 )
   {
      (*elapsed)     = (unsigned int)time_now - (unsigned int)reg->time_started;
      (*action_time) = (unsigned int)reg->time_started;
      return true;
   }
   return false; 
}


bool click_registry_get_since_last( time_t time_now, unsigned int index, unsigned int* elapsed, unsigned int* action_time )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   const ButtonRegistryBuffer* loop = local_get_last_action( reg );
   if ( loop == NULL )
      return false;
   
   *elapsed = (unsigned int)time_now - (unsigned int)loop->time;
   *action_time = (unsigned int)loop->time;
   return true;
}


