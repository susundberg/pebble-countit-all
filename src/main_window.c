#include "main.h"


#define BUFFER_SIZE 16 
#define N_LAYERS 3

static TextLayer* LOCAL_layers[N_LAYERS];
char LOCAL_layers_text[N_LAYERS][BUFFER_SIZE];


static void local_create_layer( Layer* window_layer, GRect* window_bounds, unsigned int index  )
{
  unsigned int layer_size = (window_bounds->size.h)/3;
  
  unsigned int offset_h =  index * layer_size ;
  TextLayer* tlayer = text_layer_create(GRect(0, offset_h, window_bounds->size.w, layer_size ));
  
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(tlayer ));
  LOCAL_layers[ index ] = tlayer; 
}


static void local_get_time_since( unsigned int* min, unsigned* sec )
{
  *min = (*sec)/60;
  *sec = (*sec) - (*min)*60;
  if ( *min > 99 )
  {
    *min = 99;
    *sec = 99;
  }
}

void local_set_text( unsigned int id, const char* text, unsigned int len )
{
  if ( len >= BUFFER_SIZE )
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
  local_get_time_since( &min, &sec );
  int ret = snprintf( buffer, BUFFER_SIZE, "%s %02u:%02u", desc, min, sec);
  
  if (ret + 1 > BUFFER_SIZE)
  {
     APP_LOG( APP_LOG_LEVEL_ERROR, "Too long string to print %d - %d", loop, ret );
     return;
  }
  text_layer_set_text( LOCAL_layers[loop], buffer );
}

void main_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  for ( int loop = 0; loop < N_LAYERS; loop ++ )
  {
     local_create_layer( window_layer, &window_bounds, loop );
  }
   
}

void main_window_unload(Window *window) 
{
  // Destroy TextLayer
  for ( int loop = 0; loop < N_LAYERS ; loop ++ )
     text_layer_destroy(LOCAL_layers[loop]);
}



void main_window_update_elapsed( time_t time_now )
{
   
   for ( int loop = 0 ; loop < N_LAYERS ; loop ++ )
   {
      unsigned int show_sec;

      if ( click_registry_get_elapsed( time_now, loop, &show_sec ) == true )
      {
         local_set_time_text( loop, "D", show_sec );
      }
      else if ( click_registry_get_since_last( time_now, loop, &show_sec ) == true ) 
      {
         local_set_time_text( loop, "L", show_sec );
      }
      else
      {
         local_set_text( loop, "N", 4 );
      }
   }
}