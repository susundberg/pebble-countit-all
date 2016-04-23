#include "main.h"


#define TEXT_BUFFER_SIZE 16
#define N_LAYERS 3

static TextLayer* LOCAL_layers[N_LAYERS];
char LOCAL_layers_text[N_LAYERS][TEXT_BUFFER_SIZE];
static ActionBarLayer* LOCAL_action_bar ;
static StatusBarLayer* LOCAL_status_bar;

static GBitmap* LOCAL_icon[3];
static const uint32_t RESOURCE_ICONS[] = { RESOURCE_ID_ACTION_BOTTLE, RESOURCE_ID_ACTION_DIAPER, RESOURCE_ID_ACTION_MOON };

static void local_create_layer( Layer* window_layer, GRect* window_bounds, unsigned int index  )
{
  unsigned int layer_size   = 42;
  unsigned int layer_padding = 4;
  unsigned int offset_h   = STATUS_BAR_LAYER_HEIGHT + index * layer_size;
  
  offset_h = offset_h + index*layer_padding;
  
  TextLayer* tlayer = text_layer_create(GRect(0, offset_h, window_bounds->size.w - ACTION_BAR_WIDTH, layer_size ));
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
  
  layer_add_child(window_layer, text_layer_get_layer(tlayer ));
  text_layer_set_text_alignment( tlayer, GTextAlignmentRight );
  
  LOCAL_layers[ index ] = tlayer; 
}


void get_time_splitted( unsigned int* hour, unsigned int* min, unsigned* sec )
{
  *min = (*sec)/60;
  *sec = (*sec) - (*min)*60;
  
  if ( hour != NULL )
  {
   *hour = (*min)/60;
   *min = (*min) - (*hour)*60;
   if ( *hour > 99 )
   {
      *hour = 99;
      *min  = 99;
      *sec  = 99;
   }
  }
  else
  {
     if (*min > 99 )
     {
        *min = 99;
        *sec = 99;
     }
  }
}


void local_set_text( unsigned int id, const char* text, unsigned int len )
{
  if ( len >= TEXT_BUFFER_SIZE )
  {
     APP_LOG( APP_LOG_LEVEL_ERROR, "Invalid update" );
     return;
  }
  memcpy( LOCAL_layers_text[id], text, len );
  text_layer_set_text( LOCAL_layers[id], LOCAL_layers_text[id] );
}
 

 
void local_set_time_text( unsigned int loop, const char* desc, unsigned int sec)
{
  char* buffer = LOCAL_layers_text[loop];
  unsigned int min;
//   unsigned int hour;
  get_time_splitted( NULL, &min, &sec ); // TODO CONFIG
  int ret = snprintf( buffer, TEXT_BUFFER_SIZE, "%02u:%02u", min, sec);
  
  if (ret + 1 > TEXT_BUFFER_SIZE)
  {
     APP_LOG( APP_LOG_LEVEL_ERROR, "Too long string to print %d - %d", loop, ret );
     return;
  }
  text_layer_set_text( LOCAL_layers[loop], buffer );
  layer_mark_dirty(text_layer_get_layer(LOCAL_layers[loop]));
}

void main_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  window_set_background_color(window,  GColorBlack );
  
  LOCAL_action_bar = action_bar_layer_create();
  
  action_bar_layer_set_click_config_provider(LOCAL_action_bar,click_config_provider);
  for ( int loop = 0; loop < N_LAYERS; loop ++ )
  {
     local_create_layer( window_layer, &window_bounds, loop );
     
     if ( click_registry_enabled( loop ) == true ) 
     {
        LOCAL_icon[loop] = gbitmap_create_with_resource( RESOURCE_ICONS[ loop ] );
        action_bar_layer_set_icon( LOCAL_action_bar, click_get_button_id_from_index( loop ), LOCAL_icon[loop] );  
     }
  }
  action_bar_layer_add_to_window(LOCAL_action_bar, window);

  // then add status baar  
  LOCAL_status_bar = status_bar_layer_create();
  int16_t width = layer_get_bounds(window_layer).size.w - ACTION_BAR_WIDTH;
  GRect frame = GRect(0, 0, width, STATUS_BAR_LAYER_HEIGHT);
  layer_set_frame(status_bar_layer_get_layer(LOCAL_status_bar), frame);
  layer_add_child(window_layer, status_bar_layer_get_layer(LOCAL_status_bar));  
   
  // Update all
  
   main_window_update_elapsed( time(NULL) );  
}

void main_window_unload(Window *window) 
{
  // Destroy TextLayer
  for ( int loop = 0; loop < N_LAYERS ; loop ++ )
     text_layer_destroy(LOCAL_layers[loop]);
  action_bar_layer_destroy( LOCAL_action_bar );
  status_bar_layer_destroy( LOCAL_status_bar );
}



static void local_set_layer_color_inverted( unsigned int index, bool inverted )
{
  GColor front = GColorWhite;
  GColor back  = GColorBlack;
  
  if  ( inverted == true )
  {
     back  = GColorWhite;
     front = GColorBlack;
  }

  text_layer_set_background_color( LOCAL_layers[index], back );
  text_layer_set_text_color( LOCAL_layers[index] , front );

   
}

void main_window_update_elapsed( time_t time_now )
{
   
   for ( int loop = 0 ; loop < N_LAYERS ; loop ++ )
   {
      unsigned int show_sec;

      if ( click_registry_get_elapsed( time_now, loop, &show_sec ) == true )
      {

         local_set_layer_color_inverted(loop, true);
         local_set_time_text( loop, "D", show_sec );
      }
      else if ( click_registry_get_since_last( time_now, loop, &show_sec ) == true ) 
      {
         local_set_layer_color_inverted(loop, false);
         local_set_time_text( loop, "L", show_sec );
      }
      else if ( click_registry_enabled( loop ) == true )
      {
         local_set_layer_color_inverted(loop, false);  
         local_set_text( loop, "   -   ", 7 );
      }
      else
      {
         local_set_layer_color_inverted(loop, false);
      }
   }
}