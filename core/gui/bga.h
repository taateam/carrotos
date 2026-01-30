#ifndef OUTPUT_BVGA_H
#define OUTPUT_BVGA_H

#define BGA_INDEX 0x1CE
#define BGA_DATA 0x1CF

#define BGA_MMIO_START 0xE0000000
#define BGA_MMIO_STOP 0xE1000000
static inline void bga_write(uint16_t reg, uint16_t val)
{
    outw(BGA_INDEX, reg);
    outw(BGA_DATA, val);
}

static inline uint16_t bga_read(uint16_t reg)
{
    outw(BGA_INDEX, reg);
    return inw(BGA_DATA);
}
uint32_t get_bga_framebuffer()
{
    for (int bus = 0; bus < 256; bus++)
        for (int dev = 0; dev < 32; dev++)
            for (int func = 0; func < 8; func++)
            {
                uint16_t vendor = pci_config_read16(bus, dev, func, 0x00);
                if (vendor == 0xFFFF)
                    continue;
                uint16_t device = pci_config_read16(bus, dev, func, 0x02);
                uint8_t class = pci_config_read8(bus, dev, func, 0x0B);
                if (vendor == 0x1234 && device == 0x1111)
                {
                    uint32_t fb_addr = pci_read32(bus, dev, func, 0x10) & 0xFFFFFFF0;
                    return fb_addr;
                }
            };
    // clear flag bits
}
uint64_t send_init_control_to_bga(struct frame_buffer *fb, uint8_t vga_id, uint64_t buf_addr)
{
    (void)vga_id;

    uint32_t width = fb->width;
    uint32_t height = fb->height;
    uint32_t bpp = fb->bpp;
    bga_write(0x04, 0);      // Disable
    bga_write(0x01, width);  // Xres
    bga_write(0x02, height); // Yres
    bga_write(0x03, bpp);    // BPP
    bga_write(0x06, width);  // VirtualWidth
    bga_write(0x07, height); // VirtualHeight
    bga_write(0x04, 1);      // Enable;
    return (uint64_t)get_bga_framebuffer();
};
bool map_used_bga_mmio(uint64_t mmio_addr)
{
    uint32_t start_mapped = (mmio_addr / 4096) * 4096;
    uint32_t end_mapped = div_ceil(mmio_addr + 16 * 1024, 4096) * 4096;
    for (uint64_t i = start_mapped; i < end_mapped; i += 4096)
        set_memory_block_used_when_boot(i, true);
};
uint64_t init_frame_buffer_bga(uint32_t width, uint32_t height, uint32_t bpp)
{
    if (width > VGA_MAX_WIDTH || height > VGA_MAX_HEIGHT || bpp > VGA_MAX_BPP)
        return -EINVAL;
    uint64_t fb_id = get_a_free_slot((uint64_t)&fb_global, 0, MAX_FB);
    struct frame_buffer *fb_ptr = (struct frame_buffer *)get_a_free_block_addr();
    fb_global[fb_id] = fb_ptr;
    fb_ptr->type = FB_BGA;
    fb_ptr->id = fb_id;
    fb_ptr->vbe_addr = 0;
    fb_ptr->width = width;
    fb_ptr->height = height;
    fb_ptr->bpp = bpp;
    fb_ptr->pitch = width;
    uint64_t mmio_addr = send_init_control_to_bga(fb_ptr, fb_id, 0);
    fb_ptr->buf_addr = mmio_addr;
    uint32_t* mmio_ptr=(uint32_t*)mmio_addr;
    // mmio_ptr[0]= 0x0000FF;
    // for (uint64_t i=1;i<800;i++){
    //     mmio_ptr[i]= 0xFF0000; //red;
    // }
    // for (uint64_t i=801;i<1600;i++){
    //     mmio_ptr[i]= 0x00FF00; //red;
    // }
    // for (uint64_t i=1601;i<2400;i++){
    //     mmio_ptr[i]= 0x0000FF; //red;
    // }
    map_used_bga_mmio(mmio_addr);
    if (mmio_addr)
        return fb_id;
    else
        return -EIO;
}
uint64_t resize_frame_buffer_bga(uint64_t fb_id, uint32_t new_width, uint32_t new_height, uint32_t new_bpp)
{
    if (new_width > VGA_MAX_WIDTH || new_height > VGA_MAX_HEIGHT || new_bpp > VGA_MAX_BPP)
        return -EINVAL;
    struct frame_buffer *fb_ptr = fb_global[fb_id];
    fb_global[fb_id] = fb_ptr;
    fb_ptr->type = FB_BGA;
    fb_ptr->id = fb_id;
    fb_ptr->vbe_addr = 0;
    fb_ptr->width = new_width;
    fb_ptr->height = new_height;
    fb_ptr->bpp = new_bpp;
    fb_ptr->pitch = new_width;
    uint64_t mmio_addr = send_init_control_to_bga(fb_ptr, fb_id, BGA_MMIO_START);
    fb_ptr->buf_addr = mmio_addr;
    // map_used_bga_mmio(mmio_addr);
    if (mmio_addr)
        return fb_id;
    else
        return -EIO;
}
bool delete_frame_buffer_bga(uint64_t fb_id)
{
    struct frame_buffer *old_fb_ptr = fb_global[fb_id];
    if (!old_fb_ptr)
        return false;
    set_memory_block_used_when_boot(((uint64_t)old_fb_ptr), false);
    fb_global[fb_id] = 0;
    return true;
}
#endif
