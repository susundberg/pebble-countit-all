#include <pebble.h>
// #include <assert.h>

#include "main.h"

#define MENU_ENTRY_SIZE 12
#define MENU_TITLE_MAX_SIZE 32


static SimpleMenuLayer*  LOCAL_layer_menu;;
static SimpleMenuSection LOCAL_menu_section;
static SimpleMenuItem    LOCAL_menu_items[BUFFER_SIZE];
char   LOCAL_menu_texts[ BUFFER_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_texts_small[ BUFFER_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_title[MENU_TITLE_MAX_SIZE];

static void local_menu_callback(int index, void *ctx) 
{
   
  LOCAL_menu_items[index].subtitle = "MARK";
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
   
   for (int loop = 0; loop < BUFFER_SIZE; loop ++ )
   {
      const ButtonRegistryBuffer* buffer = click_registry_get_action( LOCAL_data_index, loop );
      if ( buffer == NULL )
      {
        LOCAL_menu_items[loop].title    = " - ";
        LOCAL_menu_items[loop].subtitle = NULL;
      }
      else
      {
         time_t time_seconds_epoc = buffer->time;
         struct tm* time_splitted = localtime(&time_seconds_epoc);
         char* string_title = LOCAL_menu_texts + MENU_ENTRY_SIZE*loop;
         strftime( string_title , MENU_ENTRY_SIZE, "%m-%d %H:%M", time_splitted );
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
            int ret = snprintf( subtitle, MENU_ENTRY_SIZE, "D: %d:%d:%d", hours, minutes, seconds );
                     
            LOCAL_menu_items[loop].subtitle = subtitle;
//             total_duration += buffer->elapsed;
         }
         
      }
      APP_LOG( APP_LOG_LEVEL_INFO, "Menu entry %d - %s", loop, LOCAL_menu_items[loop].title );
   }
   
   int ret = snprintf( LOCAL_menu_title, MENU_TITLE_MAX_SIZE, "Actions: %d", (int)nvalids );
//    assert( ret > 0 && ret <= MENU_TITLE_MAX_SIZE );
}



void menu_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  LOCAL_menu_section.items = LOCAL_menu_items;
  LOCAL_menu_section.num_items = BUFFER_SIZE;
  LOCAL_menu_section.title = LOCAL_menu_title;
  local_load_data();
  
  LOCAL_layer_menu = simple_menu_layer_create(bounds, window, &LOCAL_menu_section, 1, NULL);
  
  layer_add_child(window_layer, simple_menu_layer_get_layer(LOCAL_layer_menu));
  
  
}

void menu_window_unload(Window *window) 
{
   simple_menu_layer_destroy( LOCAL_layer_menu );
}

