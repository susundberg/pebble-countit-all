#ifndef MAIN_H
#define MAIN_H

#include <pebble.h>

#define BUTTONTYPE_FLAG_SINGLE     0x01
#define BUTTONTYPE_FLAG_DURATION   0x02
#define BUTTONTYPE_FLAG_MASK       0x03


#define BUFFER_FLAG_MASK        0xF0000000 
#define BUFFER_FLAG_NOT_SENT    0x10000000  // This value has not been sent to phone
#define BUFFER_FLAG_RUNNING     0x20000000  // This entry is duration that has been started but not ended

#define BUFFER_HISTORY_ELAPSED(x) ((x)&(~BUFFER_FLAG_MASK))

#define BUTTON_N_INDEX 3
#define HISTORY_SIZE   30


#define PERSISTANT_STORAGE_DATA_START   1000
#define PERSISTANT_STORAGE_CONFIG_START 100


#define KEY_BUTTON1_ICON 100
// #define KEY_BUTTON2_ICON 101 (done dynamically with loop)
// #define KEY_BUTTON3_ICON 102 (done dynamically with loop)

#define KEY_BUTTON1_TYPE 200
// #define KEY_BUTTON2_TYPE 201 (done dynamically with loop)
// #define KEY_BUTTON3_TYPE 202 (done dynamically with loop)
    
void main_show_menu_window(unsigned int index);
void main_reload_config();


/** Functions implemented in clicks.c */
typedef struct 
{
   uint32_t time;
   uint32_t flags_n_elapsed; // BUFFER_FLAG_MASK flags and rest are duration
} ButtonRegistryBuffer;

/// Config click provider for the window
void click_config_provider( );

/** Initialize the registry */
void click_registry_init();

/** @return true if the button is enabled */
bool click_registry_enabled( unsigned int index );
/** @return true if the @param elapsed has been updated to contain duration seconds on the @param index button */
bool click_registry_get_elapsed( time_t time_now, unsigned int index, unsigned int* elapsed, unsigned int* action_time );
/** @return true if the @param elapsed has been updated as above but for since last action */
bool click_registry_get_since_last( time_t time_now, unsigned int index, unsigned int* elapsed, unsigned int* action_time );
/** @returns button id from index */
ButtonId click_get_button_id_from_index( int index );

/** Clear the @param loop entry from the @param index registry buffer */
void click_registry_clear( unsigned int index, unsigned int loop );
/** Finish it */
void click_registry_clear_finish( unsigned int index );

/** @returns the @param loop 'th action from the @param index buffer or NULL if the action has not been taken */
const ButtonRegistryBuffer* click_registry_get_action( unsigned int index, unsigned int loop );


/** Functions implemented in main_window.c */
/// Update the window to display elapsed @param time_now since start
void main_window_update_elapsed( time_t time_now  );
void get_time_splitted( unsigned int* hour, unsigned int* min, unsigned* sec );

/// Window loading function
void main_window_unload(Window *window) ;
void main_window_load(Window *window) ;

void menu_window_unload( Window *window );
void menu_window_load( Window *window );
void menu_window_set_data( unsigned int index );
void main_window_reload_config();

/** Functions implemented in config.c */
/// @return is the action enabled
uint8_t config_get( ButtonId button_id );
/** @return resource id for the icon for the given button */
uint32_t config_get_icon( ButtonId button_id );

/// Write config to storage
void config_set_flags( ButtonId button_id, uint8_t flags );
void config_set_icon ( ButtonId button_id, uint32_t icon_id ); 

/** Functions implemented in communication.c */

#define COMMUNICATION_KEY_CONFIG 200
#define COMMUNICATION_KEY_TYPE   1
#define COMMUNICATION_KEY_BUTTON 2

/// Init
void communication_init();
/// Send click event 
void communication_send_click( char key, char type );

#endif