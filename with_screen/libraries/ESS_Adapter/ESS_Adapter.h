#ifndef ESS_Adapter_h
#define ESS_Adapter_h
#define HAS_SAVED 77

#include <EEPROM.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Wire.h>

class ESS_Adapter {
    public:
        void init();

        // Transforms incoming joystick and trigger values the ESS adapted settings
        // input = [x_axis, y_axis, left_trigger, right_trigger]
        void modify_input(unsigned short* input);
        void update(unsigned short* inputs);
        bool enabled = true;
        unsigned short x_range[9] = {1, 71, 91, 125, 128, 131, 165, 185, 255};
        unsigned short y_range[9] = {1, 71, 91, 110, 117, 124, 165, 200, 255};
        unsigned short ess_start = 135; // default modified from 151
        unsigned short wlk_start = 173; // default modified from 165
        unsigned short run_start = 195; // default modified from 185
        // Current values. Range is 0 -> 255
        unsigned short set_trigger = 213;
        unsigned short find_bands(unsigned short x, unsigned short range[]);
        void set_x_coords(unsigned short* coords);
        void set_y_coords(unsigned short* coords); 
        
        // Joystick calibration coordinates
        // 0 -> 7 clockwise starting at top
        // 8 = center position
        unsigned short x_coords[9] = {128, 192, 255, 192, 128, 64, 0, 64, 128};
        unsigned short y_coords[9] = {255, 192, 128, 64, 0, 64, 128, 192, 128};
        unsigned short default_x_coords[9] = {128, 192, 255, 192, 128, 64, 0, 64, 128};
        unsigned short default_y_coords[9] = {255, 192, 128, 64, 0, 64, 128, 192, 128};
        unsigned short joy_to_pct_value(unsigned short input);
        unsigned short trigger_to_pct_value(unsigned short input);
        SSD1306AsciiWire oled;
    private:
        // The default joystick cutoffs
        unsigned short default_range[9] = {1, 71, 91, 105, 128, 151, 165, 185, 255};
        // Default trigger enable range
        unsigned short default_trigger = 213;

        // Modified joystick cutoffs. We remap the input from x/y_range to default_range
        // to get the final output.
        
        // When we modify the ess/wlk/run values, we generate a cache for every
        // possible input. This is so we can just do a direct map lookup on every
        // polling to reduce latency as much as possible.
        unsigned short x_cache[256];
        unsigned short y_cache[256];

        // Sets up the x_range and y_ranges w/ current ess/wlk/run_start
        void setup_ranges();
        // Sets the center joystick value
        void center_joysticks(unsigned short x_center, unsigned short y_center);
        // Generates the x/y_cache
        void setup_cache();

        // Saves settings to EEPROM
        void save_input();
        // Loads settings from EEPROM
        void load_input();
        unsigned short translate_input(unsigned short input, unsigned short range[]);
        unsigned short map_range(double input, double in_start, double in_end, double out_start, double out_end);
        unsigned short pct_to_joy_value(unsigned short input);

        // **NOTE** We don't actually use bary coord calibrations so we could get rid of them.
        // Calculating them every poll was too time intensive so we just do a brute force
        // range mapping on min/max x/y values and the max extension of the stick.
        // A cache for all x/y values is also too much memory. Maybe if there's an inbetween
        // that would be good, but unlikely.
        // We could delete these in the future if we never use them.
        // void get_bary_coords(double* input, double* uv_1, double* uv_2, double* uv_3, double* output);
        // bool is_within_tri(double* bary_coords);

        // // Returns bary coords for tri that the input is within
        // // and the index of the correct triangle
        // void get_coord_tri_bary(unsigned short* input, double* output_bary);

        // // Returns short[6] = {uv_1.x, uv_1.y, uv_2.x, uv_2.y, uv_3.x, uv_3.y}
        // void get_coord_tri_uvs(int input, unsigned short* output);
        // void get_default_tri_uvs(int id, unsigned short* output);

        // // transforms coordinates to default range
        // void transform_coords(short* input, double* bary, unsigned short* output);
};

#endif