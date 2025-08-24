/**
 * Ipega Diva Plus (C) CrazyRedMachine 2025
 *
 * i2c bus sniffer pico (C) Juan Schiavoni 2021
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "i2c_sniffer.pio.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "tusb_config.h"

#include "usb_descriptors.h"

#define DEBOUNCE_CYCLES 500 // number of input poll cycles to debounce (0 to disable)

#define PIN_TRIANGLE 28
#define PIN_SQUARE   27
#define PIN_CROSS    26
#define PIN_CIRCLE   22
#define PIN_L1       13
#define PIN_R1       14
#define PIN_L2       17
#define PIN_R2       16
#define PIN_SHARE    18
#define PIN_OPTIONS  19
#define PIN_HOME     20
#define PIN_R3       4
#define PIN_L3       5
#define PIN_UP       6
#define PIN_RIGHT    7
#define PIN_DOWN     8
#define PIN_LEFT     9

#define NUM_BUTTONS  17

#define PIN_MODESWITCH   15 // not considered a button

// Switch buttons
#define DPAD_UP_MASK_ON 0x00
#define DPAD_UPRIGHT_MASK_ON 0x01
#define DPAD_RIGHT_MASK_ON 0x02
#define DPAD_DOWNRIGHT_MASK_ON 0x03
#define DPAD_DOWN_MASK_ON 0x04
#define DPAD_DOWNLEFT_MASK_ON 0x05
#define DPAD_LEFT_MASK_ON 0x06
#define DPAD_UPLEFT_MASK_ON 0x07
#define DPAD_NOTHING_MASK_ON 0x08
#define A_MASK_ON 0x04
#define B_MASK_ON 0x02
#define X_MASK_ON 0x08
#define Y_MASK_ON 0x01
#define LB_MASK_ON 0x10
#define RB_MASK_ON 0x20
#define ZL_MASK_ON 0x40
#define ZR_MASK_ON 0x80
#define START_MASK_ON 0x200
#define SELECT_MASK_ON 0x100
#define L3_MASK_ON 0x400
#define R3_MASK_ON 0x800
#define HOME_MASK_ON 0x1000
#define CAPTURE_MASK_ON 0x2000
// Generic XS pad status (follows nintendo switch convention (X = up / B = down))
#define BUTTONNONE -1
#define BUTTONUP 0
#define BUTTONDOWN 1
#define BUTTONLEFT 2
#define BUTTONRIGHT 3
#define BUTTONA 4
#define BUTTONB 5
#define BUTTONX 6
#define BUTTONY 7
#define BUTTONLB 8
#define BUTTONRB 9
#define BUTTONLT 10
#define BUTTONRT 11
#define BUTTONSTART 12
#define BUTTONSELECT 13
#define AXISLX 14
#define AXISLY 15
#define AXISRX 16
#define AXISRY 17
#define BUTTONL3 18
#define BUTTONR3 19
#define BUTTONHOME 20
#define BUTTONCAPTURE 21

volatile bool g_kb_mode = false;

const uint g_but_pin[NUM_BUTTONS]  = {PIN_TRIANGLE, PIN_SQUARE, PIN_CROSS, PIN_CIRCLE, PIN_L1, PIN_R1, PIN_L2, PIN_R2, PIN_SHARE, PIN_OPTIONS, PIN_HOME, PIN_R3, PIN_L3, PIN_UP, PIN_RIGHT, PIN_DOWN, PIN_LEFT};

const uint8_t SW_KEYCODE[] = {HID_KEY_Q, HID_KEY_W, HID_KEY_O, HID_KEY_P};
const uint8_t SLIDER_KEYCODE[] = {HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_MINUS, HID_KEY_EQUAL};

uint32_t g_button_state = 0;

static uint8_t buttonStatus[22] = {0};

typedef struct joy_report_s {
    uint16_t Button; // 16 buttons; see JoystickButtons_t for bit mapping
    uint8_t  HAT;    // HAT switch; one nibble w/ unused nibble
    uint8_t  LX;     // Left  Stick X
    uint8_t  LY;     // Left  Stick Y
    uint8_t  RX;     // Right Stick X
    uint8_t  RY;     // Right Stick Y
    uint8_t  VendorSpec;
} joy_report_t;

volatile uint32_t g_full_slider;

static void update_state_joy(uint32_t button_state)
{
    static uint8_t order[] = {BUTTONX,BUTTONY,BUTTONB,BUTTONA,BUTTONLB,BUTTONRB,BUTTONLT,BUTTONRT,BUTTONSELECT,BUTTONSTART,BUTTONHOME,BUTTONR3,BUTTONL3,BUTTONUP,BUTTONRIGHT,BUTTONDOWN,BUTTONLEFT};

    /* buttons */
    for (int i=0; i<NUM_BUTTONS; i++)
    {
        if ((button_state>>i)&1) buttonStatus[order[i]] = 1;
        else buttonStatus[order[i]] = 0;
    }

    uint32_t *axis = (uint32_t *)(&(buttonStatus[AXISLX])); // effectively casting LX|LY|RX|RY as a single uint32_t
    *axis = g_full_slider;
    *axis ^= 0x80808080; //xor with center stick value for each of the 4 axis
}

joy_report_t report = {0};
void generate_report_joy(joy_report_t *report){
    memset(report, 0, sizeof(joy_report_t));
// HAT
    if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONRIGHT])){report->HAT = DPAD_UPRIGHT_MASK_ON;}
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONRIGHT])) {report->HAT = DPAD_DOWNRIGHT_MASK_ON;}
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONLEFT])) {report->HAT = DPAD_DOWNLEFT_MASK_ON;}
    else if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONLEFT])){report->HAT = DPAD_UPLEFT_MASK_ON;}
    else if (buttonStatus[BUTTONUP]) {report->HAT = DPAD_UP_MASK_ON;}
    else if (buttonStatus[BUTTONDOWN]) {report->HAT = DPAD_DOWN_MASK_ON;}
    else if (buttonStatus[BUTTONLEFT]) {report->HAT = DPAD_LEFT_MASK_ON;}
    else if (buttonStatus[BUTTONRIGHT]) {report->HAT = DPAD_RIGHT_MASK_ON;}
    else{report->HAT = DPAD_NOTHING_MASK_ON;}

// analogs
    report->LX = buttonStatus[AXISLX];
    report->LY = buttonStatus[AXISLY];
    report->RX = buttonStatus[AXISRX];
    report->RY = buttonStatus[AXISRY];

// Buttons
    if (buttonStatus[BUTTONA]) {report->Button |= A_MASK_ON;}
    if (buttonStatus[BUTTONB]) {report->Button |= B_MASK_ON;}
    if (buttonStatus[BUTTONX]) {report->Button |= X_MASK_ON;}
    if (buttonStatus[BUTTONY]) {report->Button |= Y_MASK_ON;}
    if (buttonStatus[BUTTONLB]) {report->Button |= LB_MASK_ON;}
    if (buttonStatus[BUTTONRB]) {report->Button |= RB_MASK_ON;}
    if (buttonStatus[BUTTONLT]) {report->Button |= ZL_MASK_ON;}
    if (buttonStatus[BUTTONRT]) {report->Button |= ZR_MASK_ON;}
    if (buttonStatus[BUTTONSTART]){report->Button |= START_MASK_ON;}
    if (buttonStatus[BUTTONSELECT]){report->Button |= SELECT_MASK_ON;}
    if (buttonStatus[BUTTONHOME]){report->Button |= HOME_MASK_ON;}
    if (buttonStatus[BUTTONL3]){report->Button |= L3_MASK_ON;}
    if (buttonStatus[BUTTONR3]){report->Button |= R3_MASK_ON;}
    if (buttonStatus[BUTTONCAPTURE]){report->Button |= CAPTURE_MASK_ON;}
}

void prepare_report(){
    update_state_joy(g_button_state);
    generate_report_joy(&report);
}

void send_hid() {
    if (tud_hid_ready())
    {
        tud_hid_n_report(0x00, 0x00, &report, sizeof(report));
    }
}

uint8_t nkro_report[32] = {0};
void prepare_report_kb() {
      memset(nkro_report, 0, 32);
      for (int i = 0; i < 4; i++) {
        if ((g_button_state>>i)&1) {
          uint8_t bit = SW_KEYCODE[i] % 8;
          uint8_t byte = (SW_KEYCODE[i] / 8) + 1;
          if (SW_KEYCODE[i] >= 240 && SW_KEYCODE[i] <= 247) {
            nkro_report[0] |= (1 << bit);
          } else if (byte > 0 && byte <= 31) {
            nkro_report[byte] |= (1 << bit);
          }
        }
      }

      if ((g_full_slider>>29)&7) {
        uint8_t bit = SLIDER_KEYCODE[0] % 8;
        uint8_t byte = (SLIDER_KEYCODE[0] / 8) + 1;
        if (SLIDER_KEYCODE[0] >= 240 && SLIDER_KEYCODE[0] <= 247) {
          nkro_report[0] |= (1 << bit);
        } else if (byte > 0 && byte <= 31) {
          nkro_report[byte] |= (1 << bit);
        }
      }

      for (int i = 0; i < 12; i++) {
        if ((g_full_slider>>(27-2*i))&1) {
          uint8_t bit = SLIDER_KEYCODE[i] % 8;
          uint8_t byte = (SLIDER_KEYCODE[i] / 8) + 1;
          if (SLIDER_KEYCODE[i] >= 240 && SLIDER_KEYCODE[i] <= 247) {
            nkro_report[0] |= (1 << bit);
          } else if (byte > 0 && byte <= 31) {
            nkro_report[byte] |= (1 << bit);
          }
        }
      }

      if (g_full_slider&7) {
        uint8_t bit = SLIDER_KEYCODE[11] % 8;
        uint8_t byte = (SLIDER_KEYCODE[11] / 8) + 1;
        if (SLIDER_KEYCODE[11] >= 240 && SLIDER_KEYCODE[11] <= 247) {
          nkro_report[0] |= (1 << bit);
        } else if (byte > 0 && byte <= 31) {
          nkro_report[byte] |= (1 << bit);
        }
      }
}

void send_hid_kb() {
    if (tud_hid_ready())
    {
        tud_hid_n_report(0x00, 0x00, &nkro_report, sizeof(nkro_report));
    }
}

void update_inputs() {
    uint32_t button_state = 0;
#if DEBOUNCE_CYCLES > 0

#define BOUNCE_CAN_UPDATE(x) (!x || !(--x))
    static uint16_t last_change[4] = {0};
    static uint8_t prev_state[4] = {0};
    bool input;
    for (int i=0; i<4; i++)
    {
        if (BOUNCE_CAN_UPDATE(last_change[i]))
        {
            input = gpio_get(g_but_pin[i]);
            if (!input) // only debounce on press (remove condition for debounce on release as well)
            {
                last_change[i] = DEBOUNCE_CYCLES;
            }
        } else {
            input = prev_state[i];
        }

        if (!input)
        {
            button_state |= 1<<(i);
        }
        prev_state[i] = input;
    }

    for (int i=4; i<NUM_BUTTONS; i++)
#else
    for (int i=0; i<NUM_BUTTONS; i++)
#endif
    {
        if (!gpio_get(g_but_pin[i]))
        {
            button_state |= 1<<(i);
        }
    }

    g_button_state = button_state;
}

void core1_usbtask() {
    void (*prepare_hid)() = g_kb_mode ? &prepare_report_kb : &prepare_report;
    void (*process_hid)() = g_kb_mode ? &send_hid_kb : &send_hid;
    static uint64_t last_update = 0;
    while (true) {
        uint64_t curr_time = time_us_64();
        tud_task();
        update_inputs();
        prepare_hid();
        if ( curr_time - last_update > 900 )
        {
            process_hid();
            last_update = curr_time;
        }
    }
}

void init_pins()
{
    stdio_init_all();

    for (int i=0; i<NUM_BUTTONS; i++)
    {
        gpio_init(g_but_pin[i]);
        gpio_set_dir(g_but_pin[i], GPIO_IN);
        gpio_pull_up(g_but_pin[i]);
    }

    gpio_init(PIN_MODESWITCH);
    gpio_set_dir(PIN_MODESWITCH, GPIO_IN);
    gpio_pull_up(PIN_MODESWITCH);
}

int main()
{
    set_sys_clock_khz(200000, true);
    init_pins();
    board_init();

    if (!gpio_get(PIN_HOME) && !gpio_get(PIN_CIRCLE))
    {
        reset_usb_boot(0, 0);
    }

    g_kb_mode = gpio_get(PIN_MODESWITCH); // NORMAL: keyboard mode, ARCADE: gamepad mode

    tusb_init();

    // Full speed for the PIO clock divider
    float div = 1;
    PIO pio = pio0;

    // Initialize the four state machines that decode the i2c bus states.
    uint sm_main = pio_claim_unused_sm(pio, true);
    uint offset_main = pio_add_program(pio, &i2c_main_program);
    i2c_main_program_init(pio, sm_main, offset_main, div);

    uint sm_data = pio_claim_unused_sm(pio, true);
    uint offset_data = pio_add_program(pio, &i2c_data_program);
    i2c_data_program_init(pio, sm_data, offset_data, div);

    uint sm_start = pio_claim_unused_sm(pio, true);
    uint offset_start = pio_add_program(pio, &i2c_start_program);
    i2c_start_program_init(pio, sm_start, offset_start, div);

    uint sm_stop = pio_claim_unused_sm(pio, true);
    uint offset_stop = pio_add_program(pio, &i2c_stop_program);
    i2c_stop_program_init(pio, sm_stop, offset_stop, div);

    // Start running our PIO program in the state machine
    pio_sm_set_enabled(pio, sm_main, true);
    pio_sm_set_enabled(pio, sm_start, true);
    pio_sm_set_enabled(pio, sm_stop, true);
    pio_sm_set_enabled(pio, sm_data, true);

    multicore_launch_core1(core1_usbtask);

    // Ipega slider decode loop
    bool just_started = false;
    uint8_t readidx = 0;
    uint8_t addr = 0;
    uint8_t curr_half = 0;
    uint8_t offset = 0;

    // lookup tables for quick update of g_full_slider, coordinates coincide with readidx (readidx+8 for the second half)
    // ipega has 18 zones instead of 32, so part of the slider is doubled to scale : 18 zones = 1+1+14+1+1 ==> 1+1+ 2*14 +1+1 = 32 zones
    // ( 1 2 33 44 .. 15 15 16 16 17 18 )
    const uint32_t tabf0[17] = {0, 1<<31, 3<<28,0, 3<<26, 3<<22,0, 3<<20, 3<<16, 3<<14, 3<<10, 0, 3<<8, 3<<4, 0, 3<<2, 1};
    const uint32_t tab0f[16] = {0, 1<<30, 0, 0, 3<<24, 0, 0, 3<<18, 0, 3<<12, 0, 0, 3<<6, 0, 0, 1<<1};

    uint8_t cooldown = 255;
    while (true) {
        cooldown--;
        if (!cooldown && gpio_get(PIN_MODESWITCH) != g_kb_mode)
        {
            /* change mode */
            multicore_reset_core1();
            while (!tud_disconnect()){
            sleep_ms(1000);
            };
            g_kb_mode = gpio_get(PIN_MODESWITCH);
            sleep_ms(1000);
            tud_connect();
            sleep_ms(1000);
            multicore_launch_core1(core1_usbtask);
            tud_task();
            cooldown = 255;
        }

        uint32_t val = pio_sm_get_blocking(pio, sm_main);

        // The format of the uint32_t returned by the sniffer is composed of two event
        // code bits (EV1 = Bit12, EV0 = Bit11), and when it comes to data, the nine least
        // significant bits correspond to (ACK = Bit0), and the value 8 bits
        // where (B0 = Bit1 and B7 = Bit8).
        uint32_t ev_code = (val >> 10) & 0x03;
        uint8_t  data = ((val >> 1) & 0xFF);
        //bool ack = !(val&1);

        if (ev_code == EV_START) {
            just_started = true;
        } else if (ev_code == EV_STOP) {
            addr = 0;
        } else if (ev_code == EV_DATA) {
            if (just_started)
            {
                addr = data;
                readidx = 0;
                just_started = false;
            }
            else if (addr == 0x58) // slider read request (data is slider half)
            {
                curr_half = data;
                if (curr_half == 1)
                    offset = 0;
                else
                    offset = 8;
            } else if (addr == 0x59) // slider read reply (9 bytes of data will follow)
            {
                switch (readidx){
                    case 1:
                    case 4:
                    case 7:
                        if (data & 0x0f)
                        {
                            g_full_slider |= tab0f[readidx + offset];
                        } else {
                            g_full_slider &= ~(tab0f[readidx + offset]);
                        }
                        /* fallthrough*/
                    case 2:
                    case 5:
                    case 8:
                        if (data & 0xf0)
                        {
                            g_full_slider  |= tabf0[readidx + offset];
                        } else {
                            g_full_slider &= ~(tabf0[readidx + offset]);
                        }
                    default:
                        break;
                }
            }
            readidx++;
        }
    }
}


// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
    (void)itf;
    if (report_type == HID_REPORT_TYPE_FEATURE)
    {
        /* someday */
    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}


