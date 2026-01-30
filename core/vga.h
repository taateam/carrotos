#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_ADDR 0x4000000B8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define DEFAULT_COLOR 0x07
unsigned int vga_pos = 0;

#define VGA_CTRL 0x3D4
#define VGA_DATA 0x3D5
#define CURSOR_START 0x0A
#define CURSOR_END 0x0B
#define VIDEO_MEM 0xB8000 
#define CURSOR_TOP_PIXEL 14
#define CURSOR_BOT_PIXEL 15
 void enable_cursor()
{
    outb(VGA_CTRL, CURSOR_START);
    outb(VGA_DATA, CURSOR_TOP_PIXEL);
    outb(VGA_CTRL, CURSOR_END);
    outb(VGA_DATA, CURSOR_BOT_PIXEL);
}


void set_cursor(uint16_t pos)
{
    outb(VGA_CTRL, 0x0F);
    outb(VGA_DATA, pos & 0xFF);
    outb(VGA_CTRL, 0x0E);
    outb(VGA_DATA, (pos >> 8) & 0xFF);
}
void update_cursor(){
    set_cursor(vga_pos);
}
void scroll_screen_up()
{
    uint16_t blank = (DEFAULT_COLOR << 8) | ' ';
    for (int i = 0; i < VGA_HEIGHT - 1; ++i)
    {
        for (int j = 0; j < VGA_WIDTH - 1; ++j)
        {
            ((uint16_t *)VGA_ADDR)[i * VGA_WIDTH + j] = ((uint16_t *)VGA_ADDR)[(i + 1) * VGA_WIDTH + j];
        }
    }
    for (int j = 0; j < VGA_WIDTH; ++j)
    {
        ((uint16_t *)VGA_ADDR)[(VGA_HEIGHT - 1) * VGA_WIDTH + j] = blank;
    };
    vga_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
    update_cursor();
}
static inline void print_char(char c)
{
    static volatile uint16_t *const vga = (uint16_t *)VGA_ADDR;
    // vga_pos = 0;

    if (c == '\n')
    {
        vga_pos = (vga_pos / VGA_WIDTH + 1) * VGA_WIDTH;
        update_cursor();
    }
    else
    {
        vga[vga_pos++] = 0x0F00 | (uint8_t)c; // white on black
        update_cursor();
    }

    if (vga_pos >= VGA_WIDTH * VGA_HEIGHT)
    {
        scroll_screen_up(); // wrap to top
    }
}

void clear_screen()
{
    uint16_t blank = (DEFAULT_COLOR << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
    {
        ((uint16_t *)VGA_ADDR)[i] = blank;
    }
    vga_pos = 0;
    update_cursor();
}

void delete_last_char_on_screen()
{
    if (vga_pos % VGA_WIDTH == 0)
        return; // cannot clear prev lines;
    vga_pos--;
    update_cursor();
    uint16_t blank = (DEFAULT_COLOR << 8) | ' ';
    ((uint16_t *)VGA_ADDR)[vga_pos] = blank;
}

void tsu(uint64_t x, char out[7])
{
    if (x < 1000000)
    {
        
        for (int i = 5; i >= 0; i--)
        {
            out[i] = '0' + (x % 10);
            x /= 10;
        }
        out[6] = '\0';
        return;
    }

    
    int digits = 0;
    uint64_t temp = x;
    while (temp)
    {
        temp /= 10;
        digits++;
        if (digits > 102)
        { 
            out[0] = '[';
            out[1] = 'e';
            out[2] = 'r';
            out[3] = 'r';
            out[4] = 'i';
            out[5] = ']';
            out[6] = '\0';
            return;
        }
    }

    
    int exp = digits - 3;
    temp = x;
    while (digits-- > 3)
    {
        temp /= 10;
    }

    
    uint64_t pow10 = 1;
    for (int i = 0; i < exp; i++)
    {
        pow10 *= 10;
    }
    uint64_t round = (x / pow10) % 1000;

    // Write "xxxeyy"
    out[0] = '0' + (round / 100);
    out[1] = '0' + (round / 10 % 10);
    out[2] = '0' + (round % 10);
    out[3] = 'e';
    out[4] = '0' + (exp / 10);
    out[5] = '0' + (exp % 10);
    out[6] = '\0';
}

void tsi(int64_t x, char out[8])
{
    uint64_t ux = (x < 0) ? -x : x;

    
    int digits = 0;
    uint64_t t = ux;
    while (t)
    {
        t /= 10;
        digits++;
        if (digits > 102)
        {
            out[0] = '[';
            out[1] = 'e';
            out[2] = 'r';
            out[3] = 'r';
            out[4] = 'i';
            out[5] = ']';
            out[6] = ' ';
            out[7] = '\0';
            return;
        }
    }

    
    if (ux < 1000000)
    {
        int i = 6;
        out[7] = '\0';
        for (int j = 0; j < 6; j++)
        {
            out[i--] = '0' + (ux % 10);
            ux /= 10;
        }
        if (x < 0)
            out[i] = '-';
        else
            out[i] = ' ';
        return;
    }

    
    int exp = 0;
    t = ux;
    while (t >= 1000)
    {
        t /= 10;
        exp++;
    }

    
    int lead = (int)t; 
    if (lead >= 1000)
        lead = 999; // clamp
    if (exp > 99)
    {
        for (int i = 0; i < 7; ++i)
            out[i] = '*';
        out[7] = '\0';
        return;
    }

    // Format: [-]ddd e nn
    int i = 0;
    if (x < 0)
    {
        out[i++] = '-';
    }
    else
    {
        out[i++] = ' ';
    }

    out[i++] = '0' + (lead / 100);
    out[i++] = '0' + (lead / 10 % 10);
    out[i++] = '0' + (lead % 10);
    out[i++] = 'e';
    out[i++] = '0' + (exp / 10);
    out[i++] = '0' + (exp % 10);
    out[i] = '\0';
}

// void tsf(double value, char out[11]) {
//     if (value > 9999999.99 || value < -999999.99) {
//         const char* err = "[err_f_g_]";
//         for (int i = 0; i < 10; ++i) out[i] = err[i];
//         out[10] = '\0';
//         return;
//     }


//     bool negative = value < 0;
//     if (negative) value = -value;


//     uint64_t int_part = (uint64_t)value;
//     uint64_t frac_part = (uint64_t)((value - int_part) * 100 + 0.5);


//     int i = 0;
//     if (negative) out[i++] = '-';
//     else          out[i++] = ' ';


//     for (int div = 1000000; div > 0; div /= 10) {
//         out[i++] = '0' + (int_part / div) % 10;
//     }

//     out[i++] = '.';


//     out[i++] = '0' + (frac_part / 10);
//     out[i++] = '0' + (frac_part % 10);

//     out[i] = '\0';
// }

void echo(char *str)
{
    while (*str)
    {
        print_char(*str++);
    }
}

void echoi(uint64_t input){
    char x[64];
    num_to_str(input, x);
    echo(x);
}
#endif // VGA_H
