/**
 * Main file for the pebble app -- linux remote.
 * 
 * @author Pauli Salmenrinne, 2015
 */

#include <pebble.h>
#include "main.h"


static Window *LOCAL_window_main;
static Window *LOCAL_window_menu;


static void local_time_event_handler(struct tm *tick_time, TimeUnits units_changed)
{
   time_t time_now = time( NULL );
   main_window_update_elapsed( time_now );
   
   if ( time_now >= communication_next_attempt() )
      communication_send_datas( time_now );
   
}


void main_reload_config()
{
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading config .. ");
   click_registry_init();
   main_window_reload_config();
}

static void local_init() 
{
  APP_LOG( APP_LOG_LEVEL_INFO, "Initializing .. " );
  click_registry_init();
  communication_init();
  
  tick_timer_service_subscribe( SECOND_UNIT , local_time_event_handler );
  LOCAL_window_main = window_create();
  window_set_window_handlers(LOCAL_window_main, (WindowHandlers) {
    .load   = main_window_load,
    .unload = main_window_unload
    });
  LOCAL_window_menu = window_create();
  window_set_window_handlers(LOCAL_window_menu, (WindowHandlers) {
    .load   = menu_window_load,
    .unload = menu_window_unload
    });
  
  window_stack_push(LOCAL_window_main, true);
}

void main_show_menu_window( unsigned int index )
{
   APP_LOG( APP_LOG_LEVEL_INFO, "SHOW menu for %d", index );
   menu_window_set_data( index );
   window_stack_push(LOCAL_window_menu, true);
}

static void local_deinit() 
{
  window_destroy(LOCAL_window_main);
  tick_timer_service_unsubscribe();
}

int main(void) 
{
  APP_LOG( APP_LOG_LEVEL_INFO, "Initializing .. " );
  local_init();
  APP_LOG( APP_LOG_LEVEL_INFO, "Entering main loop" );
  app_event_loop();
  local_deinit();
}


