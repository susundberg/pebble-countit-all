#ifndef MAIN_H
#define MAIN_H

#include <pebble.h>

#define FLAG_SINGLE_ACTION_BUTTON   0x01
#define FLAG_LONG_ACTION_BUTTON     0x02


/** Functions implemented in clicks.c */
/// Config click provider for the window
void click_config_provider( );

/** Initialize the registry */
void click_registry_init();

/** @return true if the @param elapsed has been updated to contain duration seconds on the @param index button */
bool click_registry_get_elapsed( time_t time_now, unsigned int index, unsigned int* elapsed );
/** @return true if the @param elapsed has been updated as above but for since last action */
bool click_registry_get_since_last( time_t time_now, unsigned int index, unsigned int* elapsed );


/** Functions implemented in main_window.c */
/// Update the window to display elapsed @param time_now since start
void main_window_update_elapsed( time_t time_now  );

/// Window loading function
void main_window_unload(Window *window) ;
void main_window_load(Window *window) ;


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