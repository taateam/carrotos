#ifndef FAT32_H
#define FAT32_H

#define CLUSTER_SIZE 4096
#define FAT32_EOC 0x0FFFFFF8

struct fat32_identifier_data
{
    uint8_t ide_channel;
    uint8_t part_id;
    // uint32_t root_cluster;
};
struct BPB_FAT32
{
    uint8_t jump_boot[3]; 
    uint8_t oem_name[8];  

    
    uint16_t bytes_per_sector;      // offset 11
    uint8_t sectors_per_cluster;    // offset 13
    uint16_t reserved_sector_count; // offset 14
    uint8_t num_fats;               // offset 16
    uint16_t root_entry_count;      // offset 17 (FAT12/16)
    uint16_t total_sectors_16;      // offset 19
    uint8_t media;                  // offset 21
    uint16_t fat_size_16;           // offset 22
    uint16_t sectors_per_track;     // offset 24
    uint16_t number_of_heads;       // offset 26
    uint32_t hidden_sectors;        // offset 28
    uint32_t total_sectors_32;      // offset 32
    uint32_t fat_size_32;           // offset 36
    uint16_t ext_flags;             // offset 40
    uint16_t fs_version;            // offset 42
    uint32_t root_cluster;          // offset 44
    uint16_t fs_info;               // offset 48
    uint16_t backup_boot_sector;    // offset 50
    uint8_t reserved[12];           // offset 52
    uint8_t drive_number;           // offset 64
    uint8_t reserved1;              // offset 65
    uint8_t boot_signature;         // offset 66
    uint32_t volume_id;             // offset 67
    char volume_label[11];          // offset 71
    char fs_type[8];                // offset 82
    
} __attribute__((packed));

uint64_t get_disk_id_in_device(struct vfs_partition *part)
{
    if (part == NULL)
        return -1;

    return ((struct fat32_identifier_data *)(part->identifier_buf))->ide_channel;
}

uint64_t get_part_id_in_device(struct vfs_partition *part)
{
    if (part == NULL)
        return -1;

    return ((struct fat32_identifier_data *)(part->identifier_buf))->part_id;
}
static uint32_t get_partition_lba(struct device *device, uint8_t part_index)
{
    uint8_t mbr[SECTOR_SIZE];

    
    if (!read_sector(device, 0, mbr))
    {
        return 0;
    }

    
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
    {
        return 0;
    }

    
    uint8_t *entry = mbr + 0x1BE + part_index * 16;

    
    uint32_t start_lba =
        entry[8] |
        (entry[9] << 8) |
        (entry[10] << 16) |
        (entry[11] << 24);

    return start_lba;
}
uint32_t get_root_dir_cluster(struct device *dev, uint8_t part_index)
{
    if (!dev)
        return 0;
    uint8_t boot_sector[SECTOR_SIZE];

    
    uint32_t part_lba = get_partition_lba(dev, part_index);
    if (part_lba == 0)
    {
        return 0;
        
    }

    
    if (!read_sector(dev, part_lba, boot_sector))
    {
        return 0; 
    }

    
    uint32_t root_cluster =
        boot_sector[44] |
        (boot_sector[45] << 8) |
        (boot_sector[46] << 16) |
        (boot_sector[47] << 24);

    return root_cluster;
}


int read_cluster(struct vfs_partition *part, uint32_t cluster_id, uint8_t *buf, size_t buf_size)
{
    if (cluster_id < 2)
    {
        return -1; 
    }

    uint32_t part_lba_start = get_partition_lba(part->host_device, get_part_id_in_device(part));
    uint8_t bpb_sector[SECTOR_SIZE];
    read_sector(part->host_device, part_lba_start, bpb_sector);
    const struct BPB_FAT32 *bpb = (struct BPB_FAT32 *)bpb_sector;
    uint32_t first_data_sector = bpb->reserved_sector_count + (bpb->num_fats * bpb->fat_size_32);

    uint32_t first_sector_of_cluster = part_lba_start + first_data_sector + (cluster_id - 2) * bpb->sectors_per_cluster;

    uint32_t total_cluster_bytes = bpb->sectors_per_cluster * bpb->bytes_per_sector;

    if (buf_size < total_cluster_bytes)
    {
        return -2; 
    }

    
    uint8_t sector_buf[SECTOR_SIZE];
    for (uint8_t i = 0; i < bpb->sectors_per_cluster; i++)
    {
        if (!read_sector(part->host_device, first_sector_of_cluster + i, sector_buf))
        {
            return -3; 
        }
        memcpy(buf + i * SECTOR_SIZE, sector_buf, SECTOR_SIZE);
    }

    return 0; 
}


uint32_t cluster_to_lba(struct BPB_FAT32 *bpb, uint32_t cluster)
{
    uint32_t first_data_sector = bpb->reserved_sector_count +
                                 (bpb->num_fats * bpb->fat_size_32);
    return ((cluster - 2) * bpb->sectors_per_cluster) + first_data_sector;
}

char *strchr(const char *str, int c)
{
    while (*str)
    {
        if (*str == (char)c)
            return (char *)str; 
        str++;
    }
    
    if (c == '\0')
        return (char *)str;
    return 0; 
}

void format_name_83(const char *src, char *dest)
{
    memset(dest, ' ', 11);
    const char *dot = strchr(src, '.');
    int i = 0;
    while (*src && src != dot && i < 8)
    {
        dest[i++] = toupper((unsigned char)*src++);
    }
    if (dot)
    {
        src = dot + 1;
        i = 8;
        while (*src && i < 11)
        {
            dest[i++] = toupper((unsigned char)*src++);
        }
    }
}

bool find_dir_entry(struct vfs_partition *part, const char *path, struct DirEntry *result)
{
    uint8_t sector[512];
    uint8_t sector_bpb[512];
    struct BPB_FAT32 *bpb;

    
    read_sector(part->host_device, get_partition_lba(part->host_device, get_part_id_in_device(part)), sector_bpb);
    bpb = (struct BPB_FAT32 *)sector_bpb;

    uint32_t current_cluster = bpb->root_cluster;

    
    if (path[0] == '/')
        path++;

    char name83[11];
    const char *token = path;
    char token_buf[DIRENT_NAME_MAX_LENGTH];

    uint32_t bpb_sector_id = get_partition_lba(part->host_device, get_part_id_in_device(part));
    while (*token)
    {
        
        int len = 0;
        while (token[len] && token[len] != '/')
        {
            token_buf[len] = token[len];
            len++;
        }
        token_buf[len] = '\0';

        // Format sang 8.3
        format_name_83(token_buf, name83);

        
        bool found = false;
        uint32_t lba = cluster_to_lba(bpb, current_cluster);
        for (uint8_t s = 0; s < bpb->sectors_per_cluster; s++)
        {
            read_sector(part->host_device, (bpb_sector_id + lba + s), sector);
            struct DirEntry *ent = (struct DirEntry *)sector;
            for (int e = 0; e < 16; e++)
            {
                // 512/32 = 16 entries/sector
                if (ent[e].name[0] == 0x00)
                    break; 
                if (!(ent[e].attr & 0x0F) && memcmp(ent[e].name, name83, 11) == 0)
                {
                    
                    if (token[len] == '/')
                    {
                        current_cluster = ((uint32_t)ent[e].first_cluster_hi << 16) |
                                          ent[e].first_cluster_lo;
                        found = true;
                    }
                    else
                    {
                        
                        memcpy((uint8_t *)result, (uint8_t *)&ent[e], sizeof(struct DirEntry));
                        return true;
                    }
                }
            }
        }

        if (!found && token[len] == '/')
        {
            return false; 
        }

        
        token += len;
        if (*token == '/')
            token++;
    }
    return true;
}
uint32_t find_cluster_from_path(struct vfs_partition *part, char *path)
{
    if (path[0] != '/')
        return 0;

    if (path[1] == 0x0)
        return get_root_dir_cluster(part->host_device, get_part_id_in_device(part));

    uint64_t len = strlen(path);
    char cur_path[len];
    strcpy(cur_path, path);

    char token_buf[DIRENT_NAME_MAX_LENGTH + 1];
    char *token = token_buf;
    split_n(cur_path, '/', 1, token, DIRENT_NAME_MAX_LENGTH);
    uint32_t root_dir_cluster = get_root_dir_cluster(part->host_device, get_part_id_in_device(part));
    uint32_t cluster = root_dir_cluster; 

    uint32_t j = 1;
    while (token != NULL && token[0] != '\0')
    {
        bool found = false;
        uint8_t buf[CLUSTER_SIZE]; 
        uint64_t partition_lba_start_offset = get_partition_lba(part->host_device, get_part_id_in_device(part));
        // struct BPB_FAT32 part_bpb;
        read_cluster(part, cluster, buf, CLUSTER_SIZE);

        for (int i = 0; i < CLUSTER_SIZE; i += sizeof(struct DirEntry))
        {
            struct DirEntry *entry = (struct DirEntry *)(buf + i);
            if (entry->name[0] == 0x00)
                break; 
            if ((uint8_t)entry->name[0] == 0xE5)
                continue; // deleted
            if (entry->attr == 0x0F)
                continue; // long name

            if (match_name(token, entry->name))
            {
                cluster = get_entry_cluster(entry);
                found = true;
                break;
            }
        }

        if (!found)
            return 0; 
        j++;
        split_n(cur_path, '/', j, token, DIRENT_NAME_MAX_LENGTH);
    }

    return cluster;
}
uint32_t get_next_cluster(struct vfs_partition *part, uint32_t cluster)
{
    uint32_t part_1st_lba = get_partition_lba(part->host_device, get_part_id_in_device(part));
    uint8_t part_1st_lba_sector[512];
    read_sector(part->host_device, part_1st_lba, part_1st_lba_sector);
    struct BPB_FAT32 *bpb = (struct BPB_FAT32 *)&part_1st_lba_sector;

    
    uint32_t fat_offset = cluster * 4; 
    uint32_t fat_sector = bpb->reserved_sector_count + (fat_offset / bpb->bytes_per_sector);
    uint32_t offset_in_sector = fat_offset % bpb->bytes_per_sector;

    
    uint32_t lba_start = get_partition_lba(part->host_device, get_part_id_in_device(part));
    uint32_t lba = lba_start + fat_sector; 
    uint8_t sector[SECTOR_SIZE];
    if (!read_sector(part->host_device, lba, sector))
        return 0xFFFFFFFF; 

    
    uint32_t next_cluster = *(uint32_t *)&sector[offset_in_sector];
    next_cluster &= 0x0FFFFFFF; 

    
    if (next_cluster >= FAT32_EOC)
        return 0; 

    return next_cluster;
}
void find_parent_path(const char *path, char *parent)
{
    size_t len = strlen(path);

    
    if (len == 0 || (len == 1 && path[0] == '/'))
    {
        parent[0] = '/';
        parent[1] = '\0';
        return;
    }

    
    while (len > 1 && path[len - 1] == '/')
    {
        len--;
    }

    
    size_t last_slash = 0;
    for (size_t i = len; i > 0; i--)
    {
        if (path[i - 1] == '/')
        {
            last_slash = i - 1;
            break;
        }
    }

    
    if (last_slash == 0)
    {
        parent[0] = '/';
        parent[1] = '\0';
        return;
    }

    
    strncpy(parent, path, last_slash + 1);
    parent[last_slash] = '\0';
}
uint64_t get_file_size(struct vfs_partition *part, const char *path)
{
    struct DirEntry entry;

    
    bool rs = find_dir_entry(part, path, &entry);
    if (!rs)
    {
        return (uint64_t)-1; 
    }

    
    return (uint64_t)entry.file_size;
}
int64_t fat32_read_file_bytes(struct vfs_partition *part, char *path, uint8_t *buf, uint64_t offset, uint64_t size)
{
    uint32_t cluster = find_cluster_from_path(part, path);
    if (cluster == 0)
        return -1; // file not found

    // uint32_t CLUSTER_SIZE = CLUSTER_SIZE;
    uint64_t file_size = get_file_size(part, path);
    if (offset >= file_size)
        return 0; 

    if (offset + size > file_size)
        size = file_size - offset; 

    
    uint64_t skip_bytes = offset;
    uint64_t skip_clusters = skip_bytes / CLUSTER_SIZE;
    uint64_t offset_in_cluster = skip_bytes % CLUSTER_SIZE;

    
    for (uint64_t i = 0; i < skip_clusters; i++)
    {
        cluster = get_next_cluster(part, cluster);
        if (cluster >= 0x0FFFFFF8)
            return -2; // EOF or bad cluster
    }

    uint8_t tmp[CLUSTER_SIZE];
    uint64_t total_read = 0;

    while (size > 0 && cluster < 0x0FFFFFF8)
    {
        if (read_cluster(part, cluster, tmp, CLUSTER_SIZE) != 0)
            return -3;

        uint64_t to_copy = CLUSTER_SIZE - offset_in_cluster;
        if (to_copy > size)
            to_copy = size;

        memcpy(buf + total_read, tmp + offset_in_cluster, to_copy);

        total_read += to_copy;
        size -= to_copy;
        offset_in_cluster = 0;

        if (size > 0)
            cluster = get_next_cluster(part, cluster);
    }

    return total_read;
}
enum FileItemType
{
    DIRENT_NOT_EXISTS = 0,
    FILE = 1,
    FOLDER = 2
};
struct file_item
{
    uint32_t disk_id;
    uint32_t part_id;
    char name[DIRENT_NAME_MAX_LENGTH];
    uint64_t file_1st_cluster;
    enum FileItemType type;
    uint64_t size;
} __attribute__((packed));
uint64_t fat32_list_directory(struct vfs_partition *part, char *path, uint8_t *buffer, uint64_t start, size_t size)
{
    uint32_t cluster = find_cluster_from_path(part, path);
    if (cluster == 0)
        return -ENOENT; 

    uint8_t buf[CLUSTER_SIZE];
    size_t bytes_written = 0;

    while (cluster < 0x0FFFFFF8)
    {
        if (read_cluster(part, cluster, buf, CLUSTER_SIZE) != 0)
            return -EIO; 

        uint64_t s = sizeof(struct DirEntry);
        uint64_t dirent_id = -1;
        for (int off = 0; off < CLUSTER_SIZE; off += sizeof(struct DirEntry))
        {
            dirent_id++;
            struct DirEntry *entry = (struct DirEntry *)(buf + off);

            if (entry->name[0] == 0x00)
                return bytes_written; 
            if ((uint8_t)entry->name[0] == 0xE5)
                continue; // deleted
            if (entry->attr == 0x0F)
                continue; // LFN

            if (entry->name[0] == 0x00)
                return bytes_written; 

            if (dirent_id < start)
                continue;

            struct dirent_in_list item;
            // item.disk_id = get_disk_id_in_device(part);
            // item.part_id = get_part_id_in_device(part);
            // item.file_1st_cluster = ((uint32_t)entry->first_cluster_hi << 16) | entry->first_cluster_lo;
            
            
            char name[12];
            memcpy(item.d_name, entry->name, 8);
            item.d_name[8] = '\0';
            for (int j = 7; j >= 0 && item.d_name[j] == ' '; j--)
                item.d_name[j] = '\0';

            if (entry->name[8] != ' ')
            {
                strcat(item.d_name, ".");
                char ext[4];
                memcpy(ext, entry->name + 8, 3);
                ext[3] = '\0';
                for (int j = 2; j >= 0 && ext[j] == ' '; j--)
                    ext[j] = '\0';
                strcat(item.d_name, ext);
            }

            item.d_ino = 0;
            item.d_off = off + 1;
            item.d_reclen = sizeof(struct dirent_in_list) + strlen(item.d_name) + 1;
            item.d_type = (entry->attr & 0x10) ? DT_DIR : DT_REG;

            
            if (bytes_written + sizeof(struct dirent_in_list) + strlen(item.d_name) > size)
                return bytes_written; 

            memcpy(buffer + bytes_written, (uint8_t *)&item, sizeof(struct dirent_in_list) + strlen(item.d_name));
            bytes_written += sizeof(struct dirent_in_list);
        }

        cluster = get_next_cluster(part, cluster);
    }

    return bytes_written;
}
int write_cluster(struct vfs_partition *part, uint32_t cluster_id, uint8_t *buf)
{
    
    if (cluster_id < 2)
    {
        return -1; 
    }

    uint32_t part_lba_start = get_partition_lba(part->host_device, get_part_id_in_device(part));
    uint8_t bpb_sector[SECTOR_SIZE];
    read_sector(part->host_device, part_lba_start, bpb_sector);
    const struct BPB_FAT32 *bpb = (struct BPB_FAT32 *)bpb_sector;
    uint32_t first_data_sector = bpb->reserved_sector_count + (bpb->num_fats * bpb->fat_size_32);

    uint32_t first_sector_of_cluster = part_lba_start + first_data_sector + (cluster_id - 2) * bpb->sectors_per_cluster;

    uint32_t total_cluster_bytes = bpb->sectors_per_cluster * bpb->bytes_per_sector;

    
    for (uint32_t i = 0; i < CLUSTER_SIZE / SECTOR_SIZE; i++)
    {
        if (write_sector(part->host_device, first_sector_of_cluster + i, buf + i * SECTOR_SIZE) != 0)
            return -1; 
    }

    return 0; 
}
struct DirentStat
{
    uint16_t creation_date;
    uint16_t creation_time;
    uint16_t modified_date;
    uint16_t modified_time;
    uint16_t last_access_date;
    uint16_t last_access_time;
    char name[DIRENT_NAME_MAX_LENGTH + 1];
    uint32_t size;
    enum PathType type;
    uint32_t parent_cluster_id;
} __attribute__((packed));
struct DirentStat fat32_stat(struct vfs_partition *part, char *path)
{
    struct DirentStat result = {0};
    struct DirentStat result0 = {0};
    if (path[0] != '/')
        return result0;
    if (path[0] == '/', 1 && path[1] == '\0')
    {
        memcpy(result.name, "/", 2);
        result.type = PATH_DIRECTORY;
        return result;
    }
    uint32_t len = strlen(path);
    uint32_t last_slash_pos = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (path[i] == '/')
            last_slash_pos = i;
    }
    char parent_path[len + 1];
    erase_mem8((uint64_t)&parent_path, len + 1);
    memcpy(parent_path, path, last_slash_pos);
    parent_path[last_slash_pos] = '\0';
    if (strlen(parent_path) == 0)
    {
        memcpy(parent_path, "/", 2);
    }
    char name_without_path[len - last_slash_pos + 1];
    erase_mem8((uint64_t)&name_without_path, len - last_slash_pos + 1);
    memcpy(name_without_path, path + last_slash_pos + 1, len - last_slash_pos); 
    uint32_t current_cluster = find_cluster_from_path(part, parent_path);
    uint32_t parent_cluster_id = current_cluster;
    uint8_t buffer[CLUSTER_SIZE];
    while (true)
    {
        read_cluster(part, current_cluster, buffer, CLUSTER_SIZE);
        struct DirEntry *dir = (struct DirEntry *)buffer;
        for (uint64_t i = 0; i < CLUSTER_SIZE / sizeof(struct DirEntry); i++)
        {
            char name83[len + 1];
            format_name_83(name_without_path, name83);
            if (dir[i].attr == 0x00)
                return result0; 
            if (dir[i].attr == 0xE5)
                continue;
            if (dir[i].attr == 0x0F)
                continue;
            if (memcmp(dir[i].name, name83, strlen(name_without_path)) == 0)
            {
                result.creation_date = dir[i].creation_date;
                result.creation_time = dir[i].creation_time;
                result.modified_date = dir[i].write_date;
                result.modified_time = dir[i].write_time;
                result.last_access_date = dir[i].last_access_date;
                memcpy(result.name, name_without_path, strlen(name_without_path) + 1);
                result.name[11] = '\0';
                result.size = dir[i].file_size;
                result.type = PATH_FILE;
                result.parent_cluster_id = parent_cluster_id;
                if (dir[i].attr == 0x10)
                    result.type = PATH_DIRECTORY;
                return result;
            }
        }
        current_cluster = get_next_cluster(part, current_cluster);
    }
    return result0;
}

uint32_t fat32_get_fat_entry(struct vfs_partition *part, uint32_t cluster_id)
{
    
    uint8_t sector_bpb[SECTOR_SIZE];
    read_sector(part->host_device, get_partition_lba(part->host_device, get_part_id_in_device(part)), sector_bpb);
    struct BPB_FAT32 *bpb = (struct BPB_FAT32 *)sector_bpb;
    uint32_t fat_start_lba = get_partition_lba(part->host_device, get_part_id_in_device(part)) + bpb->reserved_sector_count;
    uint32_t fat_offset = cluster_id * 4; 
    uint32_t sector = fat_start_lba + (fat_offset / bpb->bytes_per_sector);
    uint32_t offset = fat_offset % bpb->bytes_per_sector;

    uint8_t sector_buf[SECTOR_SIZE];
    read_sector(part->host_device, sector, sector_buf);

    uint32_t value = *(uint32_t *)(sector_buf + offset);
    return value & 0x0FFFFFFF; 
}


void fat32_set_fat_entry(struct vfs_partition *part, uint32_t cluster_id, uint32_t value)
{
    
    uint8_t sector_bpb[SECTOR_SIZE];
    read_sector(part->host_device, get_partition_lba(part->host_device, get_part_id_in_device(part)), sector_bpb);
    struct BPB_FAT32 *bpb = (struct BPB_FAT32 *)sector_bpb;
    uint32_t fat_start_lba = get_partition_lba(part->host_device, get_part_id_in_device(part)) + bpb->reserved_sector_count;
    uint32_t fat_offset = cluster_id * 4;
    uint32_t sector = fat_start_lba + (fat_offset / bpb->bytes_per_sector);
    uint32_t offset = fat_offset % bpb->bytes_per_sector;

    uint8_t sector_buf[SECTOR_SIZE];
    read_sector(part->host_device, sector, sector_buf);

    // update entry
    *(uint32_t *)(sector_buf + offset) = value & 0x0FFFFFFF;

    
    write_sector(part->host_device, sector, sector_buf);

    
    for (uint8_t i = 1; i < bpb->num_fats; i++)
    {
        uint32_t mirror_sector = sector + i * bpb->fat_size_32;
        write_sector(part->host_device, mirror_sector, sector_buf);
    }
}
uint32_t find_blank_cluster(struct vfs_partition *part);
void set_file_first_cluster(struct vfs_partition *part, char *path, uint32_t cluster)
{
    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return; 

    if (stat.type == PATH_DIRECTORY)
        return; 

    char this_name83[11];
    uint8_t buffer[CLUSTER_SIZE];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return;
            if (entry[i].attr == 0x20)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    struct DirEntry *this_file = &entry[i];
                    this_file->first_cluster_lo = (uint16_t)cluster;
                    this_file->first_cluster_hi = cluster >> 16;
                    write_cluster(part, current_parent_cluster_id, buffer);
                    return;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    };
}
uint32_t expand_file(struct vfs_partition *part, uint32_t cluster)
{
    uint32_t new_cluster = find_blank_cluster(part);
    if (new_cluster < 2)
        return 0; 

    fat32_set_fat_entry(part, cluster, new_cluster);
    fat32_set_fat_entry(part, new_cluster, FAT32_EOC);
    return new_cluster;
}
void set_file_size(struct vfs_partition *part, char *path, uint32_t new_size)
{
    struct DirentStat stat = fat32_stat(part, path);
    char this_name83[11];
    uint8_t buffer[CLUSTER_SIZE];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return;
            if (entry[i].attr == 0x20)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    struct DirEntry *this_file = &entry[i];
                    this_file->file_size = new_size;
                    write_cluster(part, current_parent_cluster_id, buffer);
                    return;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    };
}
int64_t fat32_write_file_bytes(struct vfs_partition *part, char *path, uint8_t *buf, uint64_t offset, uint64_t size)
{
    uint32_t cluster = find_cluster_from_path(part, path);
    if (cluster == 0)
    {
        
        
        cluster = find_blank_cluster(part);
        if (cluster < 2)
            return -6;
        fat32_set_fat_entry(part, cluster, FAT32_EOC);
        set_file_first_cluster(part, path, cluster);
    }
    uint64_t file_size = get_file_size(part, path);
    if (offset > file_size)
        return -2; 

    uint64_t skip_bytes = offset;
    uint64_t skip_clusters = skip_bytes / CLUSTER_SIZE;
    uint64_t offset_in_cluster = skip_bytes % CLUSTER_SIZE;

    
    for (uint64_t i = 0; i < skip_clusters; i++)
    {
        cluster = get_next_cluster(part, cluster);
        if (cluster >= FAT32_EOC)
            return -3; 
    }

    uint8_t tmp[CLUSTER_SIZE];
    uint64_t total_written = 0;

    // uint8_t b[4096];
    // read_cluster(part, cluster, b, CLUSTER_SIZE);
    while (size > 0 && cluster < 0x0FFFFFF8)
    {
        erase_mem8((uint64_t)&tmp, CLUSTER_SIZE);
        if (read_cluster(part, cluster, tmp, CLUSTER_SIZE) != 0)
            return -4;

        uint64_t to_copy = CLUSTER_SIZE - offset_in_cluster;
        if (to_copy > size)
            to_copy = size;

        memcpy(tmp + offset_in_cluster, buf + total_written, to_copy);

        if (write_cluster(part, cluster, tmp) != 0)
            return -5;

        total_written += to_copy;
        size -= to_copy;
        offset_in_cluster = 0;

        if (size > 0)
        {
            cluster = get_next_cluster(part, cluster);
            if (cluster >= FAT32_EOC)
            {
                cluster = expand_file(part, cluster);
                if (cluster == 0)
                    return -7; 
            }
        };
    }

    if (offset + total_written > file_size)
        set_file_size(part, path, offset + total_written); 

    return 0;
}
int fat32_truncate_file(struct vfs_partition *part, char *path)
{
    if (!path || path[0] != '/')
        return -EINVAL; 

    if (memcmp(path, "/", 2) == 0)
        return -EISDIR; 

    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return -ENOENT; 

    if (stat.type == PATH_DIRECTORY)
        return -EISDIR; 

    uint32_t this_cluster_id = find_cluster_from_path(part, path);
    while (this_cluster_id >= 2 && this_cluster_id < FAT32_EOC)
    {
        uint32_t next = fat32_get_fat_entry(part, this_cluster_id);
        fat32_set_fat_entry(part, this_cluster_id, 0x00000000); // free cluster
        this_cluster_id = next;
    }

    char this_name83[11];
    uint8_t buffer[CLUSTER_SIZE];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return -1;
            if (entry[i].attr == 0x20)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    struct DirEntry *this_file = &entry[i];
                    this_file->first_cluster_lo = 0;
                    this_file->first_cluster_hi = 0;
                    this_file->file_size = 0;
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp2a;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    };
jmp2a:
    return 0;
}
int fat32_unlink_file(struct vfs_partition *part, char *path)
{
    if (!path || path[0] != '/')
        return -EINVAL; 

    if (memcmp(path, "/", 2) == 0)
        return -EISDIR; 

    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return -ENOENT; 

    if (stat.type == PATH_DIRECTORY)
        return -EISDIR; 

    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    char this_name83[11];
    uint8_t buffer[CLUSTER_SIZE];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return -1;
            if (entry[i].attr == 0x20)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    entry[i].name[0] = 0xE5;
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp2;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    }
jmp2:
    uint32_t this_cluster_id = find_cluster_from_path(part, path);
    // loop to delete FAT entries;
    while (this_cluster_id >= 2 && this_cluster_id < FAT32_EOC)
    {
        uint32_t next = fat32_get_fat_entry(part, this_cluster_id);
        fat32_set_fat_entry(part, this_cluster_id, 0x00000000); // free cluster
        this_cluster_id = next;
    }

    return 0; 
}
int fat32_unlink_file_entry(struct vfs_partition *part, char *path)
{
    if (!path || path[0] != '/')
        return -EINVAL; 

    if (memcmp(path, "/", 2) == 0)
        return -EISDIR; 

    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return -ENOENT; 

    if (stat.type == PATH_DIRECTORY)
        return -EISDIR; 

    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    char this_name83[11];
    uint8_t buffer[CLUSTER_SIZE];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return -1;
            if (entry[i].attr == 0x20)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    struct DirEntry zero;
                    erase_mem8((uint64_t)&zero, 32);
                    memcpy((uint8_t *)&(entry[i]), (uint8_t *)&zero, 32);
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp2a;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    }
jmp2a:
    return 0; 
}
int fat32_rmdir(struct vfs_partition *part, char *path)
{
    if (!path || path[0] != '/')
        return -EINVAL; 

    if (memcmp(path, "/", 2) == 0)
        return -EINVAL; 

    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return -ENOENT; 

    if (stat.type == PATH_FILE)
        return -EINVAL; 

    uint8_t buffer[CLUSTER_SIZE];
    uint32_t this_cluster_id = find_cluster_from_path(part, path);
    uint32_t current_cluster_id = this_cluster_id;
    // check if this folder has subitem
    while (true)
    {
        if (current_cluster_id < 2)
            break;
        read_cluster(part, current_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                goto jmp1;
            if (((uint8_t)entry[i].name[0] != 0xE5) && ((uint8_t)entry[i].name[0] != 0x41))
            {
                if (memcmp(entry[i].name, ". ", 2) != 0 && memcmp(entry[i].name, ".. ", 3) != 0)
                {
                    return -1; // folder not empty;
                }
            }
        }
        current_cluster_id = get_next_cluster(part, current_cluster_id);
    }

jmp1:
    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    char this_name83[11];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return -1;
            if (entry[i].attr == 0x10)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    entry[i].name[0] = 0xE5;
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp2d;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    }
jmp2d:
    // loop to delete FAT entries;
    while (this_cluster_id >= 2 && this_cluster_id < FAT32_EOC)
    {
        uint32_t next = fat32_get_fat_entry(part, this_cluster_id);
        fat32_set_fat_entry(part, this_cluster_id, 0x00000000); // free cluster
        this_cluster_id = next;
    }

    return 0; 
}
int fat32_rmdir_entry(struct vfs_partition *part, char *path)
{
    if (!path || path[0] != '/')
        return -EINVAL; 

    if (memcmp(path, "/", 2) == 0)
        return -EINVAL; 

    struct DirentStat stat = fat32_stat(part, path);
    if (stat.type == PATH_NOT_FOUND)
        return -ENOENT; 

    if (stat.type == PATH_FILE)
        return -EINVAL; 

    uint8_t buffer[CLUSTER_SIZE];
    uint32_t this_cluster_id = find_cluster_from_path(part, path);
    uint32_t current_cluster_id = this_cluster_id;

    uint32_t current_parent_cluster_id = stat.parent_cluster_id;
    char this_name83[11];
    format_name_83(stat.name, this_name83);
    uint32_t i = 0;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint32_t i = 0; i < CLUSTER_SIZE / 32; i++)
        {
            if (entry[i].name[0] == 0x00)
                return -1;
            if (entry[i].attr == 0x10)
            {
                if (memcmp(this_name83, entry[i].name, 11) == 0)
                {
                    erase_mem8((uint64_t)&(entry[i]), 32);
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp2c;
                }
            }
        }
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    }
jmp2c:
    return 0; 
}
uint32_t find_blank_cluster(struct vfs_partition *part)
{
    uint32_t part_lba_start = get_partition_lba(part->host_device, get_part_id_in_device(part));

    uint8_t bpb_sector[SECTOR_SIZE];
    read_sector(part->host_device, part_lba_start, bpb_sector);

    const struct BPB_FAT32 *bpb = (const struct BPB_FAT32 *)bpb_sector;

    uint32_t fat_start_lba = part_lba_start + bpb->reserved_sector_count;
    uint32_t bytes_per_sector = bpb->bytes_per_sector;
    uint32_t entries_per_sector = bytes_per_sector / 4;

    uint8_t sector[SECTOR_SIZE];

    
    for (uint32_t cluster = 2; cluster < bpb->fat_size_32 * entries_per_sector; cluster++)
    {
        uint32_t sector_idx = cluster / entries_per_sector;
        uint32_t offset = cluster % entries_per_sector;

        read_sector(part->host_device, fat_start_lba + sector_idx, sector);

        uint32_t *fat_entries = (uint32_t *)sector;
        uint32_t value = fat_entries[offset] & 0x0FFFFFFF; 

        if (value == 0x00000000)
        {
            return cluster; 
        }
    }

    return 0; 
}
void fat_set_entry(struct vfs_partition *part, uint32_t cluster, uint32_t value)
{
    
    uint32_t lba = get_partition_lba(part->host_device, get_part_id_in_device(part));
    uint32_t buffer[SECTOR_SIZE];
    read_sector(part->host_device, lba, (uint8_t *)buffer);
    const struct BPB_FAT32 *bpb = (const struct BPB_FAT32 *)buffer;
    uint32_t fat_start = lba + bpb->reserved_sector_count;

    // FAT offset = cluster * 4
    uint32_t fat_offset = cluster * 4;
    uint32_t sector = fat_start + (fat_offset / SECTOR_SIZE);
    uint32_t offset = fat_offset % SECTOR_SIZE;

    uint32_t buffer1[SECTOR_SIZE];
    read_sector(part->host_device, sector, (uint8_t *)buffer1);

    uint32_t *entry = (uint32_t *)(buffer1 + offset / 4);
    *entry = value & 0x0FFFFFFF; 

    write_sector(part->host_device, sector, (uint8_t *)buffer1);

    
    for (int i = 1; i < bpb->num_fats; i++)
    {
        write_sector(part->host_device, sector + i * bpb->fat_size_32, (uint8_t *)buffer);
    }
}
int extend_dirent_cluster(struct vfs_partition *part, uint32_t last_cluster_id, uint32_t blank_cluster_id)
{
    uint32_t part_lba_start = get_partition_lba(part->host_device, get_part_id_in_device(part));

    uint8_t bpb_sector[SECTOR_SIZE];
    read_sector(part->host_device, part_lba_start, bpb_sector);
    const struct BPB_FAT32 *bpb = (const struct BPB_FAT32 *)bpb_sector;

    uint32_t fat_start_lba = part_lba_start + bpb->reserved_sector_count;
    uint32_t entries_per_sector = bpb->bytes_per_sector / 4;
    uint8_t sector[SECTOR_SIZE];

    
    {
        uint32_t sector_idx = last_cluster_id / entries_per_sector;
        uint32_t offset = last_cluster_id % entries_per_sector;

        read_sector(part->host_device, fat_start_lba + sector_idx, sector);
        uint32_t *fat_entries = (uint32_t *)sector;

        fat_entries[offset] = blank_cluster_id & 0x0FFFFFFF;

        write_sector(part->host_device, fat_start_lba + sector_idx, sector);
    }

    
    {
        uint32_t sector_idx = blank_cluster_id / entries_per_sector;
        uint32_t offset = blank_cluster_id % entries_per_sector;

        read_sector(part->host_device, fat_start_lba + sector_idx, sector);
        uint32_t *fat_entries = (uint32_t *)sector;

        fat_entries[offset] = 0x0FFFFFFF; 

        write_sector(part->host_device, fat_start_lba + sector_idx, sector);
    }

    
    {
        uint8_t zerobuf[CLUSTER_SIZE];
        erase_mem8((uint64_t)zerobuf, CLUSTER_SIZE);

        write_cluster(part, blank_cluster_id, zerobuf);
    }

    return 0;
}
uint32_t create_dirent_entry(struct vfs_partition *part, struct DirEntry *entry, unsigned char type, char *dirent_name)
{
    entry->attr = type;
    // TODO: correct time
    entry->creation_date = 0x5b13;
    entry->creation_time = 0x9c1a;
    entry->nt_reserved = 0;
    entry->creation_time_tenths = 0x44;
    entry->file_size = 0;
    uint32_t new_blank_cluster_id = find_blank_cluster(part);
    entry->first_cluster_hi = (new_blank_cluster_id >> 16) & 0xFFFF;
    entry->first_cluster_lo = new_blank_cluster_id & 0xFFFF;
    entry->last_access_date = 0x5b13;
    memcpy(entry->name, dirent_name, 11);
    entry->nt_reserved = 0;
    entry->write_date = 0x5b13;
    entry->write_time = 0x9c33;
    return new_blank_cluster_id;
}
uint64_t fat32_create(struct vfs_partition *part, char *path, bool is_dir)
{
    if (!path || path[0] != '/')
        return -1;

    // find parent
    uint32_t len = strlen(path);
    uint32_t last_slash_pos = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (path[i] == '/')
            last_slash_pos = i;
    }
    char parent_path[len + 1];
    erase_mem8((uint64_t)&parent_path, len + 1);
    memcpy(parent_path, path, last_slash_pos);
    parent_path[last_slash_pos] = '\0';
    if (strlen(parent_path) == 0)
    {
        memcpy(parent_path, "/", 2);
    }
    char name_without_path[len - last_slash_pos + 1];
    erase_mem8((uint64_t)&name_without_path, len - last_slash_pos + 1);
    memcpy((uint8_t *)name_without_path, (uint8_t *)path + last_slash_pos + 1, len - last_slash_pos); 
    uint32_t current_parent_cluster_id = find_cluster_from_path(part, parent_path);
    uint32_t parent_cluster_last_id = current_parent_cluster_id;
    uint32_t parent_cluster_id = current_parent_cluster_id;
    uint8_t buffer[CLUSTER_SIZE];
    // is dir?
    unsigned char type = 0x20;
    if (is_dir)
        type = 0x10;
    // name
    uint8_t dirent_name[11];
    format_name_83(name_without_path, dirent_name);
    // add to existing cluster of parent;
    bool found = false;
    uint32_t new_dirent_first_cluster = 0;
    while (true)
    {
        if (current_parent_cluster_id < 2)
            break;
        read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        for (uint64_t i = 0; i < CLUSTER_SIZE / sizeof(struct DirEntry); i++)
        {
            if (((unsigned char)entry[i].name[0]) == 0xE5 || ((unsigned char)entry[i].name[0]) == 0x00)
            {
                found = true;
                new_dirent_first_cluster = create_dirent_entry(part, &(entry[i]), type, dirent_name);
                write_cluster(part, current_parent_cluster_id, buffer);
                goto jmp3;
            }
        }
        parent_cluster_last_id = current_parent_cluster_id;
        current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
    }
jmp3:
    if (!found)
    {
        // find a blank cluster ;
        uint32_t blank_block_id = find_blank_cluster(part);
        // add blank cluster to current folder;
        extend_dirent_cluster(part, parent_cluster_last_id, blank_block_id);
        // write new dirent to new cluster of folder at position 0;
        erase_mem8((uint64_t)buffer, CLUSTER_SIZE);
        new_dirent_first_cluster = create_dirent_entry(part, (struct DirEntry *)buffer, type, dirent_name);
        write_cluster(part, blank_block_id, buffer);
    }
    if (is_dir)
    {
        erase_mem8((uint64_t)buffer, CLUSTER_SIZE);
        struct DirEntry *dir_entries = (struct DirEntry *)buffer;

        // 3. Write entry "."
        memset(dir_entries[0].name, ' ', 11);
        dir_entries[0].name[0] = '.';
        dir_entries[0].attr = 0x10; // directory
        dir_entries[0].nt_reserved = 0;
        // TODO: fix date time;
        dir_entries[0].creation_date = 0x5b13;
        dir_entries[0].creation_time = 0x9c1a;
        dir_entries[0].last_access_date = 0x5b13;
        dir_entries[0].creation_time_tenths = 0x44;
        dir_entries[0].file_size = 0;
        dir_entries[0].first_cluster_hi = (new_dirent_first_cluster >> 16) & 0xFFFF;
        dir_entries[0].first_cluster_lo = new_dirent_first_cluster & 0xFFFF;
        dir_entries[0].write_date = 0x5b13;
        dir_entries[0].write_time = 0x9c33;

        // 4. Write entry ".."
        memset(dir_entries[1].name, ' ', 11);
        dir_entries[1].name[0] = '.';
        dir_entries[1].name[1] = '.';
        dir_entries[1].attr = 0x10; // directory
        dir_entries[1].nt_reserved = 0;
        // TODO: fix date time;
        dir_entries[1].creation_date = 0x5b13;
        dir_entries[1].creation_time = 0x9c1a;
        dir_entries[1].last_access_date = 0x5b13;
        dir_entries[1].creation_time_tenths = 0x44;
        dir_entries[1].file_size = 0;
        dir_entries[1].first_cluster_hi = (parent_cluster_id >> 16) & 0xFFFF;
        dir_entries[1].first_cluster_lo = parent_cluster_id & 0xFFFF;
        dir_entries[1].write_date = 0x5b13;
        dir_entries[1].write_time = 0x9c33;

        
        write_cluster(part, new_dirent_first_cluster, buffer);
    }
    fat_set_entry(part, new_dirent_first_cluster, FAT32_EOC);
    return 0;
}
uint64_t fat32_rename(struct vfs_partition *part, char *path_src, char *path_des, bool is_dir)
{
    if (!path_des || path_des[0] != '/')
        return -1;
    if (!path_src || path_src[0] != '/')
        return -1;

    uint8_t buffer[CLUSTER_SIZE];
    struct DirEntry dirent_entry[1];
    {
        // find parent
        uint32_t len = strlen(path_src);
        uint32_t last_slash_pos = 0;
        for (uint64_t i = 0; i < len; i++)
        {
            if (path_src[i] == '/')
                last_slash_pos = i;
        }
        char parent_path[len + 1];
        erase_mem8((uint64_t)&parent_path, len + 1);
        memcpy(parent_path, path_src, last_slash_pos);
        parent_path[last_slash_pos] = '\0';
        if (strlen(parent_path) == 0)
        {
            memcpy(parent_path, "/", 2);
        }
        char name_without_path[len - last_slash_pos + 1];
        erase_mem8((uint64_t)&name_without_path, len - last_slash_pos + 1);
        memcpy(name_without_path, path_src + last_slash_pos + 1, len - last_slash_pos); 
        uint32_t current_parent_cluster_id = find_cluster_from_path(part, parent_path);
        uint32_t parent_cluster_id = current_parent_cluster_id;
        uint8_t dirent_name[11];
        format_name_83(name_without_path, dirent_name);
        // add to existing cluster of parent;
        bool found = false;
        uint32_t new_dirent_first_cluster = 0;
        while (true)
        {
            if (current_parent_cluster_id < 2)
                break;
            read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
            struct DirEntry *entry = (struct DirEntry *)buffer;
            for (uint64_t i = 0; i < CLUSTER_SIZE / sizeof(struct DirEntry); i++)
            {
                if (memcmp(entry[i].name, dirent_name, 11) == 0)
                {
                    found = true;
                    memcpy((uint8_t *)dirent_entry, (uint8_t *)&(entry[i]), 32);
                }
            }
            if (found)
                break;
            current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
        }
    }
    uint32_t des_parent_cluster_id = 0;
    {
        // find parent
        uint32_t len = strlen(path_des);
        uint32_t last_slash_pos = 0;
        for (uint64_t i = 0; i < len; i++)
        {
            if (path_des[i] == '/')
                last_slash_pos = i;
        }
        char parent_path[len + 1];
        erase_mem8((uint64_t)&parent_path, len + 1);
        memcpy(parent_path, path_des, last_slash_pos);
        parent_path[last_slash_pos] = '\0';
        if (strlen(parent_path) == 0)
        {
            memcpy(parent_path, "/", 2);
        }
        char name_without_path[len - last_slash_pos + 1];
        erase_mem8((uint64_t)&name_without_path, len - last_slash_pos + 1);
        memcpy(name_without_path, path_des + last_slash_pos + 1, len - last_slash_pos); 
        uint32_t current_parent_cluster_id = find_cluster_from_path(part, parent_path);
        uint32_t parent_cluster_last_id = current_parent_cluster_id;
        uint32_t parent_cluster_id = current_parent_cluster_id;
        des_parent_cluster_id = current_parent_cluster_id;
        // name
        uint8_t dirent_name[11];
        format_name_83(name_without_path, dirent_name);
        memcpy((uint8_t *)&(dirent_entry->name), dirent_name, 11);
        // add to existing cluster of parent;
        bool found = false;
        while (true)
        {
            if (current_parent_cluster_id < 2)
                break;
            read_cluster(part, current_parent_cluster_id, buffer, CLUSTER_SIZE);
            struct DirEntry *entry = (struct DirEntry *)buffer;
            for (uint64_t i = 0; i < CLUSTER_SIZE / sizeof(struct DirEntry); i++)
            {
                if (((unsigned char)entry[i].name[0]) == 0xE5 || ((unsigned char)entry[i].name[0]) == 0x00)
                {
                    found = true;
                    memcpy((uint8_t *)&(entry[i]), (uint8_t *)dirent_entry, 32);
                    write_cluster(part, current_parent_cluster_id, buffer);
                    goto jmp4;
                }
            }
            parent_cluster_last_id = current_parent_cluster_id;
            current_parent_cluster_id = get_next_cluster(part, current_parent_cluster_id);
        }
    jmp4:
        if (!found)
        {
            // find a blank cluster ;
            uint32_t blank_block_id = find_blank_cluster(part);
            // add blank cluster to current folder;
            extend_dirent_cluster(part, parent_cluster_last_id, blank_block_id);
            // write new dirent to new cluster of folder at position 0;
            erase_mem8((uint64_t)buffer, CLUSTER_SIZE);
            memcpy(buffer, (uint8_t *)dirent_entry, 32);
            write_cluster(part, blank_block_id, buffer);
        }
    }
    if (is_dir)
    {
        fat32_rmdir_entry(part, path_src);
        uint32_t des_cluster_id = find_cluster_from_path(part, path_des);
        uint8_t buffer[CLUSTER_SIZE];
        read_cluster(part, des_cluster_id, buffer, CLUSTER_SIZE);
        struct DirEntry *entry = (struct DirEntry *)buffer;
        entry[1].first_cluster_hi = (des_parent_cluster_id >> 16) & 0xFFFF;
        entry[1].first_cluster_lo = (des_parent_cluster_id) & 0xFFFF;
        write_cluster(part, des_cluster_id, buffer);
    }
    else
        fat32_unlink_file_entry(part, path_src);
}
#endif // SYSCALL1_H
