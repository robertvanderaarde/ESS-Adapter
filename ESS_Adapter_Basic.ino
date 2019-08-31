#include <Gamecube.h>
#include <GamecubeAPI.h>
#include <Gamecube_N64.h>
#include <N64.h>
#include <N64API.h>
#include <Nintendo.h>
#include <string.h>
#include "Nintendo.h"

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
  ModifyRUN
};

AdapterState state;
CGamecubeController GamecubeController(2);
CGamecubeConsole GamecubeConsole(10);
Gamecube_Data_t d = defaultGamecubeData;
Gamecube_Report_t nullReport;

double default_range[] = {1, 71, 91, 105, 128, 151, 165, 185, 240};
double x_range[] = {1, 71, 91, 125, 128, 131, 165, 185, 240};
double y_range[] = {1, 71, 91, 110, 117, 124, 165, 200, 240};

int ess_start = 151;
int wlk_start = 165;
int run_start = 185;
int x_cache[256]; 
int y_cache[256];

void setup() {
  setup_ranges();
  state = ModifiedInput; // This will have to be saved to EEPROM
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void setup_ranges(){
  x_range[5] = ess_start;
  x_range[3] = 128 - (ess_start - 128);

  x_range[6] = wlk_start;
  x_range[2] = 128 - (wlk_start - 128);

  x_range[7] = run_start;
  x_range[1] = 128 - (run_start - 128);

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
  GamecubeConsole.write(report);
}

void check_state_change(Gamecube_Report_t report){
  switch (state){
    case NormalInput:
       if (report.z && report.r && report.l && report.dup){
        state = ModifiedInput;
      }
      break;
    case ModifiedInput:
      if (report.z && report.r && report.l && report.cxAxis < 100){
        state = ModifyESS;
      } else if (report.z && report.r && report.l && report.cyAxis < 100 && report.cxAxis > 100 && report.cxAxis < 160){
        state = ModifyWLK;
      } else if (report.z && report.r && report.l && report.cxAxis > 160){
        state = ModifyRUN;
      } else if (report.z && report.r && report.l && report.ddown){
        state = NormalInput;
      }
    case ModifyESS:
    case ModifyWLK:
    case ModifyRUN:  
      if (report.b){
        state = ModifiedInput;
      }
      if (report.start){
        ess_start = default_range[5];
        wlk_start = default_range[6];
        run_start = default_range[7];
        setup_ranges();
      }
      break;
  }
}

void modify_ess(Gamecube_Report_t report){
  if (report.y){
    ess_start-=1;
    if (ess_start < 130){
      ess_start = 130;
    }
    setup_ranges();
  }
  if (report.x){
    ess_start+=1;
    if (ess_start >= wlk_start){
      ess_start = wlk_start - 1;
    }
    setup_ranges();
  }
}
void modify_wlk(Gamecube_Report_t report){
  if (report.y){
    wlk_start-=1;
    if (wlk_start <= ess_start){
      wlk_start = ess_start + 1;
    }
    setup_ranges();
  }
  if (report.x){
    wlk_start+=1;
    if (wlk_start >= run_start){
      wlk_start = run_start - 1;
    }
    setup_ranges();
  }
}
void modify_run(Gamecube_Report_t report){
  if (report.y){
    run_start-=1;
    if (run_start <= wlk_start){
      run_start = wlk_start + 1;
    }
    setup_ranges();
  }
  if (report.x){
    run_start+=1;
    if (run_start >= 240){
      run_start -= 1;
    }
    setup_ranges();
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
  new_report.r = 0;
  new_report.l = 0;
  new_report.cxAxis = 128;
  new_report.cyAxis = 128;

  if (state == ModifyESS){
    if (report.a){
      double val = map_range(default_range[5] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(x_range[5] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  if (state == ModifyWLK){
    if (report.a){
      double val = map_range(default_range[6] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(x_range[6] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  if (state == ModifyRUN){
    if (report.a){
      double val = map_range(default_range[7] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    } else {
      double val = map_range(x_range[7] - 128, 0, 128, 20, 96); 
      new_report.xAxis = (128 + val);
    }
  }
  return new_report;
}

Gamecube_Report_t report;
int frame = 0;
void loop() {
  frame++;
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
  }
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
