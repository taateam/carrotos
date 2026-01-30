#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#define ATA_DATA_PORT       0x1F0
#define ATA_SECTOR_COUNT    0x1F2
#define ATA_LBA_LOW         0x1F3
#define ATA_LBA_MID         0x1F4
#define ATA_LBA_HIGH        0x1F5
#define ATA_DRIVE_SELECT    0x1F6
#define ATA_COMMAND         0x1F7
#define ATA_STATUS_PORT     0x1F7

#define ATA_COMMAND_PORT    0x1F7
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_SR_BSY   0x80
#define ATA_SR_DRQ   0x08

#define ATA_PRIMARY_IO  0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

#define ATA_CMD_READ_PIO   0x20
#define ATA_SR_BSY         0x80
#define ATA_SR_DRDY        0x40
#define ATA_SR_DRQ         0x08

#define ATA_MASTER  0xA0
#define ATA_SLAVE   0xB0
#define ATA_IDENTIFY 0xEC

#define SECTOR_SIZE 512

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DRQ     0x08
#define ATA_SR_ERR     0x01

// Base I/O port cho 2 channel
#define ATA_PRIMARY_IO     0x1F0
#define ATA_SECONDARY_IO   0x170

#define ATA_PRIMARY_CTRL   0x3F6
#define ATA_SECONDARY_CTRL 0x376


#define ATA_IO_BASE(ch)   ((ch) == 0 ? ATA_PRIMARY_IO : ATA_SECONDARY_IO)
#define ATA_CTRL_BASE(ch) ((ch) == 0 ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL)


#define ATA_REG_FEATURES(ch)   (ATA_IO_BASE(ch) + 1)  // Features (write)
#define ATA_REG_SECCOUNT(ch)   (ATA_IO_BASE(ch) + 2)  // Sector Count

// static inline uint8_t inb(uint16_t port) {
//     uint8_t ret;
//     __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
//     return ret;
// }

static inline void outsw(uint16_t port, const void *addr, uint32_t count) {
    __asm__ volatile (
        "cld\n\trep outsw"
        : "+S"(addr), "+c"(count)
        : "d"(port)
        : "memory"
    );
}
// void ide_read_sector(uint32_t lba, uint8_t* buffer) {
//     outb(ATA_SECTOR_COUNT, 1);
//     outb(ATA_LBA_LOW, lba & 0xFF);
//     outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
//     outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
//     outb(ATA_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));
//     outb(ATA_COMMAND, 0x20); // READ SECTOR


//     while ((inb(ATA_COMMAND) & 0x08) == 0);

//     for (int i = 0; i < 256; i++) {
//         uint16_t data = inw(ATA_DATA_PORT);
//         buffer[i*2] = data & 0xFF;
//         buffer[i*2+1] = data >> 8;
//     }
// }

// static inline void outw(uint16_t port, uint16_t val) {
//     __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
// }

static inline void io_wait() {
    
    outb(0x80, 0);
}

static void ata_wait_ready() {
    while (inb(ATA_STATUS_PORT) & ATA_SR_BSY);
    while (!(inb(ATA_STATUS_PORT) & ATA_SR_DRQ));
}

// bool ide_write_sector(uint32_t lba, const void* buf) {
//     const uint16_t* data = (const uint16_t*)buf;


//     outb(ATA_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));
//     io_wait();


//     outb(ATA_SECTOR_COUNT, 1);
//     outb(ATA_LBA_LOW,  lba & 0xFF);
//     outb(ATA_LBA_MID,  (lba >> 8) & 0xFF);
//     outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
//     io_wait();


//     outb(ATA_COMMAND_PORT, ATA_CMD_WRITE_SECTORS);


//     ata_wait_ready();

//     // 5. Write 512 byte (256 words)
//     for (int i = 0; i < 256; i++) {
//         outw(ATA_DATA_PORT, data[i]);
//     }


//     while (inb(ATA_STATUS_PORT) & ATA_SR_BSY);


//     uint8_t status = inb(ATA_STATUS_PORT);

//     return true;
// }

bool is_ide_connected(uint8_t channel) {
    if (channel > 3) return false;

    
    uint16_t io_base = (channel < 2) ? ATA_PRIMARY_IO : ATA_SECONDARY_IO;
    uint8_t drive_sel = (channel % 2 == 0) ? ATA_MASTER : ATA_SLAVE;

    outb(io_base + 6, drive_sel);
    io_wait();

    // Clear registers
    outb(io_base + 2, 0);
    outb(io_base + 3, 0);
    outb(io_base + 4, 0);
    outb(io_base + 5, 0);
    outb(io_base + 7, ATA_IDENTIFY);
    io_wait();

    uint8_t status = inb(io_base + 7);
    if (status == 0) return false; 

    
    while (status & 0x80) {
        status = inb(io_base + 7);
    }

    
    if (status & 0x08) return true;

    return false;
}

bool read_sector_ide(uint8_t channel, uint32_t lba, uint8_t *buffer);

uint64_t count_partition(uint8_t ide_channel) {
    uint8_t sector[SECTOR_SIZE];

    
    if (!read_sector_ide(ide_channel, 0, sector)) {
        return -1;  
    }

    
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        return -2;  
    }

    uint64_t count = 0;

    
    for (uint64_t i = 0; i < 4; i++) {
        uint8_t *entry = sector + 0x1BE + i * 16;
        uint8_t type = entry[4];  

        if (type != 0x00) {
            count++;
        }
    }

    return count;
}
// extern uint8_t inb(uint16_t port);
// extern uint16_t inw(uint16_t port);
void io_wait();  

bool read_sector_ide(uint8_t channel, uint32_t lba, uint8_t *buffer) {
    uint16_t io_base = (channel == 0) ? ATA_PRIMARY_IO : ATA_SECONDARY_IO;

    
    while (inb(io_base + ATA_REG_STATUS) & ATA_SR_BSY);

    
    outb(io_base + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F)); // LBA mode, master
    io_wait();
    outb(io_base + ATA_REG_SECCOUNT0, 1);                   
    outb(io_base + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(io_base + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    
    while (1) {
        uint8_t status = inb(io_base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
    }

    
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(io_base + ATA_REG_DATA);
        buffer[i * 2]     = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return true;
}
// Control regs
#define ATA_REG_ALTSTATUS(ch)  (ATA_CTRL_BASE(ch) + 0) // Alternate Status (read)
#define ATA_REG_DEVCTRL(ch)    (ATA_CTRL_BASE(ch) + 0) // Device Control (write)

int ata_wait(uint8_t channel) {
    uint16_t io_base   = (channel == 0) ? ATA_DATA_PORT : ATA_SECONDARY_IO; // primary/secondary
    uint16_t ctrl_base = (channel == 0) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL; // control (altstatus)
    uint8_t status;

    
    for (int i = 0; i < 4; i++)
        inb(ctrl_base);

    while (1) {
        status = inb(io_base + 7); // thanh Write Status

        if (!(status & 0x80) && (status & 0x08))
            return 0;   
        if (status & 0x01)
            return -1;  
    }
}

uint64_t write_sector_ide(uint8_t ide_channel, uint32_t lba, uint8_t *buf) {
        uint16_t io_base, ctrl_base;
    uint8_t drive;

    
    if (ide_channel / 2 == 0) { // Primary
        io_base   = ATA_PRIMARY_IO;
        ctrl_base = ATA_PRIMARY_CTRL;
    } else { // Secondary
        io_base   = ATA_SECONDARY_IO;
        ctrl_base = ATA_SECONDARY_CTRL;
    }

    
    outb(io_base + 6, 0xE0 | (ctrl_base << 4) | ((lba >> 24) & 0x0F));

    // Write sector count = 1
    outb(io_base + 2, 1);

    
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));

    
    outb(io_base + 7, 0x30);

    
    uint8_t status;
    do {
        status = inb(io_base + 7);
    } while (status & 0x80); 

    if (status & 0x01)  
        return -1;

    // Write 256 word (512 byte)
    for (int i = 0; i < 256; i++) {
        uint16_t data = buf[i*2] | (buf[i*2+1] << 8);
        outw(io_base + 0, data);
    }

    
    do {
        status = inb(io_base + 7);
    } while ((status & 0x08) || (status & 0x80)); 

    return 0;
}

#endif // IDE_H
