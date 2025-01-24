#include "board_layout.h"
#include "pico/stdlib.h"

void board_output_init()
{
  uint8_t output[5] = {
      DPAD_UP_PIN,
      DPAD_DOWN_PIN,
      DPAD_RIGHT_PIN,
      DPAD_LEFT_PIN,
      FIRE_BTN,
  };

  for (uint8_t i = 0; i < 5; i++)
  {
    gpio_init(output[i]);
    gpio_set_dir(output[i], GPIO_OUT);
    gpio_put(output[i], HIGH);
  }
}