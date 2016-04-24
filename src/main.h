#ifndef MAIN_H
#define MAIN_H

#include <pebble.h>

#define FLAG_SINGLE_ACTION_BUTTON   0x01
#define FLAG_LONG_ACTION_BUTTON     0x02


#define BUTTON_N_INDEX 3
#define BUFFER_SIZE 32


#define PERSISTANT_STORAGE_DATA_START  1000

void main_show_menu_window(unsigned int index);



/** Functions implemented in clicks.c */
typedef struct __attribute__ ((__packed__))
{
   uint32_t time;
   uint16_t elapsed;
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

/** Functions implemented in config.c */
/// @return is the action enabled
uint8_t config_get( ButtonId button_id );
/// Write config to storage
void config_set( ButtonId button_id, uint8_t flags );


/** Functions implemented in communication.c */

#define COMMUNICATION_KEY_CONFIG 200
#define COMMUNICATION_KEY_TYPE   1
#define COMMUNICATION_KEY_BUTTON 2

/// Init
void communication_init();
/// Send click event 
void communication_send_click( char key, char type );

#endif