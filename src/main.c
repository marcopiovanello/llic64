#include <stdio.h>

#include "tusb.h"


#include "board_layout.h"
#include "board_output.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"

const uint LED_PIN = 25;

void led_blinking_task(void);

extern void hid_app_task(void);

int main(void)
{
  board_output_init();
  stdio_uart_init();

  multicore_launch_core1(led_blinking_task);

  tusb_init();

  while (1)
  {
    tuh_task();
    hid_app_task();
  }

  return 0;
}

// System-good task
// Blink every second to signal that the board has not crashed
void led_blinking_task(void)
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  while (1) 
  {
    gpio_put(LED_PIN, 0);
    sleep_ms(1000);
    gpio_put(LED_PIN, 1);
    sleep_ms(500);
  }
}