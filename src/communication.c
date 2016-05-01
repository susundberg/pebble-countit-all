#include <pebble.h>
#include "main.h"


#define CHECK_APP_MESSAGE(fun){\
   AppMessageResult __macro_res = fun;\
   if ( __macro_res != APP_MSG_OK ) \
   { \
      APP_LOG( APP_LOG_LEVEL_WARNING, "Communication failed: %d", (int)__macro_res); \
      return;\
   }};

void communication_send_datas()
{
   if ( click_registry_send_has_any() == false )
      return;

   if (connection_service_peek_pebblekit_connection() == false )
      return;
   
   APP_LOG( APP_LOG_LEVEL_INFO, "Communication ok, proceed with send" ); 
   
   DictionaryIterator* dict;
   
   CHECK_APP_MESSAGE( app_message_outbox_begin(&dict) );
   click_registry_send_write( dict );
   CHECK_APP_MESSAGE( app_message_outbox_send() );
}
   


static bool local_handle_icon(DictionaryIterator* iter, int loop )
{
     Tuple* tuple_icon = dict_find(iter, KEY_BUTTON1_ICON + loop );
     if ( tuple_icon != NULL )
     {
        config_set_icon( click_get_button_id_from_index( loop ), tuple_icon->value->int32 );
        return true;
     }
     return false;
}

static bool local_handle_type(DictionaryIterator* iter, int loop )
{
     Tuple* tuple_type = dict_find(iter, KEY_BUTTON1_TYPE + loop);
    
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
  bool received = false;
  for (int loop_button = 0; loop_button < 3; loop_button ++ )
  {
     
     received |= local_handle_icon( iter, loop_button );
     received |= local_handle_type( iter, loop_button );
  }
  
  
  if ( received ) 
  {
      main_reload_config();  
  }   
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  click_registry_send_clear( false );
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) 
{
   APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
   click_registry_send_clear( true );
}

void communication_init()
{
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(64, 64);
}
