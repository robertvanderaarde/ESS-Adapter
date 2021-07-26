
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Wire.h>
#include "ESS_UI.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

void ESS_UI::init(bool enable)
{
  Wire.begin();
  Wire.setClock(400000L);

#if RST_PIN >= 0
  oled.begin(&Adafruit5x7, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(System5x7);
  oled.clear();
  enabled = enable;
}

void ESS_UI::update(unsigned short ess, unsigned short wlk, unsigned short run, unsigned short trigger) {
    ess_start = ess;
    wlk_start = wlk;
    run_start = run;
    trigger_start = trigger;

    switch (state) {
      case IDLE:
        draw_idle_screen();
        break;
      case MAIN_MENU:
        draw_main_menu();
        break;
      case MODIFY:
        draw_modify_menu();
    }
}

void ESS_UI::set_enabled(bool enable) {
  enabled = enable;
}

String ESS_UI::joystick_to_pct_str (unsigned short value) {
  unsigned short val = value - 128;
  return String((unsigned short)(100 * ((double)val / (double)128)));
}

String ESS_UI::trigger_to_pct_str (unsigned short value) {
  return String((unsigned short)(100 * ((double)value / (double)256)));
}

void ESS_UI::draw_header() {
  oled.setCursor(0, 0);
  oled.print(F("ESS ADAPTER "));
  enabled ? oled.print(F("ENABLED")) : oled.print(F("DISABLED"));
  oled.setCursor(0, 1);
  oled.print(F("-----------------------------------------"));
}

void ESS_UI::toggle_enabled() {
    enabled = !enabled;
}

bool ESS_UI::get_enabled(){
    return enabled;
}

bool ESS_UI::in_menu() {
    return state != IDLE;
}

void ESS_UI::enter_menu() {
  state = MAIN_MENU;
  selected_button_index = 0;
  draw_main_menu();
}

void ESS_UI::exit_menu() {
  state = IDLE;
  selected_button_index = 0;
  modify_input_index = 0;
  currently_modifying = false;
  draw_idle_screen();
}

unsigned short* ESS_UI::get_inputs() {
  return new unsigned short[5] {
    ess_start, wlk_start, run_start, trigger_start, (unsigned short)enabled
  };
}

unsigned short* ESS_UI::get_x_coords() {
  return x_coords;
}

unsigned short* ESS_UI::get_y_coords() {
  return y_coords;
}

void ESS_UI::send_inputs(unsigned short inputs[]) {
  switch(state) {
    case MAIN_MENU:
      if (inputs[5] == 1) {
        exit_menu();
      }
      if (inputs[2] == 1) {
        if (selected_button_index > 0){
          selected_button_index--;
        }

        draw_main_menu();
      }
      if (inputs[3] == 1) {
        selected_button_index++;
        if (selected_button_index > 2){
          selected_button_index = 2;
        }

        draw_main_menu();
      }
      if (inputs[4] == 1) {
        switch (selected_button_index){
          case 0:
            toggle_enabled();
            draw_main_menu();
            break;
          case 1:
            selected_button_index = 0;
            state = MODIFY;
            draw_modify_menu();
            break;
          case 2:
            selected_button_index = 0;
            state = CALIBRATE;
            currently_modifying = false;
            modify_input_index = 0;
            draw_calibration_menu();
            break;
        }
      }
      break;

    case CALIBRATE:
      if (inputs[5] == 1 && !currently_modifying) {
        state = MAIN_MENU;
        selected_button_index = 2;
        draw_main_menu();
      }
      if (inputs[4] == 1) {
        unsigned short coord_idx = 255;
        switch (modify_input_index) {
          case 0:
            currently_modifying = true;
            break;
          case 1:
            coord_idx = 8;
            break;
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
          case 9:
            coord_idx = modify_input_index - 2;
            break;
          case 10:
            modify_input_index = 0;
            selected_button_index = 2;
            state = MAIN_MENU;
            draw_main_menu();
            return;
        }
        if (coord_idx != 255) {
            x_coords[coord_idx] = inputs[6];
            y_coords[coord_idx] = inputs[7];
        }
        
        modify_input_index++;
        if (modify_input_index > 9) {
          modify_input_index = 0;
          currently_modifying = false;
        }
        draw_calibration_menu();
      }
      if (inputs[8] == 1) {
        modify_input_index = 10;
        for (unsigned short i = 0; i < 10; i++){
          x_coords[i] = default_x_coords[i];
          y_coords[i] = default_y_coords[i];
        }
        currently_modifying = false;
        draw_calibration_menu();
      }
      break;

    case MODIFY:
      if (currently_modifying) {
        if (inputs[5] == 1 || inputs[4] == 1) {
          currently_modifying = false;
          draw_modify_menu();
        } else if (inputs[1] == 1) {
          modify_inputs(1, modify_input_index);
          draw_modify_menu();
        } else if (inputs[0] == 1) {
          modify_inputs(-1, modify_input_index);
          draw_modify_menu();
        }
      } else {
        if (inputs[2] == 1) {
          modify_input_index--;
          modify_input_index = modify_input_index < 0 ? 0 : modify_input_index;
          draw_modify_menu();
        } else if (inputs[3] == 1) {
          modify_input_index++;
          modify_input_index = modify_input_index > 3 ? 3 : modify_input_index;
          draw_modify_menu();
        } else if (inputs[4] == 1) {
          currently_modifying = true;
          draw_modify_menu();
        } else if (inputs[5] == 1) {
          state = MAIN_MENU;
          selected_button_index = 1;
          draw_main_menu();
        }
      }
  }
}

void ESS_UI::modify_inputs(unsigned short val, unsigned short index) {
  unsigned short min = 0;
  unsigned short max = 0;
  switch (index) {
    case 0:
      min = 1;
      max = wlk_start - 1;
      ess_start+=val;
      ess_start = ess_start > max ? max : ess_start < min ? min : ess_start;
      break;
    case 1:
      min = ess_start + 1;
      max = run_start - 1;
      wlk_start+=val;
      wlk_start = wlk_start > max ? max : wlk_start < min ? min : wlk_start;
      break;
    case 2:
      min = wlk_start + 1;
      max = 99;
      run_start+=val;
      run_start = run_start > max ? max : run_start < min ? min : run_start;
      break;
    case 3:
      min = 1;
      max = 99;
      trigger_start+=val;
      trigger_start = trigger_start > max ? max : trigger_start < min ? min : trigger_start;
      break;
  }
}

void ESS_UI::draw_text(String text) {
    oled.clear();
    oled.setCursor(0, 2);
    oled.print(text);
}

void ESS_UI::draw_modify_menu() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(F("MODIFY RANGES"));
  oled.setCursor(0, 1);
  oled.print(F("-----------------------------------------"));
  oled.setCursor(49, 3);
  oled.print(F("New | Default"));

  if (modify_input_index == 0){
    if (currently_modifying) {
      oled.setCursor(0, 4);
      oled.print(F(">>>>"));
    } else {
      oled.setCursor(20, 4);
      oled.print(F(">"));
    }
  } else {
    oled.setCursor(25, 4);
  }

  oled.print(F("ESS: "));
  oled.print(ess_start);
  if (ess_start < 10){
    oled.print(F(" "));
  }
  oled.print(F(" | "));
  oled.print(F("18"));

  if (modify_input_index == 1){
    if (currently_modifying) {
      oled.setCursor(0, 5);
      oled.print(F(">>>>"));
    } else {
      oled.setCursor(20, 5);
      oled.print(F(">"));
    }
  } else {
    oled.setCursor(25, 5);
  }

  oled.print(F("WLK: "));
  oled.print(wlk_start);
  if (wlk_start < 10){
    oled.print(F(" "));
  }
  oled.print(F(" | "));
  oled.print(F("29"));

  if (modify_input_index == 2){
    if (currently_modifying) {
      oled.setCursor(0, 6);
      oled.print(F(">>>>"));
    } else {
      oled.setCursor(20, 6);
      oled.print(F(">"));
    }
  } else {
    oled.setCursor(25, 6);
  }

  oled.print(F("RUN: "));
  oled.print(run_start);
  if (run_start < 10){
    oled.print(F(" "));
  }
  oled.print(F(" | "));
  oled.print(F("45"));

  if (modify_input_index == 3){
    if (currently_modifying) {
      oled.setCursor(0, 7);
      oled.print(F(">>>>"));
    } else {
      oled.setCursor(20, 7);
      oled.print(F(">"));
    }
  } else {
    oled.setCursor(25, 7);
  }

  oled.print(F("L/R: "));
  oled.print(trigger_start);
  oled.print(F(" | "));
  oled.print(F("83"));
}

void ESS_UI::draw_idle_screen() {
    oled.clear();
    draw_header();
    draw_values();

    oled.setCursor(0, 2);
    oled.print(F("L+R+Z+CLEFT"));
    oled.setCursor(0, 3);
    oled.print(F("TO ENTER"));
    oled.setCursor(0, 4);
    oled.print(F("THE MENU"));
}

void ESS_UI::draw_values() {
  if (!enabled) {
    return;
  }

  oled.setCursor(85, 4);
  oled.print(F("ESS: "));
  oled.print(ess_start);
  oled.setCursor(85, 5);
  oled.print(F("WLK: "));
  oled.print(wlk_start);
  oled.setCursor(85, 6);
  oled.print(F("RUN: "));
  oled.print(run_start);
  oled.setCursor(85, 7);
  oled.print(F("L/R: "));
  oled.print(trigger_start);
}

void ESS_UI::draw_calibration_menu() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(F("CALIBRATE JOYSTICK"));
  oled.setCursor(0, 1);
  oled.print(F("-----------------------------------------"));


  oled.setCursor(0, 2);
  switch (modify_input_index) {
    case 0:
      oled.print(F("Press A to start\ncalibration."));
      oled.setCursor(0, 5);
      oled.print(F("Press Z to reset\nvalues to default."));
      break;
    case 1:
      oled.print(F("Put joystick to center\nposition and press A."));
      break;
    case 2:
      oled.print(F("Put joystick to top\nnotch and press A."));
      break;
    case 3:
      oled.print(F("Put joystick to top\nright notch and\npress A."));
      break;
    case 4:
      oled.print(F("Put joystick to right\nnotch and press A."));
      break;
    case 5:
      oled.print(F("Put joystick to\nbottom right notch\nand press A."));
      break;
    case 6:
      oled.print(F("Put joystick to\nbottom notch and\npress A."));
      break;
    case 7:
      oled.print(F("Put joystick to\nbottom left notch\nand press A."));
      break;
    case 8:
      oled.print(F("Put joystick to left\nnotch and press A."));
      break;
    case 9:
      oled.print(F("Put joystick to top\nleft notch and\npress A."));
      break;
    case 10:
      oled.print(F("Joystick values\nreset to default!"));
      break;
  }
}

void ESS_UI::draw_main_menu() {
  oled.clear();
  draw_header();
  draw_values();

  oled.setCursor(0, 2);
  oled.print(F("B TO EXIT MENU"));

  oled.setCursor(0, 5);
  if (selected_button_index == 0){
      oled.print(F(">"));
  }
  if (enabled) {
    oled.print(F("[ DISABLE   ]"));
  } else {
    oled.print(F("[ ENABLE    ]"));
  }
  oled.setCursor(0, 6);
  if (selected_button_index == 1){
      oled.print(F(">"));
  }
  oled.print(F("[ MODIFY    ]"));

  oled.setCursor(0, 7);
  if (selected_button_index == 2){
      oled.print(F(">"));
  }
  oled.print(F("[ CALIBRATE ]"));
}