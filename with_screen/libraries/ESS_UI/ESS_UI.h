#ifndef ESS_UI_h
#define ESS_UI_h

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Wire.h>

class ESS_UI
{
    enum UIState { IDLE, MAIN_MENU, MODIFY, CALIBRATE };
    public:
        // ESS_UI();
        void init(bool enabled);
        void update(unsigned short ess, unsigned short wlk, unsigned short run, unsigned short trigger);
        String joystick_to_pct_str(unsigned short value);
        String trigger_to_pct_str(unsigned short value);
        bool get_enabled();
        void toggle_enabled();
        bool in_menu();
        void enter_menu();
        void exit_menu();
        void send_inputs(unsigned short inputs[]);
        unsigned short* get_inputs();
        void draw_text(String text);
        UIState state = IDLE;
        bool enabled;
        void set_enabled(bool enable);
        SSD1306AsciiWire oled;
        unsigned short* get_x_coords();
        unsigned short* get_y_coords();

        // Joystick Calibration
        // Sort of inefficient - maybe we just need a reference to the
        // adapter coords instead? Could just pass a pointer.
        unsigned short* x_coords;
        unsigned short* y_coords;  
        unsigned short* default_x_coords;
        unsigned short* default_y_coords;
    private:
        unsigned short ess_start;
        unsigned short wlk_start;
        unsigned short run_start;
        unsigned short trigger_start;
        void draw_header();
        void draw_main_menu();
        void draw_idle_screen();
        void draw_values();
        void draw_modify_menu();
        void draw_calibration_menu();
        void draw_calibration_values();


        unsigned short selected_button_index;
        unsigned short modify_input_index;
        bool currently_modifying;
        bool showing_calibration;
        void modify_inputs(unsigned short val, unsigned short index);
};

#endif