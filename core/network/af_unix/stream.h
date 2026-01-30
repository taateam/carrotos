#ifndef NETWORK_STREAM_H
#define NETWORK_STREAM_H

// #define MAX_UDS_BACKLOG 1024
//=============================================================
uint64_t push_backlog_stream(uint64_t child_sockid, uint64_t listener_id)
{
    struct uds_tuple_stream *lis = (struct uds_tuple_stream *)uds_socket_global[listener_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct uds_tuple_stream));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT + TCP_ADDITIONAL_SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    return fifo_write8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, 8, (uint8_t *)&child_sockid);
}
uint64_t pull_backlog_stream(uint64_t listener_id)
{
    struct uds_tuple_stream *lis = (struct uds_tuple_stream *)uds_socket_global[listener_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct uds_tuple_stream));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT + TCP_ADDITIONAL_SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    uint64_t child_sockid = -1;
    if (fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, 8, (uint8_t *)&child_sockid))
        return child_sockid;
    else
        return -1;
}
bool handle_stream_bind(uint64_t socket_id, char *path)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_STREAM)
        return false;
    strcpy(sock->tuple.local_path, path);
    sock->tuple.state = UDS_STATE_BOUND;
    return true;
}
bool handle_stream_listen(uint64_t socket_id)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_STREAM)
        return false;
    if (sock->tuple.state != UDS_STATE_BOUND)
        return false;
    for (uint64_t i = 0; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        struct uds_tuple *tmp = uds_socket_global[i];
        if (tmp && tmp->type == UDS_STREAM && strequal(tmp->local_path, sock->tuple.local_path) && tmp->state == UDS_STATE_LISTEN)
        {
            return false;
        };
    }
    sock->tuple.state = UDS_STATE_LISTEN;
    sock->tuple.head = 0;
    sock->tuple.tail = 0;
    strcpy(sock->tuple.remote_path, "");
    return create_listen_stream(get_current_pid(), sock);
    // thread_info_t *curr = threads_info_ptr[get_current_thread_id()];
    // curr->state = THREAD_BLOCKED_UDS;
    // curr->sleep_flag_addr = sock->tuple.recv_wait_flag;
    // scheduler_yeild(); // no return because yeild;
}
bool resume_first_thread_pending_on_listen_stream(uint64_t listener_id)
{
    thread_info_t **tmp = threads_info_ptr;
    uint64_t flag_addr = (uint64_t)&((struct uds_tuple_stream **)uds_socket_global)[listener_id]->listen_wait_flag;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *pointed_thread = tmp[i];
        if (pointed_thread && pointed_thread->state == THREAD_BLOCKED_UDS && pointed_thread->sleep_flag_addr == flag_addr)
        {
            tmp[i]->state = THREAD_READY;
            return i;
        }
    }
    return -1;
}
bool handle_connect_stream(uint64_t client_sock_id, struct sockaddr_uds *addr, uint64_t addrlen)
{
    struct uds_tuple_stream *client_sock = (struct uds_tuple_stream *)uds_socket_global[client_sock_id];
    if (client_sock->tuple.type != UDS_STREAM)
        return false;
    if (client_sock->tuple.state != UDS_STATE_CLOSED &&
        client_sock->tuple.state != UDS_STATE_BOUND)
        return false;
    bool found = false;
    uint64_t i = 0;
    for (; i < MAX_UDS_SOCKETS_COUNT; i++)
    {
        struct uds_tuple *tmp = uds_socket_global[i];
        if (tmp && tmp->type == UDS_STREAM && strequal(tmp->local_path, addr->sun_path) && tmp->state == UDS_STATE_LISTEN)
        {
            found = true;
            break;
        };
    }
    if (found)
    {
        uint64_t listening_sock_id = i;
        struct uds_tuple_stream *listening_sock = (struct uds_tuple_stream *)uds_socket_global[listening_sock_id];
        client_sock->tuple.state = UDS_STATE_BACKLOG_CLIENT;
        // create new sock;
        struct uds_tuple_stream tmp = {0};
        // strcpy(tmp.tuple.remote_path, listening_sock->tuple.local_path);
        uint64_t pid = get_current_pid();
        uint64_t server_sock_id = create_socket_stream(pid, &tmp);
        struct uds_tuple_stream *server_sock = (struct uds_tuple_stream *)uds_socket_global[server_sock_id];
        server_sock->tuple.state = UDS_STATE_BACKLOG_SERVER;
        server_sock->tuple.type = UDS_STREAM; // link
        server_sock->peer_id = client_sock_id;
        client_sock->peer_id = server_sock_id;
        push_backlog_stream(server_sock_id, listening_sock_id);
        // wake
        resume_first_thread_pending_on_listen_stream(listening_sock_id);
        return true;
    }
    else
    {
        return false;
    }
}
int64_t handle_stream_accept(uint64_t listener_id)
{
    struct uds_tuple_stream *listener_sock = (struct uds_tuple_stream *)uds_socket_global[listener_id];
    if (listener_sock->tuple.type != UDS_STREAM)
        return -EOPNOTSUPP;
    uint64_t server_sock_id = pull_backlog_stream(listener_id);
    if (server_sock_id == -1)
    {
        sleep_by_resource((uint64_t)&listener_sock->listen_wait_flag, THREAD_BLOCKED_UDS);
    }
    server_sock_id = pull_backlog_stream(listener_id);
    if (server_sock_id == -1)
    {
        return -1;
    }
    // uint64_t server_sock_id = client_socket_id;
    struct uds_tuple_stream *server_sock = (struct uds_tuple_stream *)uds_socket_global[server_sock_id];
    uint64_t client_sock_id = server_sock->peer_id;
    struct uds_tuple_stream *client_sock = (struct uds_tuple_stream *)uds_socket_global[client_sock_id];
    server_sock->tuple.state = UDS_STATE_ESTABLISHED;
    client_sock->tuple.state = UDS_STATE_ESTABLISHED;
    // fd;
    uint64_t pid = get_current_pid();
    struct file_inner *fd = (struct file_inner *)processes_info_ptr[pid]->fd_addr;
    uint64_t new_socket_fd_id = get_a_free_slot_with_size((uint64_t)fd, 0, MAX_FD_INNER, sizeof(struct file_inner));
    struct file_inner *newfd = &fd[new_socket_fd_id];
    newfd->type = FILE_SOCKET_UDS;
    newfd->open_flags = O_RDWR; 
    newfd->res_id = server_sock_id;
    return new_socket_fd_id;
}
bool send_stream_established_packet(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_STREAM)
        return false;
    if (sock->tuple.state != UDS_STATE_ESTABLISHED)
        return false;
    uds_send_dispatch(socket_id, payload, payload_len, 0);
    return true;
}
int64_t socket_append_uds_stream(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple_stream *t = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct uds_tuple_stream);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct uds_tuple_stream);
    uint64_t rs = fifo_write8((uint8_t *)buf_start_addr, &t->tuple.tail, &t->tuple.head, buf_len, payload_len, payload);
    return (int64_t)rs;
}
bool handle_stream_established_packet(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_STREAM)
        return false;
    if (sock->tuple.state != UDS_STATE_ESTABLISHED)
        return false;
    socket_append_uds_stream(socket_id, payload, payload_len);
    resume_first_thread_pending_on_listen_stream(socket_id);
    return false;
}
bool handle_stream_close(uint64_t socket_id)
{
    struct uds_tuple_stream *sock = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (sock->tuple.type != UDS_STREAM)
        return false;
    if (sock->tuple.state == UDS_STATE_CLOSED)
        return false;
    sock->tuple.state = UDS_STATE_CLOSED;
    resume_first_thread_pending_on_listen_stream(socket_id);
    uint64_t pid = get_current_pid();
    delete_listen_i_uds(sock->listen_id);
    delete_socket_i_uds(socket_id);
    return true;
}
bool read_socket_stream(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct uds_tuple_stream *lis = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct uds_tuple_stream));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    uint64_t rs = fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, payload_len, payload);
    if (rs != 0)
    {
        return true;
    }
    sleep_by_resource((uint64_t)&lis->tuple.recv_wait_flag, THREAD_BLOCKED_UDS);
    rs = fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, payload_len, payload);
    return rs != 0;
}
uint64_t handle_stream(uint64_t socket_id, uint8_t *buf, uint64_t buf_len)
{
    struct uds_tuple_stream *socket = (struct uds_tuple_stream *)uds_socket_global[socket_id];
    if (socket->tuple.type != UDS_STREAM)
        return -ECONNRESET;
    switch (socket->tuple.state)
    {
    case UDS_STATE_ESTABLISHED:
        handle_stream_established_packet(socket_id, buf, buf_len);
        break;

    default:
        return -ECONNRESET;
    }
    socket->tuple.last_updated_time_ms = get_time_us();
    return 0;
}
#endif // NETWORK_STREAM_H
