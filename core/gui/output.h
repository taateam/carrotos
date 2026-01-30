#ifndef output_H
#define output_H

#define MAX_FB 128
#define VGA_MAX_WIDTH 1920
#define VGA_MAX_HEIGHT 1080
#define VGA_MAX_BPP 32
extern struct frame_buffer *fb_global[MAX_FB];
#define VBE_REG_FB_BASE_LO 0x0D
#define VBE_REG_FB_BASE_HI 0x0E

#define VBE_INDEX_PORT 0x01CE
#define VBE_DATA_PORT 0x01CF
#define VBE_REG_FB_BASE_LO 0x0D
#define VBE_REG_FB_BASE_HI 0x0E

struct vbe_mode_info_v3;
enum fb_type
{
    FB_NOT_EXISTS,
    FB_VGA3,
    FB_BGA
};
struct frame_buffer
{
    enum fb_type type;
    uint64_t id;
    void *vbe_addr; 
    uint32_t width;
    uint32_t height;
    uint8_t bpp;    // bits per pixel
    uint32_t pitch; // include line padding, use for allignment, bytes per row;
    uint64_t buf_addr;
    uint8_t padding[221]; // padding for alignment 256 bytes;
} __attribute__((packed));

#include "vga.h"
#include "bga.h"
uint64_t write_frame_buffer(uint64_t res_id, uint8_t *user_space_buf, uint64_t count)
{
    struct frame_buffer *fb = fb_global[res_id];
    if (!fb)
        return (uint64_t)-1;

    uint32_t bytes_per_pixel = fb->bpp / 8;
    uint64_t row_bytes_no_padding = fb->width * bytes_per_pixel;
    uint64_t fb_size = fb->pitch * fb->height;

    if (count > fb_size)
        count = fb_size;

    uint32_t total_rows = count / row_bytes_no_padding;
    if (total_rows > fb->height)
        total_rows = fb->height;

    for (uint32_t row = 0; row < total_rows; row++)
    {
        memcpy((uint8_t *)(fb->buf_addr + row * fb->pitch), (uint8_t *)(user_space_buf + row * row_bytes_no_padding), row_bytes_no_padding);
    }
    return total_rows * row_bytes_no_padding;
}
uint64_t read_frame_buffer(uint64_t res_id, uint8_t *user_space_buf, uint64_t count)
{
    struct frame_buffer *fb = fb_global[res_id];
    if (!fb)
        return (uint64_t)-1;

    uint32_t bytes_per_pixel = fb->bpp / 8;
    uint64_t row_byte_no_padding = fb->width * bytes_per_pixel;
    uint64_t fb_size = row_byte_no_padding * fb->height;

    if (count > fb_size)
        count = fb_size;

    uint32_t total_rows = count / row_byte_no_padding;
    if (total_rows > fb->height)
        total_rows = fb->height;

    for (uint32_t row = 0; row < total_rows; row++)
    {
        memcpy(
            (uint8_t *)(fb->buf_addr + row * fb->pitch),
            (uint8_t *)(user_space_buf + row * row_byte_no_padding),
            row_byte_no_padding);
    }
    return total_rows * row_byte_no_padding;
}
uint64_t init_gui()
{
    return init_frame_buffer_bga(1920, 1080, 32);
}
uint64_t sys_write_frame_buffer(uint64_t res_id, uint8_t *buf, uint64_t count)
{
    return write_frame_buffer(res_id, buf, count);
}
uint64_t sys_read_frame_buffer(uint64_t res_id, uint8_t *buf, uint64_t count)
{
    return write_frame_buffer(res_id, buf, count);
}
#endif
