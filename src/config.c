#include <pebble.h>
#include "main.h"

static const uint32_t LOCAL_icon_resources[] = { RESOURCE_ID_ICON_000, RESOURCE_ID_ICON_001, RESOURCE_ID_ICON_002, RESOURCE_ID_ICON_003, RESOURCE_ID_ICON_004, RESOURCE_ID_ICON_005, RESOURCE_ID_ICON_006, RESOURCE_ID_ICON_007, RESOURCE_ID_ICON_008, RESOURCE_ID_ICON_009, RESOURCE_ID_ICON_010, RESOURCE_ID_ICON_011, RESOURCE_ID_ICON_012, RESOURCE_ID_ICON_013, RESOURCE_ID_ICON_014, RESOURCE_ID_ICON_015, RESOURCE_ID_ICON_016, RESOURCE_ID_ICON_017, RESOURCE_ID_ICON_018, RESOURCE_ID_ICON_019, RESOURCE_ID_ICON_020, RESOURCE_ID_ICON_021, RESOURCE_ID_ICON_022, RESOURCE_ID_ICON_023, RESOURCE_ID_ICON_024, RESOURCE_ID_ICON_025, RESOURCE_ID_ICON_026, RESOURCE_ID_ICON_027, RESOURCE_ID_ICON_028, RESOURCE_ID_ICON_029, RESOURCE_ID_ICON_030, RESOURCE_ID_ICON_031, RESOURCE_ID_ICON_032, RESOURCE_ID_ICON_033, RESOURCE_ID_ICON_034, RESOURCE_ID_ICON_035, RESOURCE_ID_ICON_036, RESOURCE_ID_ICON_037, RESOURCE_ID_ICON_038, RESOURCE_ID_ICON_039, RESOURCE_ID_ICON_040, RESOURCE_ID_ICON_041, RESOURCE_ID_ICON_042, RESOURCE_ID_ICON_043, RESOURCE_ID_ICON_044, RESOURCE_ID_ICON_045, RESOURCE_ID_ICON_046, RESOURCE_ID_ICON_047, RESOURCE_ID_ICON_048, RESOURCE_ID_ICON_049, RESOURCE_ID_ICON_050, RESOURCE_ID_ICON_051, RESOURCE_ID_ICON_052, RESOURCE_ID_ICON_053, RESOURCE_ID_ICON_054, RESOURCE_ID_ICON_055 };


uint32_t local_get_key( ButtonId button_id )
{
   return 5*(int)button_id + PERSISTANT_STORAGE_CONFIG_START;
}

uint8_t config_get( ButtonId button_id )
{
   uint32_t per_key = local_get_key(button_id);
   int32_t ret = persist_read_int( per_key );
   APP_LOG( APP_LOG_LEVEL_INFO, "Got flags %d : %d", (int)button_id, (int)ret ); 
   return (uint8_t)ret;
}

void config_set_flags(ButtonId button_id, uint8_t flags )
{
   uint32_t per_key = local_get_key(button_id);
   APP_LOG( APP_LOG_LEVEL_INFO, "Storing flags %d : %d", (int)button_id, (int)flags ); 
   if ( flags & ~FLAG_MASK_VALID )
      return;   
   persist_write_int( per_key, flags );
}

void config_set_icon (ButtonId button_id,  uint32_t icon_id )
{
   uint32_t per_key = local_get_key(button_id) + 1;
   APP_LOG( APP_LOG_LEVEL_INFO, "Save icon %d -> %d", (int)button_id, (int)icon_id ); 
   if ( icon_id > sizeof(LOCAL_icon_resources)/sizeof(uint32_t) )
      return;      
   persist_write_int( per_key, icon_id );
   
}


uint32_t config_get_icon( ButtonId button_id )
{
   uint32_t per_key = local_get_key(button_id) + 1;
   int32_t ret = persist_read_int( per_key );
   APP_LOG( APP_LOG_LEVEL_INFO, "Load icon %d -> %d", (int)button_id, (int)ret ); 
   return LOCAL_icon_resources[ (int)ret ];
}

