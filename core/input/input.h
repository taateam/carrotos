#ifndef INPUT_H
#define INPUT_H

#define TTY_INPUT_SIZE 888
#define TTY_OUTPUT_SIZE 888
#define MAX_TTY_COUNT 512

struct input_key
{
    uint8_t modifier;
    uint8_t key;
};

enum input_action
{
    IA_NONE,
    IA_PRESS,
    IA_RELEASE,
    IA_HOLD,
    IA_MOVE,
    IA_HOLD_MOVE
};

struct input_event
{
    struct timeval time;
    uint16_t type;
    uint16_t code;
    enum input_action action;
};

#define MAX_INPUT_BUFFER_LEN 170 // =4096/18

extern struct input_event input_buffer[MAX_INPUT_BUFFER_LEN];
extern uint64_t input_buffer_w_index;
extern uint64_t input_buffer_r_index;

char scancode_to_ascii(uint8_t sc)
{
    static char scancode_map[128] = {
        0,
        27,
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        '0',
        '-',
        '=',
        '\b',
        '\t',
        'q',
        'w',
        'e',
        'r',
        't',
        'y',
        'u',
        'i',
        'o',
        'p',
        '[',
        ']',
        '\n',
        0,
        'a',
        's',
        'd',
        'f',
        'g',
        'h',
        'j',
        'k',
        'l',
        ';',
        '\'',
        '`',
        0,
        '\\',
        'z',
        'x',
        'c',
        'v',
        'b',
        'n',
        'm',
        ',',
        '.',
        '/',
        0,
        '*',
        0,
        ' ',
        0, // Caps Lock
        
    };

    // if (sc >= 0x80)
    //     return scancode_map[sc - 0x80];
    return scancode_map[sc];
}
#define LSHIFT_SC 0x2A
#define RSHIFT_SC 0x36
#define LCTRL_SC 0x1D
#define RCTRL_SC 0xE01D
#define LALT_SC 0x38
#define RALT_SC 0xE038
#define LWIN_SC 0xE05B
#define RWIN_SC 0xE05C
#define DEL_SC 0xE053
#define C_SC 0x2E 
#define Z_SC 0x2C 
#define F1_SC 0x3B
#define F2_SC 0x3C
#define F3_SC 0x3D
#define F4_SC 0x3E
#define F5_SC 0x3F
#define F6_SC 0x40
#define F7_SC 0x41
#define F8_SC 0x42
#define F9_SC 0x43
#define F10_SC 0x44
#define F11_SC 0x57
#define F12_SC 0x58

#define MOD_LSHIFT 0x01
#define MOD_RSHIFT 0x02
#define MOD_LCTRL 0x04
#define MOD_RCTRL 0x08
#define MOD_LALT 0x10
#define MOD_RALT 0x20
#define MOD_LWIN 0x40
#define MOD_RWIN 0x80 // optional

#include "tty.h"
#include "key.h"
#include "mouse.h"
#endif // { INPUT_H }
