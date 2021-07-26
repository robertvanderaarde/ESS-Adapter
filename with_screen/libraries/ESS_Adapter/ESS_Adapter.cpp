#include "ESS_Adapter.h"

void ESS_Adapter::init() {
    setup_ranges();
}

void ESS_Adapter::update(unsigned short* inputs){
    ess_start = pct_to_joy_value(inputs[0]);
    wlk_start = pct_to_joy_value(inputs[1]);
    run_start = pct_to_joy_value(inputs[2]);
    set_trigger = map_range(inputs[3], 0, 100, 0, 255);
    enabled = (bool)inputs[4];
    setup_ranges();
}

// Translates 0-100 pct to 128->255 values
unsigned short ESS_Adapter::pct_to_joy_value(unsigned short input){
    return map_range(input, 0, 100, 128, 255);
}

void ESS_Adapter::setup_ranges(){
  x_range[4] = x_coords[8];
  x_range[5] = x_coords[8] + (ess_start - 128);
  x_range[3] = x_coords[8] - (ess_start - 128);

  x_range[6] = x_coords[8] + (wlk_start - 128);
  x_range[2] = x_coords[8] - (wlk_start - 128);

  x_range[7] = x_coords[8] + (run_start - 128);
  x_range[1] = x_coords[8] - (run_start - 128);

  y_range[4] = y_coords[8];
  y_range[5] = y_coords[8] + (ess_start - 128);
  y_range[3] = y_coords[8] - (ess_start - 128);

  y_range[6] = y_coords[8] + (wlk_start - 128);
  y_range[2] = y_coords[8] - (wlk_start - 128);

  y_range[7] = y_coords[8] + (run_start - 128);
  y_range[1] = y_coords[8] - (run_start - 128);

  setup_cache();
}

unsigned short ESS_Adapter::joy_to_pct_value(unsigned short value) {
    unsigned short val = value - 128;
    return (unsigned short)(100 * ((double)val / (double)128));
}

unsigned short ESS_Adapter::trigger_to_pct_value(unsigned short value) {
    return (unsigned short)(100 * ((double)value / (double)256));
}

void ESS_Adapter::setup_cache(){
  for (unsigned short i = 0; i < 256; i++){
    i = map_range(i, x_coords[6], x_coords[2], default_x_coords[6], default_x_coords[2]);
    x_cache[i] = translate_input(i, x_range);
  }
  for (unsigned short i = 0; i < 256; i++){
    i = map_range(i, y_coords[4], y_coords[0], default_y_coords[4], default_y_coords[0]);
    y_cache[i] = translate_input(i, y_range);
  }
}

void ESS_Adapter::modify_input(unsigned short* input) {
    if (enabled) {
        input[0] = x_cache[input[0]];
        input[1] = y_cache[input[1]];
        input[2] = input[2] >= set_trigger ? 1 : 0;
        input[3] = input[3] >= set_trigger ? 1 : 0;
    } else {
        input[2] = input[2] >= default_trigger ? 1 : 0;
        input[3] = input[3] >= default_trigger ? 1 : 0;
    }
}

// Returns the index for the appropriate custom range start
unsigned short ESS_Adapter::find_bands(unsigned short x, unsigned short range[]){
  for (unsigned short i = 0; i < 8; i++){
    if (range[i] <= x && range[i + 1] > x){
      return i;
    }
  }
  return -1;
}

unsigned short ESS_Adapter::map_range(double input, double in_start, double in_end, double out_start, double out_end){
  double slope = (out_end - out_start) / (in_end - in_start);
  return (unsigned short)(out_start + slope * (input - in_start));
}

// input = controller input
// type = 0 for x, 1 for y
unsigned short ESS_Adapter::translate_input(unsigned short input, unsigned short range[]){
  unsigned short idx = find_bands(input, range);
  if (idx == -1){
    return input; // Error finding the band, just return the default input
  }
  
  unsigned short in_start = range[idx];
  unsigned short in_end = range[idx + 1];
  unsigned short out_start = default_range[idx];
  unsigned short out_end = default_range[idx + 1];

  return map_range(input, in_start, in_end, out_start, out_end);
}

void ESS_Adapter::set_x_coords(unsigned short* coords) {
    for (unsigned short i = 0; i < 10; i++){
        x_coords[i] = coords[i];
    }
}

void ESS_Adapter::set_y_coords(unsigned short* coords) {
    for (unsigned short i = 0; i < 10; i++){
        y_coords[i] = coords[i];
    }
}

// void ESS_Adapter::get_bary_coords(
//     double* input,
//     double* uv_1,
//     double* uv_2, 
//     double* uv_3,
//     double* output) {

//     output[0] = (((uv_2[1] - uv_3[1]) * (input[0] - uv_3[0])) + ((uv_3[0] - uv_2[0]) * (input[1] - uv_3[1]))) / (((uv_2[1] - uv_3[1]) * (uv_1[0] - uv_3[0])) + ((uv_3[0] - uv_2[0]) * (uv_1[1] - uv_3[1])));
//     output[1] = (((uv_3[1] - uv_1[1]) * (input[0] - uv_3[0])) + ((uv_1[0] - uv_3[0]) * (input[1] - uv_3[1]))) / (((uv_2[1] - uv_3[1]) * (uv_1[0] - uv_3[0])) + ((uv_3[0] - uv_2[0]) * (uv_1[1] - uv_3[1])));
//     output[2] = 1 - output[0] - output[1];

//     return output;
// }

// bool ESS_Adapter::is_within_tri(double* bary_coords) {
//     return bary_coords[0] > -0.05 && bary_coords[1] > -0.05 && bary_coords[2] > -0.05;
// }

// void ESS_Adapter::get_coord_tri_bary(unsigned short* input, double* output_bary) {
//     unsigned short tri[6];
//     for (int i = 0; i < 8; i++){
//         get_coord_tri_uvs(i, tri);
//         // TODO this is where it goes wrong
//         double tri_1[2] = {
//             static_cast<double>(tri[0]),
//             static_cast<double>(tri[1])
//         };
//         double tri_2[2] = {
//             static_cast<double>(tri[2]),
//             static_cast<double>(tri[3])
//         };
//         double tri_3[2] = {
//             static_cast<double>(tri[4]),
//             static_cast<double>(tri[5])
//         };
//         double input_dub[2] = {
//             static_cast<double>(input[0]),
//             static_cast<double>(input[1])
//         };
//         get_bary_coords(input_dub, tri_1, tri_2, tri_3, output_bary);
//         if (is_within_tri(output_bary)) {
//             output_bary[3] = (double)i;
//             return output_bary;
//         }
//     }
//     return output_bary;
// }

// void ESS_Adapter::get_coord_tri_uvs(int id, unsigned short* output) {
//     output[0] = x_coords[id];
//     output[1] = y_coords[id];
//     output[2] = x_coords[(id + 1) % 8];
//     output[3] = y_coords[(id + 1) % 8];
//     output[4] = x_coords[8];
//     output[5] = y_coords[8];
// }

// void ESS_Adapter::get_default_tri_uvs(int id, unsigned short* output) {
//     output[0] = default_x_coords[id];
//     output[1] = default_y_coords[id];
//     output[2] = default_x_coords[(id + 1) % 8];
//     output[3] = default_y_coords[(id + 1) % 8];
//     output[4] = default_x_coords[8];
//     output[5] = default_y_coords[8];
//     return output;
// }

// void ESS_Adapter::transform_coords(short* input, double* bary, unsigned short* output) {
//     double tmp[2] = {0, 0};
//     unsigned short tri_uvs[6] = {0, 0, 0, 0, 0, 0};
//     get_default_tri_uvs((int)bary[3], tri_uvs);
//     tmp[0] = (bary[0] * tri_uvs[0]) + (bary[1] * tri_uvs[2]) + (bary[2] * tri_uvs[4]);
//     tmp[1] = (bary[0] * tri_uvs[1]) + (bary[1] * tri_uvs[3]) + (bary[2] * tri_uvs[5]);

//     output[0] = (unsigned short)tmp[0];
//     output[1] = (unsigned short)tmp[1];
// }