#ifndef DRIVER_H
#define DRIVER_H

#define MAX_PATH 4 * 256

struct device;
typedef uint64_t (*probe_function)(struct device *dev);
typedef uint64_t (*read_function)(struct device *dev, uint32_t lba_id, uint8_t *buffer);
typedef uint64_t (*write_function)(struct device *dev, uint32_t lba_id, uint8_t *buffer);
typedef uint64_t (*ioctl_function)(struct device *dev, uint64_t command, void *arg);
enum DeviceType
{
    DEVICE_NOT_FOUND = 0,
    DEVICE_BLOCK = 1,
    DEVICE_CHAR = 2,
    DEVICE_NOT_RW,
    DEVICE_OTHER
};

struct driver
{
    char name[64];
    enum DeviceType type;
    probe_function probe;
    read_function read;
    write_function write;
    ioctl_function ioctl;
};
struct device
{
    char path[256];
    uint8_t identifier_buf[64];
    uint8_t driver_data[128];
    struct driver *driver;
    uint8_t zero_buf[4096 - 256 - 64 - 128 - 8 - 8];
};
uint64_t MAX_DRIVER = 4096 * 2 / sizeof(struct driver);
extern struct driver driver_global[];
#define MAX_DEVICE 1024
extern struct device *device_global[MAX_DEVICE];
uint64_t dummy(void* input) { return 0; }
//==================================================================
#include "ide.h"
struct driver* find_driver_by_name(char* name) {
    for (int i = 0; i < MAX_DRIVER; i++) {
        if (driver_global[i].type && strcmp(driver_global[i].name, name) == 0) {
            return &driver_global[i];
        }
    }
    return NULL;
}
int64_t find_device_by_name(char* name) {
    for (int i = 0; i < MAX_DEVICE; i++) {
        if (device_global[i] != NULL && strcmp(device_global[i]->path, name) == 0) {
             return i;
        }
    }
    return -1;
}
#endif // DRIVER_H
