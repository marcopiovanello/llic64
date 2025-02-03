#define VID_M30_8BIT_DO = 0x0c3a;

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct TU_ATTR_PACKED
{
  uint8_t x, y, z, rz; // joystick axis NOT USED

  struct
  {
    uint8_t hat : 4;   // button mask for currently pressed
    uint8_t button_X : 1;
    uint8_t button_B : 1;
    uint8_t button_A : 1;
    uint8_t button_Y : 1;
  };
  
} m30_8bitdo_report_t;

#ifdef __cplusplus
 }
#endif