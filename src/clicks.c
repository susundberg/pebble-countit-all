

#include <pebble.h>
#include "main.h"



typedef struct
{
  ButtonRegistryBuffer buffer[ HISTORY_SIZE ];
  uint32_t             buffer_loop;
  
} ButtonRegistryHistory;

typedef struct
{
  ButtonRegistryHistory history;
  ButtonId button_id;
  uint32_t to_send_n;
  uint8_t  flags; 
} ButtonRegistry;

ButtonRegistry LOCAL_registry[ BUTTON_N_INDEX ];

bool click_registry_send_has_any()
{
   for (int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      if ( LOCAL_registry[ loop ].to_send_n > 0 )
         return true;
   }
   return false;
}


#define MAX_ITEMS_TO_SEND ( APP_MESSAGE_OUTBOX_SIZE_MINIMUM/sizeof(ButtonRegistryBuffer))

#define OUTBOX_SIZE  // estimate: 1 + 2 * 7 + 1 + (8 * HISTORY_SIZE) 

static uint8_t LOCAL_send_buffer[(8 * HISTORY_SIZE)]; // each item takes

#define DICT_WRITE_CHECK(fun){\
  DictionaryResult __macro_res = fun;\
  if ( __macro_res != DICT_OK )\
  {\
    APP_LOG( APP_LOG_LEVEL_ERROR, "Dict write failed %s:%d (%d)", __FILE__,__LINE__, (int)__macro_res ); \
    return 0;\
  }}


  
  
static void local_encode_uint32( uint32_t* offset, uint32_t data )
{
   for (int loop =  0; loop < 4; loop ++ )
   {
      uint32_t mask = (0xFF) << (loop*8);
      LOCAL_send_buffer[ (*offset) + loop ] = (data & mask) >> (loop*8);
   }
   *offset += 4;
}


static void local_click_write_storage( uint32_t button, ButtonRegistryHistory* hist )
{
   uint32_t storage_key = PERSISTANT_STORAGE_DATA_START + button ;
   
   int ret = persist_write_data( storage_key, hist, sizeof(ButtonRegistryHistory) );
   if ( ret != sizeof(ButtonRegistryHistory))
   {
      APP_LOG( APP_LOG_LEVEL_ERROR, "Writing history %d failed: %d vs %d", (int)storage_key, ret, sizeof(ButtonRegistryHistory) );   
   }
}


int click_registry_send_write(DictionaryIterator* dict )
{
   uint32_t send_buffer_offset = 0;
   
   for (int index = 0; index < BUTTON_N_INDEX; index ++ )
   {
      ButtonRegistry* reg = &LOCAL_registry[index];
      
      if ( reg->to_send_n == 0 )
         continue;
      
      DICT_WRITE_CHECK( dict_write_uint8( dict, COMM_KEY_BUTTON_INDEX, (uint8_t)index ) );
      
      unsigned int last_index = reg->history.buffer_loop;
      
      for ( unsigned int loop = 0; loop < HISTORY_SIZE; loop ++ )
      {
         // start from the oldest
         unsigned int current_index = (last_index + loop) % HISTORY_SIZE ;
         
         ButtonRegistryBuffer* buffer = reg->history.buffer;
         if (( buffer[current_index].flags_n_elapsed & BUFFER_FLAG_NOT_SENT) == 0x00 )
            continue;
      
        // clear not sent flag
        buffer[current_index].flags_n_elapsed &= ~BUFFER_FLAG_NOT_SENT;

        // and raise sending flag
        buffer[current_index].flags_n_elapsed |= BUFFER_FLAG_SENDING;
        
        APP_LOG( APP_LOG_LEVEL_INFO, "Sending button %d index %d", (int)index, (int)current_index );   
        
        // Todo: here we send almost double the amount we would need when button type is 
        // is single action button. I guess it doesnt matter -- the amount are small anyway.
        
        uint32_t to_send = BUFFER_FLAG_MASK | buffer[current_index].flags_n_elapsed;
        
        local_encode_uint32( &send_buffer_offset, to_send );
        local_encode_uint32( &send_buffer_offset, buffer[current_index].time );
      }
      
      DICT_WRITE_CHECK( dict_write_data( dict, COMM_KEY_BUTTON_DATA, LOCAL_send_buffer, send_buffer_offset ));
      
      reg->to_send_n = 0;
      
      return 1;
   }
   return 0;
}


void click_registry_send_clear( uint32_t index, bool sent_ok )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   uint32_t to_send = 0;   
   ButtonRegistryBuffer* buffer = reg->history.buffer;
   for ( unsigned int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      if ( buffer[loop].flags_n_elapsed & BUFFER_FLAG_SENDING ) 
         buffer[loop].flags_n_elapsed &= ~BUFFER_FLAG_SENDING ;

      if (sent_ok == false )
      {
         buffer[loop].flags_n_elapsed |= BUFFER_FLAG_NOT_SENT;
         to_send += 1;
      }
   }
   reg->to_send_n += to_send;
   local_click_write_storage( click_get_button_id_from_index(index), &reg->history); // and write it to presistant storage
}



const ButtonRegistryBuffer* click_registry_get_action( unsigned int index, unsigned int loop )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   
   unsigned int current_index = (2*HISTORY_SIZE + (reg->history.buffer_loop - 1) - loop)%HISTORY_SIZE;
   
   if (reg->history.buffer[ current_index ].time == 0 )
      return NULL;
   
   if ( reg->flags & BUTTONTYPE_FLAG_DURATION )
   {
      if ( reg->history.buffer[current_index].flags_n_elapsed & BUFFER_FLAG_RUNNING )
         return NULL;
   } 
   return &reg->history.buffer[ current_index ];
}


void click_registry_clear( unsigned int index, unsigned int loop )
{
   ButtonRegistry* reg = &LOCAL_registry[index];
   unsigned int buffer_index = (2*HISTORY_SIZE + (reg->history.buffer_loop - 1) - loop)%HISTORY_SIZE;
   
   memset( &reg->history.buffer[buffer_index], 0x00, sizeof( ButtonRegistryBuffer ) );
   APP_LOG( APP_LOG_LEVEL_INFO, "Clearing entry %d (ind: %d)", (int)loop, (int)buffer_index );   
}

void click_registry_clear_finish( unsigned int index)
{
   ButtonRegistryHistory new_history;
   ButtonRegistry* reg = &LOCAL_registry[index];
   
   memset( &new_history, 0x00, sizeof(ButtonRegistryHistory));
   
   unsigned int last_index = reg->history.buffer_loop;
   APP_LOG( APP_LOG_LEVEL_INFO, "Clearing index from %d", (int)last_index);   
   bool had_running_measurement = false;
   
   for ( unsigned int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      // copy the old data entries, starting from the oldest
      unsigned int current_index = (last_index + loop) % HISTORY_SIZE ;
      
      if ( reg->history.buffer[ current_index ].time == 0 )
         continue;
      
      APP_LOG( APP_LOG_LEVEL_INFO, "Valid index %d", (int)current_index);   
      new_history.buffer[ new_history.buffer_loop ] = reg->history.buffer[ current_index ];
      
      if ( reg->history.buffer[ current_index ].flags_n_elapsed & BUFFER_FLAG_RUNNING )
      {
         if ( loop != 0 )
         {
            APP_LOG( APP_LOG_LEVEL_ERROR, "Running measurement at index %d -> discard", (int)loop );
            continue;
         }
         had_running_measurement  = true; 
      }
      new_history.buffer_loop = (new_history.buffer_loop + 1) % HISTORY_SIZE ;   
   }
   
   if (had_running_measurement)
   {  // If we had running measurement, copy that to top and clear the floor index.
      new_history.buffer[ new_history.buffer_loop ] = new_history.buffer[ 0 ];
      memset( &new_history.buffer[0], 0x00, sizeof( ButtonRegistryBuffer ) );
   }
    
   reg->history = new_history; // shallow copy it to ram
   local_click_write_storage( click_get_button_id_from_index(index), &new_history ); // and write it to presistant storage
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






static void local_new_action( ButtonRegistry* reg, time_t time_now, uint32_t time_passed, ButtonId button, uint32_t flags )
{
   ButtonRegistryHistory* hist = &reg->history;
   if ((time_passed & BUFFER_FLAG_MASK) != 0x00)
   {
      time_passed  = (~BUFFER_FLAG_MASK);
   }
   
   flags = flags | BUFFER_FLAG_NOT_SENT;
   
   hist->buffer_loop = (hist->buffer_loop % HISTORY_SIZE); // Just to be sure there is no overflow, if the history size has changed in versions.
   hist->buffer[ hist->buffer_loop ].flags_n_elapsed = flags | time_passed;
   hist->buffer[ hist->buffer_loop ].time    = (uint32_t) time_now;
   
   if (( flags & BUFFER_FLAG_RUNNING ) == 0x00)
   {
      hist->buffer_loop += 1;
      hist->buffer_loop = (hist->buffer_loop % HISTORY_SIZE);
      reg->to_send_n += 1;
      communication_request_for_send();
   }
   
   local_click_write_storage( button, hist );
}

   
static void local_click_handler_single_action( ClickRecognizerRef recognizer, void *context) 
{
  unsigned int button_index = (unsigned int) context;
  
  ButtonRegistry* reg = &LOCAL_registry[button_index];
  time_t time_now = time( NULL );

  local_new_action( reg, time_now, 0x00, reg->button_id, 0x00 );
  main_window_update_elapsed( time_now );
}


static void local_click_open_menu(ClickRecognizerRef recognizer, void *context) 
{
   unsigned int button_index = (unsigned int) context;
   main_show_menu_window( button_index );
}

static bool local_duration_measurement_ongoing( const ButtonRegistry* reg, uint32_t* time_started )
{
  const ButtonRegistryHistory* hist = &reg->history;
  uint32_t current_index = hist->buffer_loop % HISTORY_SIZE;
  
  bool ret = ( ( hist->buffer[ current_index ].flags_n_elapsed & BUFFER_FLAG_RUNNING ) != 0x00 );
  
  if (ret == false)
     return false;
  
  (*time_started) = hist->buffer[current_index].time;
  return true;
}

static void local_click_handler_long_action( ClickRecognizerRef recognizer, void *context) 
{
  unsigned int button_index = (unsigned int) context;
  ButtonRegistry* reg = &LOCAL_registry[button_index];
  time_t time_now = time( NULL );
  uint32_t time_started;
  
  if ( local_duration_measurement_ongoing( reg, &time_started ) == false )
  {
     // No measurement ongoing, start new
     //      APP_LOG( APP_LOG_LEVEL_DEBUG, "Starte measurement on button %d %d", button_index, (int)time_now );
     local_new_action( reg, time_now, 0x00, reg->button_id, BUFFER_FLAG_RUNNING  );
     main_window_update_elapsed( time_now );
     return;
  }
  
  //   APP_LOG( APP_LOG_LEVEL_DEBUG, "Done measurement on button %d %d", button_index, (int)time_started );
  // This is finalization of the measurement
  uint32_t elapsed_long = time_now - time_started;
  local_new_action( reg, time_started, elapsed_long, reg->button_id, 0x00 );
  
  main_window_update_elapsed( time_now );
}



static void local_click_config_provider_wrapper(  ButtonId button_id, void* context ) 
{ 
   
  unsigned int button_index = (unsigned int) context;
  uint8_t flags = LOCAL_registry[button_index].flags;
  
  APP_LOG( APP_LOG_LEVEL_DEBUG, "Register button %d %d", button_index, flags );
  
  bool enabled = false;
  if ( flags & BUTTONTYPE_FLAG_SINGLE )
  {
     window_single_click_subscribe( button_id, local_click_handler_single_action ); // single click
     enabled = true;
  }
  if ( flags & BUTTONTYPE_FLAG_DURATION )
  {
     window_single_click_subscribe( button_id, local_click_handler_long_action ); // long action
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

     
static uint32_t local_to_send_count( ButtonRegistryHistory* reg )
{
   uint32_t to_send = 0;
   for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      if ( reg->buffer[ loop ].time == 0 )
         continue;
      
      if (( reg->buffer[ loop ].flags_n_elapsed & BUFFER_FLAG_NOT_SENT ) == 0x00 )
         continue;

      if (( reg->buffer[ loop ].flags_n_elapsed & BUFFER_FLAG_RUNNING ) != 0x00 )
         continue;
      
      to_send += 1;
   }
   return to_send;   
}

void click_registry_init()
{
   memset( LOCAL_registry, 0x00, sizeof(LOCAL_registry));
   
   for (int loop = 0; loop < BUTTON_N_INDEX; loop ++ )
   {
      LOCAL_registry[loop].button_id = click_get_button_id_from_index( loop );
      LOCAL_registry[loop].flags = config_get( LOCAL_registry[loop].button_id );
      local_load_buffer_from_storage( &LOCAL_registry[loop].history, PERSISTANT_STORAGE_DATA_START + LOCAL_registry[loop].button_id );
      LOCAL_registry[loop].to_send_n = local_to_send_count( &LOCAL_registry[loop].history );
      APP_LOG( APP_LOG_LEVEL_DEBUG, "Button %d has %d to send", (int)loop, (int)LOCAL_registry[loop].to_send_n );   
   } 
   
}

bool click_registry_enabled( unsigned int index )
{
   ButtonRegistry* reg = &LOCAL_registry[index];  

   if ( ( reg->flags & BUTTONTYPE_FLAG_DURATION ) != 0x00 )
      return true;
   
   if ( ( reg->flags & BUTTONTYPE_FLAG_SINGLE ) != 0x00 )
      return true;
   
   return false;
}

bool click_registry_get_elapsed( time_t time_now, unsigned int index, unsigned int* elapsed, unsigned int* action_time )
{
   ButtonRegistry* reg = &LOCAL_registry[index];  

   if ( ( reg->flags & BUTTONTYPE_FLAG_DURATION ) == 0x00 )
      return false;
   
   uint32_t started_at;
   if ( local_duration_measurement_ongoing( reg, &started_at) == true )
   {
      (*elapsed)     = (unsigned int)time_now - (unsigned int)started_at;
      (*action_time) = (unsigned int)started_at;
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


