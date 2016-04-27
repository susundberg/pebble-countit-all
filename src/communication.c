#include <pebble.h>
#include "main.h"

static void inbox_received_callback(DictionaryIterator* iter, void* context) 
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");

  for (int loop_button = 0; loop_button < 3; loop_button ++ )
  {
     Tuple* tuple_icon = dict_find(iter, KEY_BUTTON1_ICON + loop_button );
     if ( tuple_icon != NULL )
     {
        config_set_icon( click_get_button_id_from_index( loop_button ), tuple_icon->value->int32 );
     }
     
     Tuple* tuple_type = dict_find(iter, KEY_BUTTON1_TYPE + loop_button);
     if ( tuple_type != NULL )
     {
        config_set_flags( click_get_button_id_from_index( loop_button ), tuple_icon->value->int32 );
     }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

// static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
//   APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
// }

void communication_init()
{
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
//   app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(64, 64);
}

void communication_send_click( char key, char type )
{
  // Send app message
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, COMMUNICATION_KEY_BUTTON, key );
  dict_write_uint8(iter, COMMUNICATION_KEY_TYPE  , type );
  app_message_outbox_send();
}
