#include "main.h"


#define TEXT_BUFFER_SIZE 32
#define N_LAYERS 3


typedef struct
{
   TextLayer* layer;
   char       buffer[TEXT_BUFFER_SIZE];
} BufferedTextLayer;


static BufferedTextLayer LOCAL_layers[N_LAYERS];
static BufferedTextLayer LOCAL_layers_small[N_LAYERS];

static ActionBarLayer* LOCAL_action_bar ;
static StatusBarLayer* LOCAL_status_bar;

static GBitmap* LOCAL_icon[3];
static const uint32_t RESOURCE_ICONS[] = { RESOURCE_ID_ACTION_BOTTLE, RESOURCE_ID_ACTION_DIAPER, RESOURCE_ID_ACTION_MOON };

static void local_create_layer( Layer* window_layer, GRect* window_bounds, unsigned int index  )
{
  unsigned int layer_size   = 42;
  unsigned int layer_small  = 10;
  unsigned int layer_padding = 4;
  unsigned int offset_h   = STATUS_BAR_LAYER_HEIGHT + index * layer_size;
  
  offset_h = offset_h + index*layer_padding;
  
  TextLayer* tlayer = text_layer_create(GRect(0, offset_h, window_bounds->size.w - ACTION_BAR_WIDTH, layer_size ));
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));

  layer_add_child(window_layer, text_layer_get_layer(tlayer ));
  text_layer_set_text_alignment( tlayer, GTextAlignmentRight );
  LOCAL_layers[ index ].layer = tlayer; 

  tlayer = text_layer_create(GRect(0, offset_h + (layer_size  - layer_small) , window_bounds->size.w - ACTION_BAR_WIDTH, layer_small ));
  text_layer_set_font( tlayer , fonts_get_system_font(FONT_KEY_GOTHIC_18));
  LOCAL_layers_small[ index ].layer = tlayer; 
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
   if ( __macro_ret < 0 || __macro_ret >= TEXT_BUFFER_SIZE )\
   {\
      APP_LOG( APP_LOG_LEVEL_ERROR, "Too long print %s:%d -- %d", __FILE__, __LINE__, __macro_ret );\
   }\
}


void local_show_running_time( BufferedTextLayer* blayer, unsigned int sec  )
{
  unsigned int min;
  get_time_splitted( NULL, &min, &sec ); 
  
  CHECK_BUFFER_PRINT( snprintf( blayer->buffer, TEXT_BUFFER_SIZE, "%02u:%02u", min, sec) );
  text_layer_set_text( blayer->layer, blayer->buffer );
  layer_mark_dirty(text_layer_get_layer(blayer->layer));
}


void local_show_datetime( BufferedTextLayer* blayer, const char* prefix, time_t time_was, const char* postfix )
{
   struct tm* time_splitted = localtime(&time_was);
   char tmp_buffer[TEXT_BUFFER_SIZE];
   CHECK_BUFFER_PRINT( strftime( tmp_buffer, TEXT_BUFFER_SIZE, "%H:%M", time_splitted ) );
   CHECK_BUFFER_PRINT( snprintf(blayer->buffer, TEXT_BUFFER_SIZE, "%s%s%s", prefix, tmp_buffer,postfix) );
   
   text_layer_set_text( blayer->layer, blayer->buffer );
   layer_mark_dirty(text_layer_get_layer(blayer->layer));
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
  {
     text_layer_destroy(LOCAL_layers[loop].layer);
     text_layer_destroy(LOCAL_layers_small[loop].layer);
  }
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

  text_layer_set_background_color( LOCAL_layers[index].layer, back );
  text_layer_set_text_color( LOCAL_layers[index].layer , front );
}

void main_window_update_elapsed( time_t time_now )
{
   
   for ( int loop = 0 ; loop < N_LAYERS ; loop ++ )
   {
      unsigned int show_sec;
      unsigned int time_started;
      if ( click_registry_get_elapsed( time_now, loop, &show_sec, &time_started ) == true )
      {
         local_set_layer_color_inverted(loop, true);
         local_show_running_time( &LOCAL_layers[loop], show_sec );
         local_show_datetime(  &LOCAL_layers_small[loop], "Started at ", time_started, "" );
      }
      else if ( click_registry_get_since_last( time_now, loop, &show_sec, &time_started ) == true ) 
      {
         local_set_layer_color_inverted(loop, false);
         local_show_datetime(  &LOCAL_layers[loop], "", time_started, "" );
         local_show_verbose_running_time(  &LOCAL_layers_small[loop], "Last ", time_started, " ago" );
      }
      else if ( click_registry_enabled( loop ) == true )
      {
         local_set_layer_color_inverted(loop, false);  
         text_layer_set_text( LOCAL_layers[loop].layer, "   -   " );
      }
      else
      {
         local_set_layer_color_inverted(loop, false);
      }
   }
}