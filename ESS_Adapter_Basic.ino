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

enum AdapterState {
  NormalInput,
  ModifiedInput,
  ModifyESS,
  ModifyWLK,
  ModifyRUN,
  ModifyTriggers
};

AdapterState state;
CGamecubeController GamecubeController(2);
CGamecubeConsole GamecubeConsole(10);
Gamecube_Data_t d = defaultGamecubeData;
Gamecube_Report_t nullReport;
Gamecube_Report_t report;
bool adjusted_last_frame = true;

double default_range[] = {1, 71, 91, 105, 128, 151, 165, 185, 255};
double x_range[] = {1, 71, 91, 125, 128, 131, 165, 185, 255};
double y_range[] = {1, 71, 91, 110, 117, 124, 165, 200, 255};

int ess_start = 151;
int wlk_start = 165;
int run_start = 185;
int x_center = 128;
int y_center = 128;
int x_cache[256]; 
int y_cache[256];
int default_trigger = 213;
int set_trigger = 213;

void setup() {
  load_input();
  // put your setup code here, to run once:
  Serial.begin(115200);
}

bool load_input(){
  // If we haven't saved, quit out
  if (EEPROM.read(0) != HAS_SAVED){
    set_trigger = 213;
    state = ModifiedInput;
    setup_ranges();
    return false;
  }
  
  x_center = EEPROM.read(1);
  y_center = EEPROM.read(2);
  ess_start = EEPROM.read(3);
  wlk_start = EEPROM.read(4);
  run_start = EEPROM.read(5);
  set_trigger = EEPROM.read(7);
  int modified = EEPROM.read(6);
  if (modified){
    state = ModifiedInput;
  } else {
    state = NormalInput;
  }
  setup_ranges();
  return true;
}

void save_input(){
  EEPROM.write(0, HAS_SAVED); // Write a -100
  EEPROM.write(1, x_center);
  EEPROM.write(2, y_center);
  EEPROM.write(3, ess_start);
  EEPROM.write(4, wlk_start);
  EEPROM.write(5, run_start);
  EEPROM.write(7, set_trigger);

  int modified = 1; // 0 for NORMAL STATE, 1 for anything else (will be modified though)
  if (state == NormalInput){
    modified = 0;
  }
  EEPROM.write(6, modified);
}

void center_joysticks(Gamecube_Report_t report){
  x_center = report.xAxis;
  y_center = report.yAxis;
}

void setup_ranges(){
  x_range[4] = x_center;
  x_range[5] = x_center + (ess_start - 128);
  x_range[3] = x_center - (ess_start - 128);

  x_range[6] = x_center + (wlk_start - 128);
  x_range[2] = x_center - (wlk_start - 128);

  x_range[7] = x_center + (run_start - 128);
  x_range[1] = x_center - (run_start - 128);

  y_range[4] = y_center;
  y_range[5] = y_center + (ess_start - 128);
  y_range[3] = y_center - (ess_start - 128);

  y_range[6] = y_center + (wlk_start - 128);
  y_range[2] = y_center - (wlk_start - 128);

  y_range[7] = y_center + (run_start - 128);
  y_range[1] = y_center - (run_start - 128);

  setup_cache();
}

void setup_cache(){
  for (int i = 0; i < 256; i++){
    x_cache[i] = translate_input(i, true, x_range);
  }
  for (int i = 0; i < 256; i++){
    y_cache[i] = translate_input(i, false, y_range);
  }
}

void modify_input(Gamecube_Report_t report) {
  report.xAxis = x_cache[report.xAxis];
  report.yAxis = y_cache[report.yAxis];
  if (report.left >= set_trigger){
    report.l = 1;
  }
  if (report.right >= set_trigger){
    report.r = 1;
  }
  GamecubeConsole.write(report);
}

void check_state_change(Gamecube_Report_t report){
  switch (state){
    case NormalInput:
       if (report.z && report.r && report.l && report.dup){
        state = ModifiedInput;
        save_input();
      }
      break;
    case ModifiedInput:
      if (report.z && report.r && report.l && report.cxAxis < 100){
        center_joysticks(report);
        state = ModifyESS;
      } else if (report.z && report.r && report.l && report.cyAxis < 100 && report.cxAxis > 100 && report.cxAxis < 160){
        center_joysticks(report);
        state = ModifyWLK;
      } else if (report.z && report.r && report.l && report.cxAxis > 160){
        center_joysticks(report);
        state = ModifyRUN;
      } else if (report.z && report.r && report.l && report.ddown){
        state = NormalInput;
        save_input();
      } else if (report.z && report.r && report.l && report.a && report.x && report.y){
        state = ModifyTriggers;
      }
      break;
    case ModifyESS:
    case ModifyWLK:
    case ModifyRUN:  
    case ModifyTriggers:
      if (report.b){
        setup_ranges();
        state = ModifiedInput;
        save_input();
      }
      if (report.start){
        ess_start = default_range[5];
        wlk_start = default_range[6];
        run_start = default_range[7];
        x_center = 128;
        y_center = 128;
        set_trigger = default_trigger;
        setup_ranges();
      }
      break;
  }
}

void modify_ess(Gamecube_Report_t report){
  if (report.y && !adjusted_last_frame){
    ess_start-=1;
    if (ess_start < 129){
      ess_start = 129;
    }
  }
  if (report.x && !adjusted_last_frame){
    ess_start+=1;
    if (ess_start >= wlk_start){
      ess_start = wlk_start - 1;
    }
  }
}
void modify_wlk(Gamecube_Report_t report){
  if (report.y && !adjusted_last_frame){
    wlk_start-=1;
    if (wlk_start <= ess_start){
      wlk_start = ess_start + 1;
    }
  }
  if (report.x && !adjusted_last_frame){
    wlk_start+=1;
    if (wlk_start >= run_start){
      wlk_start = run_start - 1;
    }
  }
}
void modify_run(Gamecube_Report_t report){
  if (report.y && !adjusted_last_frame){
    run_start-=1;
    if (run_start <= wlk_start){
      run_start = wlk_start + 1;
    }
  }
  if (report.x && !adjusted_last_frame){
    run_start+=1;
    if (run_start >= 255){
      run_start -= 1;
    }
  }
}
void modify_trigger(Gamecube_Report_t report){
  if (report.y && !adjusted_last_frame){
    set_trigger-=5;
    if (set_trigger < 10){
      set_trigger = 10;
    }
  }
  if (report.x && !adjusted_last_frame){
    set_trigger+=5;
    if (set_trigger > 213){
      set_trigger = 213;
    }
  }
}

Gamecube_Report_t mod_joystick(Gamecube_Report_t report){
  Gamecube_Report_t new_report = report;
  new_report.a = 0;
  new_report.b = 0;
  new_report.x = 0;
  new_report.y = 0;
  new_report.start = 0;
  new_report.dleft = 0;
  new_report.dright = 0;
  new_report.ddown = 0;
  new_report.dup = 0;
  new_report.z = 0;
  new_report.r = 1;
  new_report.l = 0;
  new_report.cxAxis = 128;
  new_report.cyAxis = 128;
  new_report.yAxis = y_center;

  if (state == ModifyESS){
    if (report.a){
      double val = map_range(default_range[5] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(ess_start - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  if (state == ModifyWLK){
    if (report.a){
      double val = map_range(default_range[6] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(wlk_start - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  if (state == ModifyRUN){
    if (report.a){
      double val = map_range(default_range[7] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(run_start - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  if (state == ModifyTriggers){
    if (report.a){
      double val = map_range(default_trigger, 0, 255, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(set_trigger, 0, 255, 20, 78); 
      new_report.xAxis = (128 + val);
    }
  }
  return new_report;
}

void loop() {
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
  }
  bool x_y = (report.x || report.y);
  check_state_change(report);
  switch (state){
    case NormalInput:
      GamecubeConsole.write(report);
      break;
    case ModifiedInput:
      modify_input(report);
      break;
    case ModifyESS:
      modify_ess(report);
      report = mod_joystick(report);
      GamecubeConsole.write(report);
      break;
    case ModifyWLK:
      modify_wlk(report);
      report = mod_joystick(report);
      GamecubeConsole.write(report);
      break;
    case ModifyRUN:
      modify_run(report);
      report = mod_joystick(report);
      GamecubeConsole.write(report);
      break;
    case ModifyTriggers:
      modify_trigger(report);
      report = mod_joystick(report);
      GamecubeConsole.write(report);
      break;
  }

  if (x_y){
    adjusted_last_frame = true;
  } else {
    adjusted_last_frame = false;
  }
}

// Returns the index for the appropriate custom range start
int find_bands(int x, double range[]){
  for (int i = 0; i < 8; i++){
    if (range[i] <= x && range[i + 1] > x){
      return i;
    }
  }
  return -1;
}

double map_range(int input, double in_start, double in_end, double out_start, double out_end){
  double slope = (out_end - out_start) / (in_end - in_start);
  return out_start + slope * (input - in_start);
}

// input = controller input
// type = 0 for x, 1 for y
int translate_input(int input, int isX, double range[]){
  int idx = find_bands(input, range);
  if (idx == -1){
    Serial.println("Error finding band");
    return input; // Error finding the band, just return the default input
  }
  
  double in_start = range[idx];
  double in_end = range[idx + 1];
  double out_start = default_range[idx];
  double out_end = default_range[idx + 1];

  return map_range(input, in_start, in_end, out_start, out_end);
}

Gamecube_Report_t copy_report(Gamecube_Report_t input){
  Gamecube_Report_t output;
  memcpy(&output, &input, sizeof(input));
  return output;
}
