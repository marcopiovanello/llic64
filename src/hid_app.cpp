#include "tusb.h"

#include "board_layout.h"

#include "pico/stdlib.h"
#include "hid_app.h"

#include <list>
#include <set>
#include <algorithm>

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

extern "C" void hid_app_task(void)
{
    return;
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  printf("Device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  printf("VID = %04x, PID = %04x\r\n", vid, pid);

  if (!tuh_hid_receive_report(dev_addr, instance))
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

inline bool diff_than_2(uint8_t x, uint8_t y)
{
  return (x - y > 2) || (y - x > 2);
}

// check if 2 reports are different enough
bool diff_report(hid_gamepad_report_t const *rpt1, hid_gamepad_report_t const *rpt2)
{
  bool result;

  // x, y, z, rz must different than 2 to be counted
  result = diff_than_2(rpt1->x, rpt2->x) || diff_than_2(rpt1->y, rpt2->y) ||
           diff_than_2(rpt1->z, rpt2->z) || diff_than_2(rpt1->rz, rpt2->rz);

  // check the result with mem compare
  result |= memcmp(&rpt1->ry + 1, &rpt2->ry + 1, sizeof(hid_gamepad_report_t) - 6);

  return result;
}

//--------------------------------------------------------------------+
// Core functionalities
//--------------------------------------------------------------------+

// process joystick HID report
void process_joystick_report(hid_gamepad_report_t const *report, uint16_t len)
{
  // previous report used to compare for changes
  static hid_gamepad_report_t prev_report = {0};

  len--;

  hid_gamepad_report_t dev_report;
  memcpy(&dev_report, report, sizeof(dev_report));

  if (diff_report(&prev_report, &dev_report))
  {

    stick_handler(dev_report.rz, dev_report.z, 10);

    dev_dpad_handler(dev_report.hat);

    // board_toggle_output(UP_PIN, dev_report.west);
    // board_toggle_output(FIRE_BTN, dev_report.south);
    // board_toggle_output(JUMP_BTN, dev_report.north);
    board_toggle_output(FIRE_BTN, dev_report.buttons & (GAMEPAD_BUTTON_A | GAMEPAD_BUTTON_B | GAMEPAD_BUTTON_C));
  }

  prev_report = dev_report;
}

static inline void keyboard_key_handler(uint8_t keycode, bool enabled)
{
  switch (keycode) 
  {
    case HID_KEY_KEYPAD_0: case HID_KEY_J: case HID_KEY_Z:
      board_toggle_output(FIRE_BTN, enabled);
      break;
    case HID_KEY_KEYPAD_8: case HID_KEY_W: case HID_KEY_ARROW_UP:
      board_toggle_output(UP_PIN, enabled);
      break;
    case HID_KEY_KEYPAD_6: case HID_KEY_D: case HID_KEY_ARROW_RIGHT:
      board_toggle_output(RIGHT_PIN, enabled);
      break;
    case HID_KEY_KEYPAD_5: case HID_KEY_KEYPAD_2: case HID_KEY_S: case HID_KEY_ARROW_DOWN:
      board_toggle_output(DOWN_PIN, enabled);
      break;
    case HID_KEY_KEYPAD_4: case HID_KEY_A: case HID_KEY_ARROW_LEFT:
      board_toggle_output(LEFT_PIN, enabled);
      break;
    default:
      break;
  }
}

// it's actually horrible
static inline void socd_key_filter(std::list<uint8_t>& keycodes)
{
  uint8_t keycode = keycodes.back();

  switch (keycode) 
  {  
    case HID_KEY_KEYPAD_8: case HID_KEY_W: case HID_KEY_ARROW_UP:
    {
      auto iterator = std::remove_if(keycodes.begin(), keycodes.end(), [](uint8_t k){
        return k == HID_KEY_KEYPAD_5 
          || k == HID_KEY_KEYPAD_2 
          || k == HID_KEY_S 
          || k == HID_KEY_ARROW_DOWN;
      });
      keycodes.erase(iterator, keycodes.end());
      board_toggle_output(DOWN_PIN, HIGH);
      break;
    }
    case HID_KEY_KEYPAD_6: case HID_KEY_D: case HID_KEY_ARROW_RIGHT:
    {
      auto iterator = std::remove_if(keycodes.begin(), keycodes.end(), [](uint8_t k){
        return k == HID_KEY_KEYPAD_4 
          || k == HID_KEY_A 
          || k == HID_KEY_ARROW_LEFT; 
      });
      keycodes.erase(iterator, keycodes.end());
      board_toggle_output(LEFT_PIN, HIGH);
      break;
    }
    case HID_KEY_KEYPAD_5: case HID_KEY_KEYPAD_2: case HID_KEY_S: case HID_KEY_ARROW_DOWN:
    {
      auto iterator = std::remove_if(keycodes.begin(), keycodes.end(), [](uint8_t k){
        return k == HID_KEY_KEYPAD_8
          || k == HID_KEY_W
          || k == HID_KEY_ARROW_UP;
      });
      keycodes.erase(iterator, keycodes.end());
      board_toggle_output(UP_PIN, HIGH);
      break;
    }
    case HID_KEY_KEYPAD_4: case HID_KEY_A: case HID_KEY_ARROW_LEFT:
    {
      auto iterator = std::remove_if(keycodes.begin(), keycodes.end(), [](uint8_t k){
        return k == HID_KEY_KEYPAD_6
          || k == HID_KEY_D
          || k == HID_KEY_ARROW_RIGHT;
      });
      keycodes.erase(iterator, keycodes.end());
      board_toggle_output(RIGHT_PIN, HIGH);
      break;
    }
    default:
      break;
  }
}

static inline bool find_key_in_report(const hid_keyboard_report_t &report, uint8_t keycode) 
{
  for (uint8_t i = 0; i < 6; i++) 
  {
    if (report.keycode[i] == keycode) return true;
  }

  return false;
}

void process_keyboard_report(const hid_keyboard_report_t *report)
{
  // needs to be static since it has to be persisted between calls

  static hid_keyboard_report_t prev_report = { 0, 0, { 0 } };

  // use a vector easily to remove keycodes when SOCD will be implemented

  std::list<uint8_t> report_keycodes { std::begin(report->keycode), std::end(report->keycode) };

  //socd_key_filter(report_keycodes);

  for (auto const &keycode : report_keycodes)
  {
    if (!keycode) continue;
    
    if (!find_key_in_report(prev_report, keycode)) 
    {
      keyboard_key_handler(keycode, true);
    } 
  }
  
  // use two sets to quickly find which keycode isn't present in the current report

  std::set<uint8_t> prev_keycodes{ std::begin(prev_report.keycode), std::end(prev_report.keycode) };
  std::set<uint8_t> curr_keycodes{ report_keycodes.begin(), report_keycodes.end() };

  std::set<uint8_t> released_keys{};

  std::set_difference(
    prev_keycodes.begin(), prev_keycodes.end(), 
    curr_keycodes.begin(), curr_keycodes.end(),
    std::inserter(released_keys, released_keys.begin())
  );

  for (auto const& keycode : released_keys)
  {
    // disable the output of the released key
    keyboard_key_handler(keycode, false);
  }

  prev_report = *report;
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
  case HID_ITF_PROTOCOL_KEYBOARD:
    process_keyboard_report(reinterpret_cast<hid_keyboard_report_t const *>(report));
    break;
  default:
    process_joystick_report(reinterpret_cast<hid_gamepad_report_t const *>(report), len);
    break;
  }

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance))
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

inline void board_toggle_output(uint8_t pin, uint8_t enabled)
{
  enabled ? gpio_put(pin, LOW) : gpio_put(pin, HIGH);
}

static inline void dev_dpad_handler(uint8_t state)
{
  printf("dpad state: %02x\n", state);

  if (state == GAMEPAD_HAT_UP) 
  {
    gpio_put(UP_PIN, LOW);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }
  
  if (state == GAMEPAD_HAT_UP_RIGHT) 
  {
    gpio_put(UP_PIN, LOW);
    gpio_put(RIGHT_PIN, LOW);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }

  if (state == GAMEPAD_HAT_RIGHT) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, LOW);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }
    
  if (state == GAMEPAD_HAT_DOWN_RIGHT) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, LOW);
    gpio_put(DOWN_PIN, LOW);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }
  
  if (state == GAMEPAD_HAT_DOWN) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, LOW);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }

  if (state == GAMEPAD_HAT_DOWN_LEFT) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, LOW);
    gpio_put(LEFT_PIN, LOW);
    return;
  }

  if (state == GAMEPAD_HAT_LEFT) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, LOW);
    return;
  }

  if (state == GAMEPAD_HAT_UP_LEFT) 
  {
    gpio_put(UP_PIN, LOW);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, LOW);
    return;
  }

  if (state == GAMEPAD_HAT_CENTERED) 
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(RIGHT_PIN, HIGH);
    gpio_put(DOWN_PIN, HIGH);
    gpio_put(LEFT_PIN, HIGH);
    return;
  }
}
/*
  Handle the stick rotation in the 2 axis
  --------------------------------------------------------------------
  The default state is the following:
  127   127    -> released
  The following transorm will be applied for-each report
   y     x
  127   0      -> Left
  127   255    -> Right
  0     127    -> Up
  255   127    -> Down
  --------------------------------------------------------------------
  A dead zone can be applied to capture the input.
  The output transform will be the following, where d is the dead zone
     y       x
  127     (127-d)
  127     (127+d)
  (127-d)   127
  (127+d)   127
*/
static inline void stick_handler(uint8_t deg_value_y, uint8_t deg_value_x, uint8_t dead_zone)
{
  if (deg_value_y == 127)
  {
    gpio_put(UP_PIN, HIGH);
    gpio_put(DOWN_PIN, HIGH);
  }
  if (deg_value_y > (127 + dead_zone))
  {
    gpio_put(DOWN_PIN, LOW);
  }
  if (deg_value_y < (127 - dead_zone))
  {
    gpio_put(UP_PIN, LOW);
  }
  deg_value_x > (127 + dead_zone)
      ? gpio_put(RIGHT_PIN, LOW)
      : gpio_put(RIGHT_PIN, HIGH);
  deg_value_x < (127 - dead_zone)
      ? gpio_put(LEFT_PIN, LOW)
      : gpio_put(LEFT_PIN, HIGH);
}