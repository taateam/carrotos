#ifndef VFS_H
#define VFS_H

struct DirentStat;
struct vfs_partition;
typedef uint64_t (*probe_function_partition)(struct vfs_partition *part);
typedef uint64_t (*read_function_partition)(struct vfs_partition *part, char *path, void *buf, uint64_t start_offset, uint64_t size);
typedef uint64_t (*list_function_partition)(struct vfs_partition *part, char *path, void *buf, uint64_t start_offset, uint64_t size);
typedef uint64_t (*write_function_partition)(struct vfs_partition *part, char *path, void *buf, uint64_t start_offset, uint64_t size);
typedef uint64_t (*chmod_function_partition)(struct vfs_partition *part, uint64_t command, void *arg);
typedef uint64_t (*create_function_partition)(struct vfs_partition *part, char *path, bool is_dir, uint16_t mode);
typedef struct DirentStat (*stat_function_partition)(struct vfs_partition *part, char *path);
typedef uint64_t (*truncate_function_partition)(struct vfs_partition *part, char *path);
typedef uint64_t (*delete_function_partition)(struct vfs_partition *part, char *path, bool is_dir);
typedef uint64_t (*rename_function_partition)(struct vfs_partition *part, char *src, char *dest, bool is_file);
enum PathType
{
    PATH_NOT_FOUND = 0,
    PATH_FILE = 1,
    PATH_DIRECTORY = 2
};

struct vfs_partition_driver
{
    char fs_name[64];
    probe_function_partition probe;
    read_function_partition read;
    list_function_partition list;
    write_function_partition write;
    chmod_function_partition chmod;
    create_function_partition create;
    stat_function_partition stat;
    truncate_function_partition truncate;
    delete_function_partition delete;
    rename_function_partition rename;
};
struct vfs_partition
{
    char path[MAX_PATH];
    uint8_t identifier_buf[64];
    uint8_t driver_data[128];
    struct vfs_partition_driver *driver;
    struct device *host_device;
    uint8_t zero_buf[4096 - 8 - MAX_PATH - 64 - 128 - 8 - 8];
};
struct mount_point
{
    char src[MAX_PATH];
    char dest[MAX_PATH];
    int flags;
    struct vfs_partition *part;
    uint8_t zero_buf[4096 - 2 * MAX_PATH - 4];
};
#define MAX_MOUNT_POINT 512
extern struct mount_point *mount_global[MAX_MOUNT_POINT];
uint64_t MAX_PARTITION_DEVICE_DRIVER = 4096 * 2 / sizeof(struct vfs_partition_driver);
extern struct vfs_partition_driver partition_driver_global[];
#define MAX_DEVICE 1024
extern struct vfs_partition *partition_global[MAX_DEVICE];
uint64_t find_part_by_name(char *name)
{
    for (uint64_t i = 0; i < MAX_DEVICE; i++)
    {
        if (partition_global[i] != NULL && strcmp(partition_global[i]->path, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
int64_t mount(char *src, char *dest, int flags)
{
    uint64_t id = get_a_free_slot((uint64_t)&mount_global, 0, MAX_MOUNT_POINT);
    if (id == MAX_MOUNT_POINT)
    {
        echo("No free mount point slot!!!");
        return -1;
    }
    uint64_t device_id = 0;
    for (uint64_t i = 0; i < MAX_DEVICE; i++)
    {
        if (strcmp(partition_global[i]->path, src) == 0)
        {
            device_id = i;
            break;
        }
    }
    if (device_id == MAX_DEVICE)
    {
        echo("Device not found for mount source!!!");
        return -1;
    }
    struct mount_point *mp = (struct mount_point *)get_a_free_block_addr();
    mp->flags = flags;
    strcpy(mp->src, src);
    strcpy(mp->dest, dest);
    uint64_t part_id = find_part_by_name(mp->src);
    mp->part = partition_global[part_id];
    mount_global[id] = mp;
    return id;
}
void get_parent_path_at_level(uint64_t level, char *absolute_path, char *out_path)
{
    uint64_t str_len = strlen(absolute_path);
    uint64_t count_all_seperator = 0;
    for (uint64_t j = 0; j < str_len; j++)
    {
        if (absolute_path[j] == '/')
            count_all_seperator++;
    }
    uint64_t count_separators_from_end = 0;
    uint64_t i = str_len;
    for (; i >= 0; i--)
    {
        if (absolute_path[i] == '/')
            count_separators_from_end++;
        if (count_separators_from_end >= count_all_seperator - level)
            break;
    }
    if (i > 0)
    {
        memcpy(out_path, absolute_path, i);
        out_path[i + 1] = '\0';
    }
    else
    {
        strcpy(out_path, "/");
    }
    return;
};
void get_child_path_at_level(uint64_t level, char *absolute_path, char *out_path)
{
    uint64_t str_len = strlen(absolute_path);
    uint64_t count_separators_from_begin = 0;
    uint64_t i = 0;
    for (; i < str_len; i++)
    {
        if (absolute_path[i] == '/')
            count_separators_from_begin++;
        if (count_separators_from_begin == level + 1)
            break;
    }
    memcpy(out_path, absolute_path + i, str_len - i);
    out_path[str_len - i + 1] = '\0';
};
int64_t find_mount_by_path(char *absolute_path)
{
    uint64_t max_level = 0;
    for (uint64_t i = 0; i < strlen(absolute_path); i++)
    {
        if (absolute_path[i] == '/')
            max_level++;
    }
    char tmp_path[MAX_PATH];
    char *debug_tmp_path = tmp_path;
    struct vfs_partition *matched_device = NULL;
    int64_t j = 0;
    for (uint64_t i = max_level; i >= 0; i--)
    {
        memset(tmp_path, 0, sizeof(tmp_path));
        get_parent_path_at_level(i, absolute_path, tmp_path);
        for (j = MAX_MOUNT_POINT - 1; j >= 0; j--)
        {
            if (mount_global[j] != NULL && strcmp((char *)&(mount_global[j]->dest), tmp_path))
            {
                goto end_find;
            }
        }
    }
end_find:
    return j;
}
uint64_t get_mount_deep_level(uint64_t mount_id)
{
    struct mount_point *mp = mount_global[mount_id];
    if (mp == NULL)
        return -1;

    char *dest = mp->dest;
    uint64_t level = 0;
    for (uint64_t i = 0; i < strlen(dest); i++)
    {
        if (dest[i] == '/')
            level++;
    }
    return level - 1;
}
void get_child_path_from_mount_point(uint64_t mount_id, char *absolute_path, char *out_path)
{
    struct mount_point *mp = mount_global[mount_id];
    uint64_t dest_len = strlen(mp->dest);
    if (strcmp(mp->dest, "/") == 0)
    {
        strcpy(out_path, absolute_path);
    }
    else
    {
        strcpy(out_path, absolute_path + dest_len);
    }
}
struct fat32_identification_data
{
    uint8_t ide_channel;
    uint8_t part_id;
    // uint32_t root_cluster;
};
#endif // VFS_H
