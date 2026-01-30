#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

struct mouse_packet
{
    uint8_t buttons; // Byte 0: left/middle/right + sign/overflow
    int8_t x;        // Byte 1: delta X
    int8_t y;        // Byte 2: delta Y
};
extern struct mouse_packet mouse_buffer[4096 / 3];
static uint8_t mouse_int_global = 0;
static bool mouse_x_sign = true, mouse_y_sign = true;
static bool mouse_left_button_down = false, mouse_right_button_down = false, mouse_middle_button_down = false;
static uint8_t mouse_x_delta = 0, mouse_y_delta = 0;
static uint64_t mouse_buffer_head = 0, mouse_buffer_tail = 0;
uint8_t create_buttons_byte(bool left, bool middle, bool right,
                            bool x_sign, bool y_sign)
{
    uint8_t b = 0;
    if (left)
        b |= 0x1;
    if (right)
        b |= 0x2;
    if (middle)
        b |= 0x4;
    if (x_sign)
        b |= 0x10;
    if (y_sign)
        b |= 0x20;
    b |= 0x8; // bit always 1
    return b;
}
uint64_t read_mouse_buffer(uint8_t *out_buf, uint64_t len)
{
    uint64_t read_count = fifo_read24((uint8_t *)mouse_buffer, &mouse_buffer_tail, &mouse_buffer_head, 4096 / 3, len, out_buf);
    return read_count;
}
uint64_t write_mouse_buffer(bool left, bool middle, bool right,
                            bool x_sign, bool y_sign,
                            uint8_t x_delta, uint8_t y_delta)
{
    struct mouse_packet pkt;
    pkt.buttons = create_buttons_byte(left, middle, right, x_sign, y_sign);
    pkt.x = x_delta;
    pkt.y = y_delta;
    uint64_t write_count = fifo_write24((uint8_t *)mouse_buffer, &mouse_buffer_tail, &mouse_buffer_head, 4096 / 3, 3, (uint8_t *)&pkt);
    return write_count; // success
}
//===============================================================================================
int64_t syscall_read_mouse(uint8_t *buf, uint64_t len, bool non_blocking_flags)
{
    if (len < 3)
        return -EINVAL;
    uint64_t read_count = read_mouse_buffer(buf, len / 3);
    if (read_count)
        return read_count * 3;
    if (non_blocking_flags)
        return -EAGAIN;
    scheduler_yeild();
    read_count = read_mouse_buffer(buf, len / 3);
    return read_count * 3; // bytes read
}
void isr_mouse_handler(uint8_t mouse_byte)
{
    // echo("~");
    // echoi(mouse_byte);
    // echo("\n");
    //===========================================
    uint8_t mouse_int = mouse_int_global;
    mouse_int_global = (mouse_int_global + 1) % 3;
    if (mouse_int == 0)
    {
        if (!(mouse_int & (1 << 3)))
        {
            
            return;
        }
        mouse_x_sign = mouse_byte & 0x10;
        mouse_y_sign = mouse_byte & 0x20;
        mouse_left_button_down = mouse_byte & 0x1;
        mouse_right_button_down = mouse_byte & 0x2;
        mouse_middle_button_down = mouse_byte & 0x4;
    }
    else if (mouse_int == 1)
    {
        mouse_x_delta = mouse_byte;
    }
    else if (mouse_int == 2)
    {
        mouse_y_delta = mouse_byte;
        write_mouse_buffer(mouse_left_button_down, mouse_middle_button_down, mouse_right_button_down,
                           mouse_x_sign, mouse_y_sign,
                           mouse_x_delta, mouse_y_delta);
    }
    return;
};
#endif // { MOUSE_H }
