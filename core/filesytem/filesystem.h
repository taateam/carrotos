#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <stdint.h>
#define DIRENT_NAME_MAX_LENGTH 64
struct file
{
    uint64_t id;
    char path[512]; 
    struct file_metadata meta;
    uint64_t offset; 
    int open_flags;  
    enum file_type type;

    
    
};
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
struct dirent_in_list
{
    uint64_t d_ino;          // inode number
    uint64_t d_off;          // offset to next dirent
    unsigned short d_reclen; // length of this record
    unsigned char d_type;    // DT_REG, DT_DIR, ...
    char d_name[];           // filename (null-terminated)
};
#define MAX_FD 10480
extern struct file *fd_global[MAX_FD];
struct PartitionEntry
{
    uint8_t status;         // 0x80 = active
    uint8_t chs_first[3];   // CHS address of first absolute sector
    uint8_t type;           // Partition type
    uint8_t chs_last[3];    // CHS address of last absolute sector
    uint32_t lba_first;     // LBA of first sector
    uint32_t total_sectors; // Number of sectors in partition
} __attribute__((packed));
struct DirEntry
{
    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} __attribute__((packed));
#define SECTOR_SIZE 512
#define PARTITION_ENTRY_SIZE 16
#define MBR_PARTITION_TABLE_OFFSET 0x1BE
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE 0xAA55

#include "special_file.h"
size_t strncpy(char *dst, const char *src, size_t max_len)
{
    size_t i = 0;

    if (max_len == 0)
        return 0; 

    while (src[i] != '\0' && i < max_len - 1)
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0'; 
    return i;
}
bool split_n(const char *str, char delim, uint64_t index, char *result, size_t result_size)
{
    uint64_t count_delim = 0;
    uint64_t j = 0;
    bool in_token = false;

    for (uint64_t i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == delim)
        {
            count_delim++;
            if (count_delim > index)
                break;
            in_token = false;
            continue;
        }

        if (count_delim == index)
        {
            if (j + 1 >= result_size) 
                return false;
            result[j++] = str[i];
            in_token = true;
        }
    }

    if (count_delim < index)
    { 
        result[0] = '\0';
        return false;
    }

    result[j] = '\0';
    return true;
}

static inline char toupper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

uint32_t get_entry_cluster(struct DirEntry *e)
{
    return ((uint32_t)e->first_cluster_hi << 16) | e->first_cluster_lo;
}
bool read_sector(struct device *dev, uint32_t lba, uint8_t *buffer);
int get_partition_n(int n, struct PartitionEntry *out_entry)
{
    if (n < 0 || n >= 4)
        return -1; 

    uint8_t buf[SECTOR_SIZE];
    if (read_sector(0, 0, buf) != 0)
        return -2;

    uint16_t signature = *(uint16_t *)(buf + MBR_SIGNATURE_OFFSET);
    if (signature != MBR_SIGNATURE)
        return -3; // Not a valid MBR

    struct PartitionEntry *ptable = (struct PartitionEntry *)(buf + MBR_PARTITION_TABLE_OFFSET);
    *out_entry = ptable[n];
    return 0;
}
#define ATTR_DIRECTORY 0x10


int match_name(char *path_name, char *dir_entry_name)
{
    char fat_name[12]; // 11 + null
    memcpy(fat_name, dir_entry_name, 11);
    fat_name[11] = '\0';

    // Convert "FILE.TXT" -> "FILE    TXT"
    char formatted[12] = "           ";
    formatted[11] = '\0';
    int i = 0, j = 0;
    while (path_name[i] && path_name[i] != '.' && i < 8)
        formatted[i++] = toupper(path_name[j++]);
    if (path_name[i] == '.')
    {
        i = 8;
        j++;
        while (path_name[j] && i < 11)
            formatted[i++] = toupper(path_name[j++]);
    }
    return memcmp(fat_name, formatted, 11) == 0;
}

bool read_sector(struct device *dev, uint32_t lba, uint8_t *buffer)
{
    return dev->driver->read(dev, lba, buffer);
}
uint64_t write_sector(struct device *dev, uint32_t lba, uint8_t *buffer)
{
    return dev->driver->write(dev, lba, buffer);
}
//=====================================================================
//=====================================================================
//=====================================================================
#include "./fat32.h"
uint64_t mkfile(char *path)
{
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    return mount_global[mount_id]->part->driver->create(mount_global[mount_id]->part, child_path, false, 0);
}
uint64_t mkdir(char *path)
{
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    return mount_global[mount_id]->part->driver->create(mount_global[mount_id]->part, child_path, true, 0);
}
void stat_dirent(char *path_src, struct DirentStat *stat)
{
    uint64_t mount_id = find_mount_by_path(path_src);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path_src, child_path);
    if (mount_id == (uint64_t)-1)
        return;
    //=====================================
    struct DirentStat stat1 = mount_global[mount_id]->part->driver->stat(mount_global[mount_id]->part, child_path);
    memcpy((uint8_t *)stat, (uint8_t *)&stat1, sizeof(struct DirentStat));
}
struct DirentStat stat_dirent_ret(char *path_src)
{
    uint64_t mount_id = find_mount_by_path(path_src);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path_src, child_path);
    if (mount_id == (uint64_t)-1)
        return (struct DirentStat){0};
    //=====================================
    return mount_global[mount_id]->part->driver->stat(mount_global[mount_id]->part, child_path);
}
bool delete_dirent(char *path)
{
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    struct DirentStat stat;
    stat_dirent(path, &stat);
    if (stat.type == PATH_FILE)
        return mount_global[mount_id]->part->driver->delete(mount_global[mount_id]->part, child_path, false);
    else if (stat.type == PATH_DIRECTORY)
        return mount_global[mount_id]->part->driver->delete(mount_global[mount_id]->part, child_path, true);
    else
        return false;
}
bool rename_dirent(char *path_src, char *path_des)
{
    uint64_t src_mnt_id = find_mount_by_path(path_src);
    uint64_t dst_mnt_id = find_mount_by_path(path_des);
    if (src_mnt_id < 0)
        return false;
    if (src_mnt_id != dst_mnt_id)
        return false;
    uint64_t mount_lv = get_mount_deep_level(src_mnt_id);
    char src_child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path_src, src_child_path);
    char dst_child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path_des, dst_child_path);
    //========================================
    struct DirentStat stat;
    stat_dirent(path_src, &stat);
    if (stat.type == PATH_FILE)
        return mount_global[src_mnt_id]->part->driver->rename(mount_global[src_mnt_id]->part, src_child_path, dst_child_path, false);
    else if (stat.type == PATH_DIRECTORY)
        return mount_global[src_mnt_id]->part->driver->rename(mount_global[src_mnt_id]->part, src_child_path, dst_child_path, true);
    else
        return false;
}
bool is_dir(char *path)
{
    struct DirentStat stat1;
    stat_dirent(path, &stat1);
    return stat1.type == PATH_DIRECTORY;
}
bool list_dir(char *path, uint8_t *buf, uint64_t start, uint64_t length)
{
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    if (!is_dir(path))
        return -ENOTDIR;
    mount_global[mount_id]->part->driver->list(mount_global[mount_id]->part, child_path, buf, start, length);
}
int64_t read_file(char *path, uint8_t *buf, uint64_t start, uint64_t count)
{
    
    if (buf == 0 || count == 0)
        return -EINVAL;

    
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    mount_global[mount_id]->part->driver->read(mount_global[mount_id]->part, child_path, buf, start, count);
    return 0;
}
int64_t write_file(char *path, uint8_t *buf, uint64_t start, uint64_t count)
{
    
    if (buf == 0 || count == 0)
        return 0;

    
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //=====================================
    mount_global[mount_id]->part->driver->write(mount_global[mount_id]->part, child_path, buf, start, count);
    return 0;
}
bool path_exists(char *path)
{
    struct DirentStat ignore = {0};
    stat_dirent(path, &ignore);
    return ignore.type != PATH_NOT_FOUND;
}
bool erase_file(char *path)
{
    uint64_t mount_id = find_mount_by_path(path);
    uint64_t mount_lv = get_mount_deep_level(mount_id);
    char child_path[MAX_PATH];
    get_child_path_at_level(mount_lv, path, child_path);
    if (mount_id == (uint64_t)-1)
        return -ENOENT;
    //===================================
    return mount_global[mount_id]->part->driver->truncate(mount_global[mount_id]->part, child_path) == 0;
}
// File open flags (theo POSIX, Linux-style)
#define O_RDONLY 0x0000 
#define O_WRONLY 0x0001 
#define O_RDWR 0x0002   

#define O_APPEND 0x0400   
#define O_CREAT 0x0040    
#define O_TRUNC 0x0200    
#define O_EXCL 0x0080     
#define O_NONBLOCK 0x0800 
#define O_NOCTTY 0x0100   
#define O_SYNC 0x101000   

// Mode mask (cho open)
#define O_ACCMODE 0x0003 
#define F_DUPFD 0        // duplicate fd
#define F_GETFD 1        // get fd flags
#define F_SETFD 2        // set fd flags
#define F_GETFL 3        // get file status flags
#define F_SETFL 4        // set file status flags

void init_driver_ide()
{
    // ide driver
    uint64_t ide_driver_id = get_a_free_slot_with_size((uint64_t)&driver_global, 0, 4096 * 2, sizeof(struct driver));
    struct driver *ide_driver = &driver_global[ide_driver_id];
    strcpy(ide_driver->name, "ide");
    ide_driver->type = DEVICE_BLOCK;
    ide_driver->probe = (probe_function)&is_ide_connected;
    ide_driver->read = (read_function)&read_sector_ide;
    ide_driver->write = (write_function)&write_sector_ide;
    ide_driver->ioctl = (ioctl_function)&dummy;
    // ide device;
    struct device *ide_device = (struct device *)get_a_free_block_addr();
    ide_device->driver = ide_driver;
    strcpy(ide_device->path, "/dev/ide0");
    device_global[0] = ide_device;
};
void init_vfs_fat32()
{
    // fat32 driver
    uint64_t fat32_driver_id = get_a_free_slot_with_size((uint64_t)&partition_driver_global, 0, 4096 * 2, sizeof(struct vfs_partition_driver));
    struct vfs_partition_driver *fat32_driver = &partition_driver_global[fat32_driver_id];
    strcpy(fat32_driver->fs_name, "fat32");
    fat32_driver->probe = (probe_function_partition)&dummy;
    fat32_driver->read = (read_function_partition)&fat32_read_file_bytes;
    fat32_driver->list = (list_function_partition)&fat32_list_directory;
    fat32_driver->write = (write_function_partition)&fat32_write_file_bytes;
    fat32_driver->chmod = (chmod_function_partition)&dummy;
    fat32_driver->create = (create_function_partition)&fat32_create;
    fat32_driver->stat = (stat_function_partition)&fat32_stat;
    fat32_driver->truncate = (truncate_function_partition)&fat32_truncate_file;
    fat32_driver->delete = (delete_function_partition)&fat32_rmdir_entry;
    fat32_driver->rename = (rename_function_partition)&fat32_rename;
    // fat32_device
    struct vfs_partition *fat32_device = (struct vfs_partition *)get_a_free_block_addr();
    strcpy(fat32_device->path, "/dev/sda1");
    fat32_device->driver = fat32_driver;
    int64_t dev_id = find_device_by_name("/dev/ide0");
    if (dev_id != -1)
        return;
    fat32_device->host_device = device_global[dev_id];
    struct fat32_identification_data *fat32_id = (struct fat32_identification_data *)&fat32_device->identifier_buf;
    fat32_id->ide_channel = 0;
    fat32_id->part_id = 0;
    partition_global[0] = fat32_device;
    mount("/dev/sda", "/", 0);
};
#endif // FILESYSTEM_H;
