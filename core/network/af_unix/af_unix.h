#ifndef AF_UNIX_H
#define AF_UNIX_H

#define MAC_CACHE_TIMEOUT 60000ULL

#define UDS_MAX_PATH 108
#define DGRAM_ADDITIONAL_SOCKET_BLOCK_COUNT 3
#define STREAM_ADDITIONAL_SOCKET_BLOCK_COUNT 1

#define MAX_UDS_SOCKETS_COUNT 5120
#define MAX_UDS_LISTENS_COUNT 5120
enum uds_state_t
{
    UDS_STATE_CLOSED = 0, 
    UDS_STATE_BOUND,
    UDS_STATE_LISTEN, 
    UDS_STATE_BACKLOG_CLIENT,
    UDS_STATE_BACKLOG_SERVER,
    UDS_STATE_ESTABLISHED, 
};
enum uds_socket_type
{
    UDS_NONE,
    UDS_DGRAM,
    UDS_STREAM
};
struct uds_tuple
{
    uint64_t id;
    enum uds_socket_type type;      //;
    char local_path[UDS_MAX_PATH];  
    char remote_path[UDS_MAX_PATH]; 
    enum uds_state_t state;
    uint64_t recv_wait_flag;
    struct timeval last_updated_time_ms;
    uint64_t head;
    uint64_t tail;
};

struct uds_tuple_stream
{
    struct uds_tuple tuple;
    uint8_t retry_count; // tcp state;
    uint64_t seq_local;  // tcp seq;
    uint64_t seq_remote;
    uint64_t listen_wait_flag; // cho listen()
    uint64_t listen_id;
    uint64_t peer_id;
};
struct sockaddr_uds
{
    uint16_t sun_family; // AF_UNIX
    char sun_path[108];
};
extern struct uds_tuple *uds_listen_global[MAX_UDS_LISTENS_COUNT];
extern struct uds_tuple *uds_socket_global[MAX_UDS_SOCKETS_COUNT];
struct uds_tuple_stream **uds_socket_global1 = (struct uds_tuple_stream **)uds_socket_global;
//===================================================
bool uds_tuple_equal(struct uds_tuple *a, struct uds_tuple *b)
{
    if (!strequal(a->local_path, b->local_path))
        return false;
    if (!strequal(a->remote_path, b->remote_path))
        return false;
    if (a->type != b->type)
        return false;
    return true;
}
//=========================================================================
// uint64_t resume_uds_first_thread_pending_on_listen(uint64_t listen_id)
// {
//     thread_info_t **tmp = threads_info_ptr;
//     uint64_t flag_addr = (uint64_t)&((struct uds_tuple *)uds_listen_global)[listen_id].wait_flag;
//     for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
//     {
//         thread_info_t *pointed_thread = tmp[i];
//         if (pointed_thread->state == THREAD_BLOCKED_UDS && pointed_thread->sleep_flag_addr == flag_addr)
//         {
//             tmp[i]->state = THREAD_READY;
//             return i;
//         }
//     }
//     return -1;
// }

bool uds_addr_in_use(char *path)
{
    for (int i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        struct uds_tuple *t = (struct uds_tuple *)uds_socket_global[i];
        if (!t || t->type == UDS_NONE)
            continue;

        if (t->type != UDS_NONE && strequal(path, t->local_path))
        {
            return true; // found conflict
        }
    }
    // TODO: search more files;
    if (path_exists(path))
    {
        return true;
    }
    return false;
}
uint64_t uds_find_ready_connection(uint64_t listen_id)
{
    struct uds_tuple **tmp = uds_socket_global;
    struct uds_tuple *tmp1 = (struct uds_tuple *)uds_listen_global[listen_id];
    for (uint64_t i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        if (tmp[i]->state == NET_STATE_SYN_RECEIVED &&
            strequal(tmp1->local_path, tmp[i]->local_path) &&
            strequal(tmp1->remote_path, tmp[i]->remote_path))
            return i;
    }
    return -1;
}
uint64_t count_sockets_backlog_uds(uint64_t listen_id)
{
    struct uds_tuple **tmp = uds_socket_global;
    struct uds_tuple *tmp1 = (struct uds_tuple *)&uds_listen_global[listen_id];
    uint64_t count = 0;
    for (uint64_t i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        if (tmp[i]->state == NET_STATE_SYN_RECEIVED &&
            strequal(tmp1->local_path, tmp[i]->local_path) &&
            strequal(tmp1->remote_path, tmp[i]->remote_path))
            count++;
    }
    return count;
}
uint64_t create_listen_stream(uint64_t pid, struct uds_tuple_stream *tuple)
{
    uint64_t i = 0;
    for (; i < MAX_UDS_LISTENS_COUNT; i++)
    {
        if (uds_listen_global[i] == 0)
            continue;
        struct uds_tuple *tmp = uds_listen_global[i];
        if (memcmp(tmp, tuple, sizeof(struct uds_tuple)) == 0)
            return -1; // error duplicate listen/soccket;
    }
    for (i = 0; i < MAX_UDS_LISTENS_COUNT; i++)
    {
        if (uds_listen_global[i] != 0)
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(SOCKET_BLOCK_COUNT + STREAM_ADDITIONAL_SOCKET_BLOCK_COUNT) * 4096 + HM;
        uds_listen_global[i] = (struct uds_tuple *)new_free_block_addr;
        uint64_t *new_free_block_ptr = (uint64_t *)new_free_block_addr;
        memcpy((uint8_t *)new_free_block_ptr, (uint8_t *)tuple, sizeof(struct uds_tuple));
        break;
    }
    return i;
}
uint64_t create_socket_dgram(uint64_t pid, struct uds_tuple *tuple)
{
    uint64_t i;
    for (i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        if (uds_socket_global[i] == 0)
            continue;
        struct uds_tuple *tmp = uds_socket_global[i];
        if (memcmp(tmp, tuple, sizeof(struct uds_tuple)) == 0)
            return -1; // error duplicate listen/socket;
    }
    for (i = 0; i < MAX_UDS_LISTENS_COUNT; i++)
    {
        if (uds_socket_global[i] != 0)
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(SOCKET_BLOCK_COUNT + DGRAM_ADDITIONAL_SOCKET_BLOCK_COUNT) * 4096 + HM;
        uds_socket_global[i] = (struct uds_tuple *)new_free_block_addr;
        struct uds_tuple *new_free_block_ptr = (struct uds_tuple *)new_free_block_addr;
        memcpy((uint8_t *)new_free_block_ptr, (uint8_t *)tuple, sizeof(struct uds_tuple));
        new_free_block_ptr->id = i;
        break;
    }
    return i;
}
uint64_t create_socket_stream(uint64_t pid, struct uds_tuple_stream *tuple)
{
    uint64_t i = 0;
    // for (; i < MAX_UDS_SOCKETS_COUNT; i++)
    // {
    //     if (uds_listen_global[i] == 0)
    //         continue;
    //     struct uds_tuple *tmp = uds_socket_global[i];
    //     if (memcmp(tmp, tuple, sizeof(struct uds_tuple_stream)) == 0)
    //         return -1; // error duplicate listen/soccket;
    // }
    for (i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        if (uds_socket_global[i] != 0)
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(SOCKET_BLOCK_COUNT + STREAM_ADDITIONAL_SOCKET_BLOCK_COUNT) * 4096 + HM;
        uds_socket_global[i] = (struct uds_tuple *)new_free_block_addr;
        struct uds_tuple_stream *new_free_block_ptr = (struct uds_tuple_stream *)new_free_block_addr;
        memcpy((uint8_t *)new_free_block_ptr, (uint8_t *)tuple, sizeof(struct uds_tuple_stream));
        new_free_block_ptr->tuple.id = i;
        break;
    }
    return i;
}
uint64_t delete_listen_uds(uint64_t pid, struct uds_tuple *tuple)
{
    uint64_t i = 0;
    for (; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        if (uds_listen_global[i] == 0)
            continue;
        struct uds_tuple *tmp = uds_socket_global[i];
        uint64_t tmp_addr = (uint64_t)tmp;
        if (memcmp(tmp, tuple, sizeof(struct uds_tuple)) == 0)
        {
            erase_block((uint64_t *)tmp_addr);
            erase_block((uint64_t *)(tmp_addr + NET_BLOCK_SIZE));
            uds_listen_global[i] = 0;
            return 0; // found listen/socket;
        }
    }
    return -1;
}
uint64_t delete_socket_uds(uint64_t pid, struct uds_tuple *tuple)
{
    uint64_t i = 0;
    for (; i < NET_BLOCK_SIZE / 8; i++)
    {
        if (uds_socket_global[i] == 0)
            continue;
        struct uds_tuple *tmp = uds_socket_global[i];
        uint64_t tmp_addr = (uint64_t)tmp;
        if (memcmp(tmp, tuple, sizeof(struct uds_tuple)) == 0)
        {
            erase_block((uint64_t *)tmp_addr);
            erase_block((uint64_t *)(tmp_addr) + NET_BLOCK_SIZE);
            uds_socket_global[i] = 0;
            return 0; // found listen/socket;
        }
    }
    return -1;
}
uint64_t delete_listen_i_uds(uint64_t id)
{
    if (uds_listen_global[id] == 0)
        return -1;
    struct uds_tuple *tmp = uds_listen_global[id];
    uint64_t tmp_addr = (uint64_t)tmp;
    erase_block((uint64_t *)tmp_addr);
    erase_block((uint64_t *)(tmp_addr) + NET_BLOCK_SIZE);
    uds_listen_global[id] = 0;
    return 0; // found listen/socket;
}
uint64_t delete_socket_i_uds(uint64_t id)
{
    if (uds_socket_global[id] == 0)
        return -1;
    struct uds_tuple *tmp = uds_socket_global[id];
    uint64_t tmp_addr = (uint64_t)tmp;
    erase_block((uint64_t *)tmp_addr);
    erase_block((uint64_t *)(tmp_addr) + NET_BLOCK_SIZE);
    uds_socket_global[id] = 0;
    return 0; // found listen/socket;
}
//=======================
#include "dgram.h"
#include "stream.h"
//=======================
void handle_uds(uint64_t uds_sock_id, uint8_t *buf, uint64_t buf_len);
bool uds_send_dispatch(uint64_t sender_uds_sock_id, void *data, uint64_t len, char *dst_addr)
{
    if (!dst_addr)
        dst_addr = uds_socket_global[sender_uds_sock_id]->remote_path;
    uint64_t receiver_uds_sock_id = find_socket_uds_with_local_addr(dst_addr);
    if (receiver_uds_sock_id == (uint64_t)-1)
        return false; 
    handle_uds(receiver_uds_sock_id, data, len);
    return true;
};
void handle_uds(uint64_t uds_sock_id, uint8_t *buf, uint64_t buf_len)
{
    struct uds_tuple *tup = uds_socket_global[uds_sock_id];
    if (tup->type == UDS_DGRAM)
    {
        handle_dgram(uds_sock_id, buf, buf_len);
    }
    if (tup->type == UDS_STREAM)
    {
        handle_stream(uds_sock_id, buf, buf_len);
    }
}
//============================================================
#define TCP_MTU 1500
#define STREAM_TIMEWAIT_TIMEOUT_SECONDS 60
void kernel_thread_stream()
{
    while (1)
    {
        kernel_thread_sleep(1000);
        for (uint64_t i = 0; i < 4096 / 8; i++)
        {
            struct uds_tuple *tuple = (struct uds_tuple *)uds_socket_global[i];
            if (tuple->type != UDS_STREAM)
                continue;
            struct uds_tuple_stream *tuple_stream = (struct uds_tuple_stream *)uds_socket_global[i];
            if (tuple_stream->retry_count > 0 && tuple_stream->retry_count < 04)
            {
                uint32_t len = *(uint64_t *)((uint64_t)tuple_stream + 4096 * 4 - 8);
                if (len > TCP_MTU)
                    len = TCP_MTU;
                uds_send_dispatch(i, (uint8_t *)tuple_stream + sizeof(struct uds_tuple_stream), len, 0);
                tuple_stream->retry_count++;
            }
            
            if (tuple_stream->tuple.state == NET_STATE_TIME_WAIT)
            {
                tuple_stream->retry_count++;
                if (_later(subtract_time_us(get_time_us(), STREAM_TIMEWAIT_TIMEOUT_SECONDS), tuple_stream->tuple.last_updated_time_ms))
                {
                    tuple_stream->tuple.state = NET_STATE_CLOSED;
                    delete_socket_i_uds(i); 
                }
            }

            
            // if (tuple_tcp->state == NET_STATE_CLOSED && tuple_tcp->ref_count == 0)
            // {
            //     release_socket(i);
            // };
        };
    };
}
#endif // AF_UNIX_H
