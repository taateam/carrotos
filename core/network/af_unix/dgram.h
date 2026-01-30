#ifndef NETWORKS_DGRAM_H
#define NETWORKS_DGRAM_H

#define DGRAM_MTU 1024
#define MAX_UDS_BACKLOG 1024
// =====================================================================
uint64_t find_socket_uds(struct uds_tuple *tuple)
{
    for (uint64_t i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        struct uds_tuple *tmp = uds_socket_global[i];
        if (tmp && uds_tuple_equal(tmp, tuple))
            return i;
    }
    return -1; // error duplicate listen/soccket;
}
uint64_t find_socket_uds_with_local_addr(char *path)
{
    for (uint64_t i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        struct uds_tuple *tmp = uds_socket_global[i];
        if (strcmp(path, tmp->local_path))
            return i;
    }
    return -1; // error duplicate listen/soccket;
}
uint64_t find_listen_uds(struct uds_tuple *tuple)
{
    for (uint64_t i = 0; i < MAX_UDS_LISTENS_COUNT; i++)
    {
        struct uds_tuple *tmp = uds_listen_global[i];
        if (uds_tuple_equal(tmp, tuple))
            return i;
    }
    return -1; // error duplicate listen/soccket;
}
bool handle_dgram_bind(uint64_t socket_id, char *path)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_DGRAM)
        return false;
    strcpy(sock->tuple.local_path, path);
    if (sock->tuple.state == UDS_STATE_CLOSED)
        sock->tuple.state = UDS_STATE_BOUND;
    return true;
}
bool handle_connect_dgram(uint64_t socket_id, struct sockaddr_uds *addr, uint64_t addrlen)
{
    struct uds_tuple *sock = (struct uds_tuple *)uds_socket_global[socket_id];
    if (sock->type != UDS_DGRAM)
        return false;
    // if (sock->state != UDS_STATE_CLOSED &&
    //     sock->state != UDS_STATE_BOUND)
    //     return false;
    bool found = false;
    uint64_t i = 0;
    struct uds_tuple *tmp = NULL;
    for (; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        tmp = uds_socket_global[i];
        if (tmp->type == UDS_DGRAM && strequal(tmp->local_path, addr->sun_path) && tmp->state != UDS_STATE_CLOSED)
        {
            found = true;
            break;
        };
    }
    if (found)
    {
        strcpy(sock->remote_path, addr->sun_path);
        tmp->state = UDS_STATE_ESTABLISHED;
        sock->state = UDS_STATE_ESTABLISHED;
        return true;
    }
    else
    {
        return false;
    }
}
int64_t read_socket_dgram_non_blocking(struct uds_tuple *t, uint8_t *payload, uint64_t payload_len)
{
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple);
    memset(payload, 0, payload_len);
    buf_len = div_ceil(buf_len, DGRAM_MTU) * DGRAM_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + DGRAM_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    uint64_t len = len_records[t->head / DGRAM_MTU];
    uint8_t tmp_buf[DGRAM_MTU];
    memset(tmp_buf, 0, DGRAM_MTU);
    uint64_t rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, DGRAM_MTU, tmp_buf);
    memcpy(payload, tmp_buf, payload_len);
    if (rs && len)
        return (int64_t)len > payload_len ? payload_len : len;
}
int64_t read_socket_dgram_blocking(struct uds_tuple *t, uint8_t *payload, uint64_t payload_len)
{
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple);
    memset(payload, 0, payload_len);
    buf_len = div_ceil(buf_len, DGRAM_MTU) * DGRAM_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + DGRAM_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    uint64_t len = len_records[t->head / DGRAM_MTU];
    uint8_t tmp_buf[DGRAM_MTU];
    memset(tmp_buf, 0, DGRAM_MTU);
    uint64_t rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, DGRAM_MTU, tmp_buf);
    memcpy(payload, tmp_buf, payload_len);
    if (rs)
        return (int64_t)len > payload_len ? payload_len : len;
    //===================================
    uint64_t tid = get_current_thread_id();
    thread_info_t *thread = threads_info_ptr[tid];
    sleep_by_resource((uint64_t)&t->recv_wait_flag, THREAD_BLOCKED_NETWORK);
    // scheduler_yeild();
    //===================================
    if (t->state == NET_STATE_CLOSE_WAIT)
        return -EBADF;
    rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, DGRAM_MTU, tmp_buf);
    memcpy(payload, tmp_buf, payload_len);
    return (int64_t)len > payload_len ? payload_len : len;
}
int64_t read_socket_dgram(struct uds_tuple *t, uint8_t *payload, uint64_t payload_len, bool flags_non_blocking)
{
    if (payload_len > DGRAM_MTU)
        return -EINVAL;
    if (!flags_non_blocking)
        return read_socket_dgram_blocking(t, payload, payload_len);
    else
        return read_socket_dgram_non_blocking(t, payload, payload_len);
}
int64_t read_socket_by_id_uds(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    // uint64_t socket_id = find_socket_uds(tuple);
    struct uds_tuple *t = uds_socket_global[socket_id];
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple);
    uint64_t rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, payload_len, payload);
    return (int64_t)rs;
}
int64_t socket_append_uds(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple *t = uds_socket_global[socket_id];
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple);
    uint64_t rs = fifo_write8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, payload_len, payload);
    return (int64_t)rs;
}
bool listen_append_uds(uint64_t listen_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple *t = uds_listen_global[listen_id];
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple);
    uint64_t rs = fifo_write8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, payload_len, payload);
    return (int64_t)rs;
}
bool handle_dgram_packet(char *path, uint8_t *payload, uint64_t payload_len)
{
    if (!payload_len)
        return false;
    uint64_t socket_id = find_socket_uds_with_local_addr(path);
    if (socket_id != -1)
    {
        struct uds_tuple *sock = uds_socket_global[socket_id];
        socket_append_uds(socket_id, payload, payload_len);
        for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
        {
            if (threads_info_ptr[i] && threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t)&sock->recv_wait_flag)
                threads_info_ptr[i]->state = THREAD_READY;
        }
        return true;
    }
    // uint64_t listen_id = find_listen_uds(&tmp);
    // if (listen_id != -1)
    // {
    //     listen_append_uds(socket_id, payload, payload_len);
    //     return true;
    // }
    return false;
}
void handle_dgram(uint64_t socket_id, uint8_t *buf, uint64_t buf_len)
{
    char *path = uds_socket_global[socket_id]->local_path; 
    handle_dgram_packet(path, buf, buf_len);
    return;
}
bool uds_send_dispatch(uint64_t sender_uds_sock_id, void *data, uint64_t len, char *dst_addr);
bool nic_send_dgram_package(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple *sock = uds_socket_global[socket_id];
    uds_send_dispatch(socket_id, payload, payload_len, 0);
    return true;
}
#endif // NETWORKS_DGRAM_H
