#include "pico/stdlib.h"

#define VID_M30_8BIT_DO = 0x0c3a;

typedef struct TU_ATTR_PACKED
{
  uint8_t x, y, z, rz; // joystick axis NOT USED

  struct
  {
    uint8_t dpad : 4;
    uint8_t x : 1;
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t y : 1;
  };
  
} m30_8bitdo_report_t;