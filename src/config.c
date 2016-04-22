#include "main.h"


uint8_t config_get( ButtonId button_id )
{
   if ( button_id == BUTTON_ID_UP )
      return FLAG_LONG_ACTION_BUTTON;
   if ( button_id == BUTTON_ID_SELECT )
      return FLAG_SINGLE_ACTION_BUTTON;
   return 0x00;
}