#ifndef OUTPUT_VGA_H
#define OUTPUT_VGA_H

struct vbe_mode_info_v3
{
    uint16_t ModeAttributes;   // mode attributes
    uint8_t WinAAttributes;    // window A attributes
    uint8_t WinBAttributes;    // window B attributes
    uint16_t WinGranularity;   // window granularity in KB
    uint16_t WinSize;          // window size in KB
    uint16_t WinASegment;      // window A segment
    uint16_t WinBSegment;      // window B segment
    uint32_t WinFuncPtr;       // pointer to window function (for banked modes)
    uint16_t BytesPerScanLine; // pitch in bytes

    uint16_t XResolution;       // horizontal resolution in pixels
    uint16_t YResolution;       // vertical resolution in pixels
    uint8_t XCharSize;          // character cell width in pixels (text modes)
    uint8_t YCharSize;          // character cell height in pixels (text modes)
    uint8_t NumberOfPlanes;     // number of memory planes
    uint8_t BitsPerPixel;       // bits per pixel
    uint8_t NumberOfBanks;      // number of banks (banked modes)
    uint8_t MemoryModel;        // memory model type
    uint8_t BankSize;           // bank size in KB
    uint8_t NumberOfImagePages; // number of images
    uint8_t Reserved1;          // reserved

    uint8_t RedMaskSize; // size of direct color masks
    uint8_t RedFieldPosition;
    uint8_t GreenMaskSize;
    uint8_t GreenFieldPosition;
    uint8_t BlueMaskSize;
    uint8_t BlueFieldPosition;
    uint8_t RsvdMaskSize;
    uint8_t RsvdFieldPosition;
    uint8_t DirectColorModeInfo;

    // VBE 2.0+ protected/linear framebuffer
    uint64_t PhysBasePtr;        // physical address of linear framebuffer (64-bit in v3)
    uint64_t OffScreenMemOffset; // not always used
    uint16_t OffScreenMemSize;   // size in KB

    uint8_t Reserved2[206]; // pad to 256 bytes total
} __attribute__((packed));
bool send_init_control_to_vga_v3(struct frame_buffer *fb, uint8_t vga_id, uint64_t buf_addr)
{
    struct vbe_mode_info_v3 *mode_info = (struct vbe_mode_info_v3 *)fb->vbe_addr;
    (void)vga_id;

    if (!mode_info || mode_info->PhysBasePtr == 0)
        return false;

    
    uint64_t fb_phys = mode_info->PhysBasePtr;

    
    // uint8_t *kernel_fb_addr = map_physical_memory(fb_phys, mode_info->BytesPerScanLine * mode_info->YResolution);
    // if (!kernel_fb_addr)
    //     return false;

    
    

    
    // fb->addr = kernel_fb_addr;

    return true;
}

uint64_t init_frame_buffer_vse(uint32_t width, uint32_t height, uint32_t bpp)
{
    if (width > VGA_MAX_WIDTH || height > VGA_MAX_HEIGHT || bpp > VGA_MAX_BPP)
        return -EINVAL;
    uint64_t fb_id = get_a_free_slot((uint64_t)&fb_global, 0, MAX_FB);
    uint32_t pitch = div_ceil(width * bpp / 8, 256) * 256;
    uint64_t fb_buf_size = pitch * height;
    uint64_t total_fb_size = sizeof(struct frame_buffer) + fb_buf_size;
    uint64_t blocks_needed = div_ceil(total_fb_size, 4096);
    struct frame_buffer *fb_ptr = (struct frame_buffer *)get_continuous_free_blocks(blocks_needed);
    fb_global[fb_id] = fb_ptr;
    fb_ptr->type = FB_VGA3;
    fb_ptr->id = fb_id;
    fb_ptr->vbe_addr = (struct vbe_mode_info_v3 *)((uint64_t)fb_ptr) + sizeof(struct frame_buffer);
    fb_ptr->width = width;
    fb_ptr->height = height;
    fb_ptr->bpp = bpp;
    fb_ptr->pitch = pitch;
    fb_ptr->buf_addr = sizeof(struct frame_buffer) + (uint64_t)fb_ptr;
    uint64_t mmio_addr = send_init_control_to_vga_v3(fb_ptr, fb_id, (uint64_t)fb_ptr);
    if (mmio_addr)
        return fb_id;
    else
        return -EIO;
}
uint64_t resize_frame_buffer_vga(uint64_t fb_id, uint32_t new_width, uint32_t new_height, uint32_t new_bpp)
{
    if (new_width > VGA_MAX_WIDTH || new_height > VGA_MAX_HEIGHT || new_bpp > VGA_MAX_BPP)
        return -EINVAL;
    // new;
    uint32_t new_pitch = div_ceil(new_width * new_bpp / 8, 256) * 256;
    uint64_t new_fb_buf_size = new_pitch * new_height;
    uint64_t new_total_fb_size = sizeof(struct frame_buffer) + new_fb_buf_size;
    uint64_t new_blocks_needed = div_ceil(new_total_fb_size, 4096);
    // old;
    struct frame_buffer *old_fb_ptr = fb_global[fb_id];
    if (!old_fb_ptr)
        return -ENOENT;
    uint32_t old_pitch = old_fb_ptr->pitch;
    uint32_t old_height = old_fb_ptr->height;
    uint64_t old_fb_buf_size = old_pitch * old_height;
    uint64_t old_total_fb_size = sizeof(struct frame_buffer) + old_fb_buf_size;
    uint64_t old_blocks_needed = div_ceil(old_total_fb_size, 4096);

    struct frame_buffer *new_fb_ptr;
    // no resize
    if (old_blocks_needed == new_blocks_needed)
    {
        new_fb_ptr = old_fb_ptr;
    }
    else
    {
        new_fb_ptr = (struct frame_buffer *)get_continuous_free_blocks(new_blocks_needed);
        for (uint64_t i = 0; i < old_blocks_needed; i++)
        {
            set_memory_block_used_when_boot(((uint64_t)old_fb_ptr) / 4096 + i, false);
        }
    }
    fb_global[fb_id] = new_fb_ptr;
    new_fb_ptr->id = fb_id;
    new_fb_ptr->vbe_addr = (struct vbe_mode_info_v3 *)((uint64_t)new_fb_ptr) + sizeof(struct frame_buffer);
    new_fb_ptr->width = new_width;
    new_fb_ptr->height = new_height;
    new_fb_ptr->bpp = new_bpp;
    new_fb_ptr->pitch = new_pitch;
    if (send_init_control_to_vga_v3(new_fb_ptr, fb_id, sizeof(struct frame_buffer) + (uint64_t)new_fb_ptr))
        return fb_id;
    else
        return -EIO;
}
bool delete_frame_buffer_vga(uint64_t fb_id)
{
    struct frame_buffer *old_fb_ptr = fb_global[fb_id];
    if (!old_fb_ptr)
        return false;
    uint32_t old_pitch = old_fb_ptr->pitch;
    uint32_t old_height = old_fb_ptr->height;
    uint64_t old_fb_buf_size = old_pitch * old_height;
    uint64_t old_total_fb_size = sizeof(struct frame_buffer) + old_fb_buf_size;
    uint64_t old_blocks_needed = div_ceil(old_total_fb_size, 4096);
    for (uint64_t i = 0; i < old_blocks_needed; i++)
    {
        set_memory_block_used_when_boot(((uint64_t)old_fb_ptr) / 4096 + i, false);
    }
    fb_global[fb_id] = 0;
    return true;
}

#endif
