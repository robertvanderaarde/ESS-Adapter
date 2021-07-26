#include <ESS_Adapter.h>

#include <ESS_UI.h>

#include <Gamecube.h>
#include <GamecubeAPI.h>
#include <Gamecube_N64.h>
#include <N64.h>
#include <N64API.h>
#include <Nintendo.h>
#include <string.h>
#include "Nintendo.h"
#include <EEPROM.h>

// Arbitrary number detailing if we've saved before
#define HAS_SAVED 77

/**
 * Purple adapter uses 2-10
 *                    (controller-console)
 * Gold adapter uses 2-9? (need to test)
 * Standard adapters use 6-3
 * (First standard adapter uses 6-2)
 */

CGamecubeController GamecubeController(4);
CGamecubeConsole GamecubeConsole(3);
Gamecube_Report_t report;

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

ESS_UI ui;
ESS_Adapter adapter;

void setup() {
  Serial.begin(115200);
  adapter.init();
  ui.init(true);
  ui.x_coords = adapter.x_coords;
  ui.y_coords = adapter.y_coords;
  ui.default_x_coords = adapter.default_x_coords;
  ui.default_y_coords = adapter.default_y_coords;
  load_inputs();

  adapter.oled = ui.oled;
}

void send_modified_inputs() {
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
    short input[4] = {
      report.xAxis, report.yAxis, report.left, report.right
    };
    
    adapter.modify_input(input);
    
    report.xAxis = input[0];
    report.yAxis = input[1];
    report.l = input[2];
    report.r = input[3];
  }
  GamecubeConsole.write(report);

  // Check to see if we should enter the menu
  if (report.r && report.l && report.z && report.cxAxis < 100) {
    enter_menu();
  }
}

short ui_inputs[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
bool needs_update = false;

void enter_menu() {
  needs_update = true;
  ui.enter_menu();
}

void update_ui_input(short idx, short button_val) {
    if (button_val > 0 && button_val < 3) {
      ui_inputs[idx]++;
    } else {
      ui_inputs[idx] = 0; 
    }
}

void send_ui_inputs() {
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
    update_ui_input(0, report.dleft);
    update_ui_input(1, report.dright);
    update_ui_input(2, report.dup);
    update_ui_input(3, report.ddown);
    update_ui_input(4, report.a);
    update_ui_input(5, report.b);
    ui_inputs[6] = report.xAxis;
    ui_inputs[7] = report.yAxis;
    update_ui_input(8, report.z);
    ui.send_inputs(ui_inputs);
  }
}

void save_inputs() {
  short* inputs = ui.get_inputs();
  EEPROM.write(0, 6); // Has saved! We can load them next time.
  EEPROM.write(1, inputs[0]); // ess_start
  EEPROM.write(2, inputs[1]); // wlk_start
  EEPROM.write(3, inputs[2]); // run_start
  EEPROM.write(4, inputs[3]); // trigger_start
  EEPROM.write(5, inputs[4]); // enabled

  for (int i = 6; i < 15; i++) {
    EEPROM.write(i, adapter.x_coords[i - 6]);
  }
  
  for (int i = 15; i < 24; i++) {
    EEPROM.write(i, adapter.y_coords[i - 15]);
  }
}

void load_inputs() {
  short inputs[5] = {
    EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5)
  };

  // If we've saved
  if (EEPROM.read(0) == 6) {
    ui.update(inputs[0], inputs[1], inputs[2], inputs[3]);
    ui.set_enabled((bool)inputs[4]);
    adapter.update(inputs);
    adapter.enabled = (bool)inputs[4];

    for (int i = 6; i < 15; i++) {
      adapter.x_coords[i - 6] = EEPROM.read(i);
    }

    for (int i = 15; i < 24; i++) {
      adapter.y_coords[i - 15] = EEPROM.read(i);
    }
  } else {
    ui.update(
      adapter.joy_to_pct_value(adapter.ess_start),
      adapter.joy_to_pct_value(adapter.wlk_start),
      adapter.joy_to_pct_value(adapter.run_start),
      adapter.trigger_to_pct_value(adapter.set_trigger)
    );
  }
  ui.exit_menu();
}

short input = 0;
// Allows for sending joystick inputs using dpad
void send_test_input() {
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
    if (report.dright) {
      input++;
      input = input < 0 ? 0 : input > 255 ? 255 : input;

      ui.oled.clear();
      ui.oled.print(input);
    }
    if (report.dleft) {
      input--;
      input = input < 0 ? 0 : input > 255 ? 255 : input;

      ui.oled.clear();
      ui.oled.print(input);
    }
    report.xAxis = input;
    report.dleft = 0;
    report.dright = 0;
  }
  
  GamecubeConsole.write(report);
}

void loop() {
  if (!ui.in_menu()) {
    if (needs_update) {
      adapter.update(ui.get_inputs());
      save_inputs();
      needs_update = false;
    }
    send_modified_inputs();
  } else {
    send_ui_inputs();
  }
}
