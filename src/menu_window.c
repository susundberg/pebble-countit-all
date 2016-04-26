#include <pebble.h>
#include <assert.h>

#include "main.h"

#define MENU_ENTRY_SIZE 16
#define MENU_TITLE_MAX_SIZE 32


static SimpleMenuLayer*  LOCAL_layer_menu;;
static SimpleMenuSection LOCAL_menu_section;
static SimpleMenuItem    LOCAL_menu_items[HISTORY_SIZE];
char   LOCAL_menu_texts[ HISTORY_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_texts_small[ HISTORY_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_title[MENU_TITLE_MAX_SIZE];

static const char* LOCAL_menu_subtitle_delete = "DELETE"  ;
static const char* LOCAL_menu_title_none = " - ";

static void local_menu_callback(int index, void *ctx) 
{
  
  if ( LOCAL_menu_items[index].subtitle != LOCAL_menu_subtitle_delete )
  {
     LOCAL_menu_items[index].subtitle = LOCAL_menu_subtitle_delete ;
  }
  else
  {
     LOCAL_menu_items[index].subtitle = NULL;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(LOCAL_layer_menu));
}


static unsigned int LOCAL_data_index ;

void menu_window_set_data( unsigned int index )
{
   LOCAL_data_index = index;
}


void local_load_data( )
{
   uint8_t flags = config_get( click_get_button_id_from_index( LOCAL_data_index ));
   
   uint32_t nvalids = 0;
//    uint32_t total_duration = 0;
//    uint32_t total_interval = 0;
   memset( LOCAL_menu_items, 0x00, sizeof(LOCAL_menu_items));
   
   for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      const ButtonRegistryBuffer* buffer = click_registry_get_action( LOCAL_data_index, loop );
      if ( buffer == NULL )
      {
        LOCAL_menu_items[loop].title    = LOCAL_menu_title_none ;
        LOCAL_menu_items[loop].subtitle = NULL;
      }
      else
      {
         time_t time_seconds_epoc = buffer->time;
         struct tm* time_splitted = localtime(&time_seconds_epoc);
         char* string_title = LOCAL_menu_texts + MENU_ENTRY_SIZE*loop;
//          strftime( string_title , MENU_ENTRY_SIZE, "%m-%d %H:%M:%S", time_splitted );
         // 24.12. 12:32:12
         strftime( string_title , MENU_ENTRY_SIZE, "%m.%d. %H:%M:%S", time_splitted );
         LOCAL_menu_items[loop].title = string_title;
         
         nvalids += 1;
         LOCAL_menu_items[loop].callback = local_menu_callback;
         
         if ( flags & FLAG_LONG_ACTION_BUTTON ) 
         {
            char* subtitle = LOCAL_menu_texts_small + MENU_ENTRY_SIZE*loop;
            unsigned int hours;
            unsigned int minutes;
            unsigned int seconds = buffer->elapsed;
            get_time_splitted( &hours, &minutes, &seconds);
            snprintf( subtitle, MENU_ENTRY_SIZE, "D: %02d:%02d:%02d", hours, minutes, seconds );
            LOCAL_menu_items[loop].subtitle = subtitle;
         }
         
      }
      APP_LOG( APP_LOG_LEVEL_INFO, "Menu entry %d - %s", loop, LOCAL_menu_items[loop].title );
   }
   
   snprintf( LOCAL_menu_title, MENU_TITLE_MAX_SIZE, "Actions: %d", (int)nvalids );
}



void menu_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  LOCAL_menu_section.items = LOCAL_menu_items;
  LOCAL_menu_section.num_items = HISTORY_SIZE;
  LOCAL_menu_section.title = LOCAL_menu_title;
  local_load_data();
  
  LOCAL_layer_menu = simple_menu_layer_create(bounds, window, &LOCAL_menu_section, 1, NULL);
  
  layer_add_child(window_layer, simple_menu_layer_get_layer(LOCAL_layer_menu));
  
  
}

void menu_window_unload(Window *window) 
{
   simple_menu_layer_destroy( LOCAL_layer_menu );

   for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      if ( LOCAL_menu_items[loop].title == LOCAL_menu_title_none )
         break;
      
      if ( LOCAL_menu_items[loop].subtitle == LOCAL_menu_subtitle_delete )
      { // marked for deletion
         click_registry_clear( LOCAL_data_index, loop );
      }
   }
   click_registry_clear_finish( LOCAL_data_index );
}

