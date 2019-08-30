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

CGamecubeController GamecubeController(2);
CGamecubeConsole GamecubeConsole(10);
Gamecube_Data_t d = defaultGamecubeData;

double default_range[] = {1, 71, 91, 105, 128, 151, 165, 185, 240};
double x_range[] = {1, 71, 91, 125, 128, 131, 165, 185, 240};
double y_range[] = {1, 71, 91, 110, 117, 124, 165, 200, 240};

int x_cache[256]; 
int y_cache[256];

void setup() {
  setup_cache();
  // put your setup code here, to run once:
  Serial.begin(115200);
}
void setup_cache(){
  for (int i = 0; i < 256; i++){
    x_cache[i] = translate_input(i, true, x_range);
  }
  for (int i = 0; i < 256; i++){
    y_cache[i] = translate_input(i, false, y_range);
  }
}

Gamecube_Report_t report;
void take_input() {
  if (GamecubeController.read()){
    report = GamecubeController.getReport();
    report.xAxis = x_cache[report.xAxis];
    report.yAxis = y_cache[report.yAxis];
    GamecubeConsole.write(report);
  }
}

void loop() {
  take_input();
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

// input = controller input
// type = 0 for x, 1 for y
int translate_input(int input, int isX, double range[]){
  int idx = find_bands(input, range);
//  if (isX){
//    idx = x_cache[input];
//  } else {
//    idx = y_cache[input];
//  }
  if (idx == -1){
    Serial.println("Error finding band");
    return input; // Error finding the band, just return the default input
  }
  
  double in_start = range[idx];
  double in_end = range[idx + 1];
  double out_start = default_range[idx];
  double out_end = default_range[idx + 1];

  double slope = (out_end - out_start) / (in_end - in_start);
  return out_start + slope * (input - in_start);
}
