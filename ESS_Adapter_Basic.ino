#include <Gamecube.h>
#include <GamecubeAPI.h>
#include <Gamecube_N64.h>
#include <N64.h>
#include <N64API.h>
#include <Nintendo.h>
#include <string.h>
#include "Nintendo.h"

CGamecubeController GamecubeController(2);
CGamecubeConsole GamecubeConsole(10);
Gamecube_Data_t d = defaultGamecubeData;

double default_range[] = {1, 71, 91, 105, 128, 151, 165, 185, 240};
double x_range[] = {1, 71, 91, 125, 128, 131, 165, 185, 240};
double y_range[] = {1, 71, 81, 110, 117, 131, 180, 200, 240};

int x_cache[256]; 
int y_cache[256];

void setup() {
  setup_cache();
  // put your setup code here, to run once:
  Serial.begin(115200);
}
void setup_cache(){
  for (int i = 0; i < 256; i++){
    x_cache[i] = find_bands(i, x_range);
  }
  for (int i = 0; i < 256; i++){
    y_cache[i] = find_bands(i, y_range);
  }
}

Gamecube_Report_t input_report;
Gamecube_Report_t send_report;

Gamecube_Report_t copy_report(Gamecube_Report_t input){
  Gamecube_Report_t output;
  memcpy(&output, &input, sizeof(input));
  return output;
}

void take_input() {
  if (GamecubeController.read()){
    input_report = GamecubeController.getReport();
    make_output();
  }
}

void make_output(){
  Gamecube_Report_t output_report = copy_report(input_report);
  int xAxis = translate_input(output_report.xAxis, 1, x_range);
  int yAxis = translate_input(output_report.yAxis, 0, y_range);
  output_report.xAxis = xAxis;
  output_report.yAxis = yAxis;
  GamecubeConsole.write(output_report);
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
  int idx = 0;
  if (isX){
    idx = x_cache[input];
  } else {
    idx = y_cache[input];
  }
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
