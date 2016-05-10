#include "main.h"


#define TEXT_HISTORY_SIZE 32
#define N_LAYERS 3


typedef struct
{
   TextLayer* layer;
   char       buffer[TEXT_HISTORY_SIZE];
} BufferedTextLayer;


static BufferedTextLayer LOCAL_layers[N_LAYERS];
static BufferedTextLayer LOCAL_layers_small[N_LAYERS];

static ActionBarLayer* LOCAL_action_bar ;
static StatusBarLayer* LOCAL_status_bar;

static GBitmap* LOCAL_icon[3] = { NULL, NULL, NULL };

static void local_set_layer_color_inverted( unsigned int index, bool inverted )
{
  GColor front = GColorWhite;
  GColor back  = GColorBlack;
  
  if  ( inverted == false )
  {
     back  = GColorWhite;
     front = GColorBlack;
  }

  text_layer_set_background_color( LOCAL_layers[index].layer, back );
  text_layer_set_text_color( LOCAL_layers[index].layer , front );
  text_layer_set_background_color( LOCAL_layers_small[index].layer, back );
  text_layer_set_text_color( LOCAL_layers_small[index].layer , front );
}

static void local_create_layer( Layer* window_layer, GRect* window_bounds, unsigned int index  )
{
  unsigned int layer_size   = 52;
  unsigned int layer_small  = 24;
  unsigned int layer_padding = 0;
  unsigned int offset_h   = STATUS_BAR_LAYER_HEIGHT + index * layer_size;
  
  offset_h = offset_h + index*layer_padding;
  
  TextLayer* tlayer = text_layer_create(GRect(0, offset_h, window_bounds->size.w - ACTION_BAR_WIDTH, layer_size - layer_small));
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));  
  text_layer_set_text_alignment( tlayer, GTextAlignmentCenter );
  LOCAL_layers[ index ].layer = tlayer; 
  layer_add_child(window_layer, text_layer_get_layer(tlayer ));
  
  tlayer = text_layer_create(GRect(0, offset_h + (layer_size  - layer_small) , window_bounds->size.w - ACTION_BAR_WIDTH, layer_small ));
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_GOTHIC_18));
  LOCAL_layers_small[ index ].layer = tlayer; 
  layer_add_child(window_layer, text_layer_get_layer(tlayer ));
  text_layer_set_text_alignment( tlayer, GTextAlignmentCenter );  
  local_set_layer_color_inverted( index, true );
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


 

#define CHECK_BUFFER_PRINT( fun ) \
{ \
   int __macro_ret = fun;\
   if ( __macro_ret < 0 || __macro_ret >= TEXT_HISTORY_SIZE )\
   {\
      APP_LOG( APP_LOG_LEVEL_ERROR, "Too long print %s:%d -- %d", __FILE__, __LINE__, __macro_ret );\
   }\
}


void local_show_running_time( BufferedTextLayer* blayer, unsigned int sec  )
{
  unsigned int min;
  unsigned int hour;
  get_time_splitted( &hour, &min, &sec ); 
  
  CHECK_BUFFER_PRINT( snprintf( blayer->buffer, TEXT_HISTORY_SIZE, "%02u:%02u:%02u", hour, min, sec) );
  
  text_layer_set_text( blayer->layer, blayer->buffer );
  layer_mark_dirty(text_layer_get_layer(blayer->layer));
}


void local_show_datetime( BufferedTextLayer* blayer, const char* prefix, time_t time_was, const char* postfix )
{
   struct tm* time_splitted = localtime(&time_was);
   char tmp_buffer[TEXT_HISTORY_SIZE];
   CHECK_BUFFER_PRINT( strftime( tmp_buffer, TEXT_HISTORY_SIZE, "%H:%M", time_splitted ) );
   CHECK_BUFFER_PRINT( snprintf(blayer->buffer, TEXT_HISTORY_SIZE, "%s%s%s", prefix, tmp_buffer,postfix) );
   
   text_layer_set_text( blayer->layer, blayer->buffer );
   layer_mark_dirty(text_layer_get_layer(blayer->layer));
}


void main_window_reload_config()
{
  for ( int loop = 0; loop < N_LAYERS; loop ++ )
  {
    if ( LOCAL_icon[loop] != NULL )
    {
      gbitmap_destroy( LOCAL_icon[loop] );  
      LOCAL_icon[loop] = NULL;
    }   
    
    if ( click_registry_enabled( loop ) == true ) 
    {
       LOCAL_icon[loop] = gbitmap_create_with_resource( config_get_icon( click_get_button_id_from_index( loop ) ) );
       action_bar_layer_set_icon( LOCAL_action_bar, click_get_button_id_from_index( loop ), LOCAL_icon[loop] );  
    }
  }
  action_bar_layer_set_click_config_provider(LOCAL_action_bar,click_config_provider);
}

void main_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  window_set_background_color(window,  GColorBlack );
  
  LOCAL_action_bar = action_bar_layer_create();
  
  
  for ( int loop = 0; loop < N_LAYERS; loop ++ )
  {
     local_create_layer( window_layer, &window_bounds, loop );
  }
  action_bar_layer_add_to_window(LOCAL_action_bar, window);

  // then add status bar  
  LOCAL_status_bar = status_bar_layer_create();
  int16_t width = layer_get_bounds(window_layer).size.w - ACTION_BAR_WIDTH;
  GRect frame = GRect(0, 0, width, STATUS_BAR_LAYER_HEIGHT);
  layer_set_frame(status_bar_layer_get_layer(LOCAL_status_bar), frame);
  layer_add_child(window_layer, status_bar_layer_get_layer(LOCAL_status_bar));  
   
  // Update all
  main_window_reload_config();
  main_window_update_elapsed( time(NULL) );  
}

void main_window_unload(Window *window) 
{
  // Destroy TextLayer
  for ( int loop = 0; loop < N_LAYERS ; loop ++ )
  {
     text_layer_destroy(LOCAL_layers[loop].layer);
     text_layer_destroy(LOCAL_layers_small[loop].layer);
     
     if ( LOCAL_icon[loop] != NULL )
     {
        gbitmap_destroy( LOCAL_icon[loop] );  
        LOCAL_icon[loop] = NULL;
     }   
  }
  
  action_bar_layer_destroy( LOCAL_action_bar );
  status_bar_layer_destroy( LOCAL_status_bar );
}




void local_show_verbose_running_time( BufferedTextLayer* blayer, const char* prefix, unsigned int time_since, const char* postfix )
{
  unsigned int min;
  unsigned int hour;
  get_time_splitted( &hour, &min, &time_since); 
  
  if ( hour == 0 )
  {
     CHECK_BUFFER_PRINT( snprintf( blayer->buffer, TEXT_HISTORY_SIZE, "%dmin ago", min ) );
  }
  else
  {
     CHECK_BUFFER_PRINT( snprintf( blayer->buffer, TEXT_HISTORY_SIZE, "%dh %dmin ago", hour, min ) );
  }

  text_layer_set_text( blayer->layer, blayer->buffer );
  layer_mark_dirty(text_layer_get_layer(blayer->layer));
}

void main_window_update_elapsed( time_t time_now )
{
   
   for ( int loop = 0 ; loop < N_LAYERS ; loop ++ )
   {
      unsigned int elapsed_sec;
      unsigned int time_started;
      if ( click_registry_enabled( loop ) == false )
      {
         text_layer_set_text( LOCAL_layers[loop].layer, "" );
         text_layer_set_text( LOCAL_layers_small[loop].layer, ""  );
      }
      else if ( click_registry_get_elapsed( time_now, loop, &elapsed_sec, &time_started ) == true )
      {
         local_show_running_time( &LOCAL_layers[loop], elapsed_sec );
         local_show_datetime(  &LOCAL_layers_small[loop], "Started at ", time_started, "" );
      }
      else if ( click_registry_get_since_last( time_now, loop, &elapsed_sec, &time_started ) == true ) 
      {
         local_show_datetime(  &LOCAL_layers[loop], "", time_started, "" );
         local_show_verbose_running_time(  &LOCAL_layers_small[loop], "Last ", elapsed_sec, " ago" );
      }
      else if ( click_registry_enabled( loop ) == true )
      {
         text_layer_set_text( LOCAL_layers[loop].layer, "   -   " );
      }
   }
}