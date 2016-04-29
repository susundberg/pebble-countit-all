#include <pebble.h>
#include <math.h>

#include "main.h"

#define MENU_ENTRY_SIZE 16
#define MENU_TITLE_MAX_SIZE 16
#define LOCAL_MENU_SECTIONS_N 2
#define MENU_ACTIONS_SIZE 3 // statistics: avg duration, statistics: avg interval, clear all
static SimpleMenuLayer*  LOCAL_layer_menu;
static SimpleMenuSection LOCAL_menu_section[LOCAL_MENU_SECTIONS_N];
static SimpleMenuItem    LOCAL_menu_hist_items[HISTORY_SIZE];
static SimpleMenuItem    LOCAL_menu_action_items[MENU_ACTIONS_SIZE];

char   LOCAL_menu_hist_texts[ HISTORY_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_hist_texts_small[ HISTORY_SIZE * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_title[MENU_TITLE_MAX_SIZE];

static const char* LOCAL_menu_subtitle_delete = "DELETE"  ;
static const char* LOCAL_menu_title_none = " - ";
static const char* LOCAL_menu_title_clearall = "Delete all";
static const char* LOCAL_menu_title_unclearall = "Dont delete!";

#define MENU_ENTRY_SIZE_LARGE 32
char   LOCAL_menu_action_texts[ 2 * MENU_ENTRY_SIZE]; 
char   LOCAL_menu_action_texts_small[ 2 * MENU_ENTRY_SIZE_LARGE]; 


#define CHECK_BUFFER_N_PRINT( count, fun ) {\
   int __macro_ret = fun;\
   if ( __macro_ret < 0 || __macro_ret >= count )\
   {\
      APP_LOG( APP_LOG_LEVEL_ERROR, "Too long print %s:%d -- %d", __FILE__, __LINE__, __macro_ret );\
   }}\

#define CHECK_BUFFER_PRINT( fun ) CHECK_BUFFER_N_PRINT( MENU_ENTRY_SIZE, fun );





static void local_set_normal_subtitle( int index )
{
   if ( LOCAL_menu_hist_texts_small[index*MENU_ENTRY_SIZE] == 0 )
   {
      LOCAL_menu_hist_items[index].subtitle = NULL;
   }
   else
   {   
      LOCAL_menu_hist_items[index].subtitle = LOCAL_menu_hist_texts_small + index*MENU_ENTRY_SIZE;
   }  
}
static void local_menu_clearall_callback( int index, void *ctx)
{
  if ( LOCAL_menu_action_items[0].title == LOCAL_menu_title_clearall )
  {
      for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
      {
         if ( LOCAL_menu_hist_items[loop].title != LOCAL_menu_title_none )
            LOCAL_menu_hist_items[loop].subtitle = LOCAL_menu_subtitle_delete ;
      }
      
      LOCAL_menu_action_items[0].title    = LOCAL_menu_title_unclearall;
      LOCAL_menu_action_items[0].subtitle = NULL;
  }
  else
  {
      for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
      {
         local_set_normal_subtitle( loop );
      }
      LOCAL_menu_action_items[0].title    = LOCAL_menu_title_clearall;
      LOCAL_menu_action_items[0].subtitle = NULL;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(LOCAL_layer_menu));
}

static void local_menu_hist_callback(int index, void *ctx) 
{
  
  if ( LOCAL_menu_hist_items[index].subtitle != LOCAL_menu_subtitle_delete )
  {
     LOCAL_menu_hist_items[index].subtitle = LOCAL_menu_subtitle_delete ;
  }
  else
  {
     local_set_normal_subtitle(index);
  }
  layer_mark_dirty(simple_menu_layer_get_layer(LOCAL_layer_menu));
}


static unsigned int LOCAL_data_index ;

void menu_window_set_data( unsigned int index )
{
   LOCAL_data_index = index;
}

typedef struct
{
   uint64_t sum_x;
   uint64_t sum_x2; // x^2
} Calculateor_AvgStd;


uint32_t isqrt_iterative(uint64_t const n)
{
    uint64_t xk = n;
    if (n == 0) return 0;
    if (n == 18446744073709551615ULL) return 4294967295U;
    do
    {
        uint64_t const xk1 = (xk + n / xk) / 2;
        if (xk1 >= xk)
        {
            return xk;
        }
        else
        {
            xk = xk1;
        }
    } while (1);
}


static void local_calculator_register_value( Calculateor_AvgStd* calc, uint32_t value )
{
   calc->sum_x  += value;
   calc->sum_x2 += value*value;
}

static float local_calculator_get_avg_value( const Calculateor_AvgStd* calc, uint32_t count )
{
   return (float)calc->sum_x / (float)count;
}

static float local_calculator_get_std_value( const Calculateor_AvgStd* calc, uint32_t count )
{
   uint64_t upper = count * calc->sum_x2 - calc->sum_x*calc->sum_x;
   uint64_t lower = count * (count - 1);
   if ( count <= 1 )
      return 0.0f;
   uint32_t isqrt = isqrt_iterative( upper/ lower );
   return (float) isqrt;
}


static void local_print_buffer_avg_std( const Calculateor_AvgStd* calc, uint32_t count, char* buffer )
{
   unsigned int avg_value_s = (unsigned int)(local_calculator_get_avg_value( calc, count ) + 0.5f);
   unsigned int std_value_s = (unsigned int)(local_calculator_get_std_value( calc, count ) + 0.5f);
   
   unsigned int avg_value_m;
   unsigned int std_value_m;
   
   get_time_splitted( NULL, &avg_value_m, &avg_value_s );
   get_time_splitted( NULL, &std_value_m, &std_value_s );
   
   APP_LOG( APP_LOG_LEVEL_INFO, "avg %02d:%02d std %02d:%02d",  avg_value_m,avg_value_s, std_value_m,std_value_s  );
   
   // avg 99:99 std 12:12
   CHECK_BUFFER_N_PRINT( MENU_ENTRY_SIZE_LARGE, snprintf( buffer, MENU_ENTRY_SIZE_LARGE, "avg %02d:%02d std %02d:%02d", 
                                                          avg_value_m,avg_value_s, 
                                                          std_value_m,std_value_s ) );
}


static void local_load_data( )
{
   uint8_t flags = config_get( click_get_button_id_from_index( LOCAL_data_index ));
   
   uint32_t nvalids = 0;
   memset( LOCAL_menu_hist_items, 0x00, sizeof(LOCAL_menu_hist_items));
   memset( LOCAL_menu_hist_texts_small, 0x00, sizeof(LOCAL_menu_hist_texts_small));
   memset( LOCAL_menu_hist_texts, 0x00, sizeof(LOCAL_menu_hist_texts));
   memset( LOCAL_menu_action_texts_small, 0x00, sizeof(LOCAL_menu_action_texts_small));
   memset( LOCAL_menu_action_texts, 0x00, sizeof(LOCAL_menu_action_texts));
   
   Calculateor_AvgStd intervals;
   Calculateor_AvgStd durations;
   memset( &intervals, 0x00, sizeof(intervals));
   memset( &durations, 0x00, sizeof(durations));
   
   uint32_t time_seconds_epoc_last = 0;
   
   for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      const ButtonRegistryBuffer* buffer = click_registry_get_action( LOCAL_data_index, loop );
      if ( buffer == NULL )
      {
        LOCAL_menu_hist_items[loop].title    = LOCAL_menu_title_none ;
        LOCAL_menu_hist_items[loop].subtitle = NULL;
      }
      else
      {
         time_t time_seconds_epoc = buffer->time;
         struct tm* time_splitted = localtime(&time_seconds_epoc);
         char* string_title = LOCAL_menu_hist_texts + MENU_ENTRY_SIZE*loop;
         // 24.12. 12:32:12
         CHECK_BUFFER_PRINT( strftime( string_title , MENU_ENTRY_SIZE, "%m.%d. %H:%M:%S", time_splitted ) );
         LOCAL_menu_hist_items[loop].title = string_title;
         
         // items are in order of loop=0 newest = largest
         if ( loop >= 1 )
         {
            uint32_t loop_interval = time_seconds_epoc_last - time_seconds_epoc;
            local_calculator_register_value( &intervals, loop_interval );
         } 
         
         time_seconds_epoc_last = time_seconds_epoc;   
         nvalids += 1;
         LOCAL_menu_hist_items[loop].callback = local_menu_hist_callback;
         
         if ( flags & BUTTONTYPE_FLAG_DURATION ) 
         {
            char* subtitle = LOCAL_menu_hist_texts_small + MENU_ENTRY_SIZE*loop;
            unsigned int hours;
            unsigned int minutes;
            unsigned int seconds = BUFFER_HISTORY_ELAPSED( buffer->flags_n_elapsed );
            
            local_calculator_register_value( &durations, seconds );
            get_time_splitted( &hours, &minutes, &seconds);
            CHECK_BUFFER_PRINT( snprintf( subtitle, MENU_ENTRY_SIZE, "D: %02d:%02d:%02d", hours, minutes, seconds ) );
            LOCAL_menu_hist_items[loop].subtitle = subtitle;
         }
      }
      APP_LOG( APP_LOG_LEVEL_INFO, "Menu entry %d - %s", loop, LOCAL_menu_hist_items[loop].title );
   }
   
   CHECK_BUFFER_PRINT( snprintf( LOCAL_menu_title, MENU_TITLE_MAX_SIZE, "History: %d", (int)nvalids ) );
   
   
   if ( nvalids > 1 )
   {
      CHECK_BUFFER_PRINT( snprintf( LOCAL_menu_action_texts, MENU_ENTRY_SIZE, "Interval (min)"));
      local_print_buffer_avg_std( &intervals, nvalids - 1, LOCAL_menu_action_texts_small );
   }
   else
   {
      CHECK_BUFFER_PRINT( snprintf( LOCAL_menu_action_texts,MENU_ENTRY_SIZE, "Interval: N/A" ) );
      LOCAL_menu_action_texts_small[0] = 0;
   }
   
    if ( ( flags & BUTTONTYPE_FLAG_DURATION ) && nvalids > 0 )
    {
      CHECK_BUFFER_PRINT( snprintf( LOCAL_menu_action_texts + MENU_ENTRY_SIZE, MENU_ENTRY_SIZE, "Duration (min)"));
      local_print_buffer_avg_std( &durations, nvalids, LOCAL_menu_action_texts_small + MENU_ENTRY_SIZE_LARGE );
    }
    else
    {
       CHECK_BUFFER_PRINT( snprintf( LOCAL_menu_action_texts + MENU_ENTRY_SIZE, MENU_ENTRY_SIZE, "Duration: N/A" ) );
       LOCAL_menu_action_texts_small[MENU_ENTRY_SIZE_LARGE] = 0;
    }
}



void menu_window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  memset( LOCAL_menu_action_items, 0x00, sizeof(LOCAL_menu_action_items));

  
  uint8_t flags = config_get( click_get_button_id_from_index( LOCAL_data_index ));
  
  LOCAL_menu_section[0].items = LOCAL_menu_action_items;
  LOCAL_menu_section[0].num_items = (( flags & BUTTONTYPE_FLAG_DURATION ) != 0x00) ? (MENU_ACTIONS_SIZE) : (MENU_ACTIONS_SIZE - 1);
  LOCAL_menu_section[0].title = "Actions";
  
  LOCAL_menu_action_items[0].callback = local_menu_clearall_callback;
  LOCAL_menu_action_items[0].title    = LOCAL_menu_title_clearall;
  for ( int loop = 0; loop < 2;loop ++ )
  {
     LOCAL_menu_action_items[loop + 1].title    = LOCAL_menu_action_texts + loop * MENU_ENTRY_SIZE ;
     LOCAL_menu_action_items[loop + 1].subtitle = LOCAL_menu_action_texts_small + loop * MENU_ENTRY_SIZE_LARGE;
  }

  
  LOCAL_menu_section[1].items = LOCAL_menu_hist_items;
  LOCAL_menu_section[1].num_items = HISTORY_SIZE;
  LOCAL_menu_section[1].title = LOCAL_menu_title;
  local_load_data();
  
  LOCAL_layer_menu = simple_menu_layer_create(bounds, window, LOCAL_menu_section, LOCAL_MENU_SECTIONS_N, NULL);
  
  layer_add_child(window_layer, simple_menu_layer_get_layer(LOCAL_layer_menu));
  
  
}

void menu_window_unload(Window *window) 
{
   simple_menu_layer_destroy( LOCAL_layer_menu );

   for (int loop = 0; loop < HISTORY_SIZE; loop ++ )
   {
      if ( LOCAL_menu_hist_items[loop].title == LOCAL_menu_title_none )
         break;
      
      if ( LOCAL_menu_hist_items[loop].subtitle == LOCAL_menu_subtitle_delete )
      { // marked for deletion
         click_registry_clear( LOCAL_data_index, loop );
      }
   }
   click_registry_clear_finish( LOCAL_data_index );
}

