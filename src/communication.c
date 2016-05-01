#include <pebble.h>
#include "main.h"


#define CHECK_APP_MESSAGE(fun){\
   AppMessageResult __macro_res = fun;\
   if ( __macro_res != APP_MSG_OK ) \
   { \
      APP_LOG( APP_LOG_LEVEL_WARNING, "Communication failed: %d", (int)__macro_res); \
      return;\
   }};

static time_t LOCAL_time_next_send = 0;
static bool   LOCAL_js_booted = false;

time_t communication_next_attempt()
{
   return LOCAL_time_next_send;
}

void communication_request_for_send()
{
   LOCAL_time_next_send = 0;
}


void communication_send_datas( time_t time_now )
{
   APP_LOG( APP_LOG_LEVEL_INFO, "Comm: Check for data send" ); 
   
   if ( click_registry_send_has_any() == false )
   {
      LOCAL_time_next_send = time_now + 120; // well something large
      return;
   }
   
   APP_LOG( APP_LOG_LEVEL_INFO, "Comm: We got data" ); 
   
   if (connection_service_peek_pebblekit_connection() == false )
      return;
   
   APP_LOG( APP_LOG_LEVEL_INFO, "Comm: We got connection" ); 
   if ( LOCAL_js_booted == false )
      return;
      
   APP_LOG( APP_LOG_LEVEL_INFO, "Comm: JS has booted. " ); 
   
   DictionaryIterator* dict;
   
   CHECK_APP_MESSAGE( app_message_outbox_begin(&dict) );
   click_registry_send_write( dict );
   CHECK_APP_MESSAGE( app_message_outbox_send() );
   
   LOCAL_time_next_send = time_now + 10;
}
   


static bool local_handle_icon(DictionaryIterator* iter, int loop )
{
     Tuple* tuple_icon = dict_find(iter, COMM_KEY_BUTTON1_ICON + loop );
     if ( tuple_icon != NULL )
     {
        config_set_icon( click_get_button_id_from_index( loop ), tuple_icon->value->int32 );
        return true;
     }
     return false;
}

static bool local_handle_type(DictionaryIterator* iter, int loop )
{
     Tuple* tuple_type = dict_find(iter, COMM_KEY_BUTTON1_TYPE + loop);
    
     if ( tuple_type != NULL )
     {
        
        uint8_t flag = 0x00;
        switch ( tuple_type->value->cstring[0] ) 
        {
           case 'D':
              flag = BUTTONTYPE_FLAG_DURATION;
              break;
           case 'S':
              flag = BUTTONTYPE_FLAG_SINGLE;
              break;
           default:
              flag = 0x00;
              break;
        }       
        config_set_flags( click_get_button_id_from_index( loop ), flag );
        return true;
     }
     return false;
}

static void inbox_received_callback(DictionaryIterator* iter, void* context) 
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  bool received_config = false;
  for (int loop_button = 0; loop_button < 3; loop_button ++ )
  {
     
     received_config |= local_handle_icon( iter, loop_button );
     received_config |= local_handle_type( iter, loop_button );
  }
  
  
  Tuple* tuple_js_boot = dict_find(iter, COMM_KEY_JS_READY );
  
  if (tuple_js_boot!=NULL)
  {
     APP_LOG(APP_LOG_LEVEL_INFO, "Comm: JS booted ok!");
     LOCAL_js_booted = true;
  }
  
  if ( received_config ) 
  {
      main_reload_config();  
  }   
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static bool local_get_sent_index(DictionaryIterator *iter, uint8_t* index )
{
  Tuple* tuple_index = dict_find(iter, COMM_KEY_BUTTON_INDEX );
  
  if ( tuple_index == NULL )
  {
     APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success, but no tuple index!");   
     return false;
  }
  
  *index = tuple_index->value->uint8;
  return true; 
}

static void outbox_failed_callback(DictionaryIterator* iter, AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  uint8_t index;
  if ( local_get_sent_index( iter, &index ) == false)
     return;
  click_registry_send_clear( index, false );
}

static void outbox_sent_callback(DictionaryIterator* iter, void *context) 
{
  uint8_t index;
  if ( local_get_sent_index( iter, &index ) == false)
     return;
  
   click_registry_send_clear( index, true );
   
   if ( click_registry_send_has_any() == false )
      return;
   communication_request_for_send();
}

void communication_init()
{
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(64, 64);
}
