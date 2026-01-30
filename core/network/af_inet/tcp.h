#ifndef NETWORKS_TCP_H
#define NETWORKS_TCP_H

//=============================================================
// bool send_tcp_packet(uint64_t socket_id, uint8_t *payload, uint64_t payload_index, uint64_t payload_len)
// {
//     // send buffer in lower 4*4kb
//     struct net_tuple_tcp *tuple = (struct net_tuple_tcp *)&socket_global[socket_id];
//     if (payload_index + payload_len > tuple->seq_remote + 4 * 4096 - sizeof(struct net_tuple_tcp) - 8)
//         return false;
//     if (payload_index < tuple->seq_local)
//         return false;
//     memcpy((uint8_t *)((uint64_t)tuple) + payload_index - tuple->seq_local + sizeof(struct net_tuple_tcp), payload, payload_len);
//     tuple->tuple.last_updated_time_ms = get_time_ms();
// }
uint16_t tcp_checksum(struct tcp_hdr *tcp, size_t tcp_len,
                      uint32_t src_ip, uint32_t dst_ip,
                      const uint8_t *payload, size_t payload_len)
{
    uint32_t sum = 0, s = 0;
    uint16_t total_len = tcp_len + payload_len;

    // --- 1. pseudo header ---
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += (TCP_PROTOCOL_NUMBER);
    sum += (total_len);

    
    const uint8_t *hdr_bytes = (const uint8_t *)tcp;
    size_t hdr_len = tcp_len;
    while (hdr_len > 1)
    {
        sum += (hdr_bytes[0] << 8) | hdr_bytes[1];
        hdr_bytes += 2;
        hdr_len -= 2;
    }
    if (hdr_len == 1)
        sum += (hdr_bytes[0] << 8);

    // --- 3. Payload ---
    const uint8_t *p = payload;
    size_t len = payload_len;
    while (len > 1)
    {
        sum += (p[0] << 8) | p[1];
        p += 2;
        len -= 2;
    }
    if (len == 1)
        sum += (p[0] << 8);

    
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    sum = htons(~sum);
    return sum;
}
bool nic_send_tcp_package(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[socket_id];

    struct tcp_hdr tcp;
    memset(&tcp, 0, sizeof(tcp));

    tcp.src_port = htons(sock->tuple.local_port);
    tcp.dst_port = htons(sock->tuple.remote_port);

    tcp.seq = htonl(sock->seq_local);
    tcp.ack_seq = htonl(sock->seq_remote); 
    tcp.data_offset_reserved = (5 << 4);
    tcp.flags = (payload_len == 0 ? 0x10 : 0x18); 
    tcp.window = htons(65535);

    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(&tcp, sizeof(tcp), sock->tuple.local_ip_l, sock->tuple.remote_ip_l, payload, payload_len);

    
    struct net_tuple_tcp *tuple = (struct net_tuple_tcp *)socket_global[socket_id];
    tuple->tuple.last_updated_time_us = get_time_us();
    return nic_send_ipv4(tuple->tuple.local_ip_l, tuple->tuple.remote_ip_l, TCP, &tcp, sizeof(tcp),
                         payload, payload_len);
}
bool tcp_send_established(uint64_t sock_id)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[sock_id];
    uint8_t *payload = (uint8_t *)(((uint64_t)sock) + 4096 * SOCKET_BLOCK_COUNT);
    nic_send_tcp_package(sock_id, payload, sock->send_payload_len);
}
uint64_t tcp_copy_payload_and_send_established(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[socket_id];
    if (sock->ack_est_blocked)
    {
        sleep_by_resource((uint64_t)&sock->send_wait_flag, THREAD_BLOCKED_NETWORK);
        // scheduler_yeild();
    }
    if (sock->tuple.state != NET_STATE_ESTABLISHED)
        return -ENOTSOCK;
    uint8_t *payload_in_sock = (uint8_t *)(((uint64_t)sock) + 4096 * SOCKET_BLOCK_COUNT);
    uint64_t len = payload_len > TCP_MTU ? TCP_MTU : payload_len;
    sock->send_retry_count = 1;
    sock->ack_est_blocked = true;
    sock->send_payload_len = len;
    memcpy(payload_in_sock, payload, len);
    tcp_send_established(socket_id);
    return (len);
}
bool tcp_send_ack(struct net_tuple_tcp *sock, uint64_t payload_index, uint64_t payload_len)
{
    // struct net_tuple_tcp *sock = sockets_global[socket_id];

    
    uint8_t tmp[1];
    uint8_t *payload = (uint8_t *)&tmp;
    if (payload_len > 0)
    {
        payload = (uint8_t *)(((uint64_t)sock) + sizeof(struct net_tuple_tcp) + payload_index);
    }

    struct tcp_hdr tcp;
    memset(&tcp, 0, sizeof(tcp));

    tcp.src_port = htons(sock->tuple.local_port);
    tcp.dst_port = htons(sock->tuple.remote_port);
    tcp.seq = htonl(sock->seq_local);
    tcp.ack_seq = htonl(sock->seq_remote);
    tcp.data_offset_reserved = (5 << 4);
    tcp.flags = TCP_ACK;
    tcp.window = htons(65535);

    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(
        &tcp, sizeof(tcp),
        sock->tuple.local_ip_l, sock->tuple.remote_ip_l,
        payload, payload_len);

    nic_send_ipv4(
        sock->tuple.local_ip_l,
        sock->tuple.remote_ip_l,
        6, &tcp, sizeof(tcp),
        payload, payload_len);

    sock->seq_local += payload_len;
    sock->tuple.last_updated_time_us = get_time_us();
    return true;
}
bool tcp_send_synack(struct net_tuple_tcp *sock, uint32_t remote_ack_le, uint64_t payload_len)
{
    // struct net_tuple_tcp *sock = sockets_global[socket_id];

    uint8_t payload[1];
    // if (payload_len > 0)
    // {
    //     payload = (uint8_t *)sock + sizeof(struct net_tuple_tcp) + (payload_index - sock->seq_local);
    // }

    struct tcp_hdr tcp;
    memset(&tcp, 0, sizeof(tcp));

    remote_ack_le++;
    tcp.src_port = htons(sock->tuple.local_port);
    tcp.dst_port = htons(sock->tuple.remote_port);
    tcp.seq = htonl(sock->seq_local);
    tcp.ack_seq = htonl(remote_ack_le);
    tcp.data_offset_reserved = (5 << 4);
    tcp.flags = TCP_SYN | TCP_ACK; 
    tcp.window = htons(65535);

    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(
        &tcp, sizeof(tcp),
        sock->tuple.local_ip_l, sock->tuple.remote_ip_l,
        payload, payload_len);

    nic_send_ipv4(
        sock->tuple.local_ip_l,
        sock->tuple.remote_ip_l,
        6, &tcp, sizeof(tcp),
        payload, payload_len);

    sock->seq_remote = remote_ack_le;   
    sock->seq_local += payload_len + 1; 
    return true;
}

int64_t tcp_send_syn(uint64_t socket_id)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[socket_id];
    if (sock->tuple.version == 0)
        return -EINVAL;

    
    if (sock->seq_local == 0)
    {
        
        static uint32_t next_isn = 0x1000;
        next_isn += 1; 
        sock->seq_local = next_isn;
    }

    
    uint8_t payload[0];
    uint64_t payload_len = 0;
    uint64_t payload_index = sock->seq_local; // SYN uses this seq

    // 3) build TCP header
    struct tcp_hdr tcp;
    memset(&tcp, 0, sizeof(tcp));

    tcp.src_port = htons(sock->tuple.local_port);
    tcp.dst_port = htons(sock->tuple.remote_port);

    if (!sock->tuple.local_ip_l)
        sock->tuple.local_ip_l = nic_global[1]->ip[0].addr_l;
    if (!sock->tuple.local_port)
        sock->tuple.local_port = allocate_random_high_port(UDP);

    tcp.seq = htonl(sock->seq_local);
    tcp.ack_seq = htonl(0);              // SYN has no ack
    tcp.data_offset_reserved = (5 << 4); // header length = 5 * 4 = 20 bytes

    tcp.flags = TCP_SYN; // only SYN
    tcp.window = htons(65535);

    // 4) checksum
    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(&tcp, sizeof(tcp),
                                (sock->tuple.local_ip_l), (sock->tuple.remote_ip_l),
                                payload, payload_len);

    // 5) send via nic_send_ipv4 (like your other functions)
    int64_t sent = nic_send_ipv4(sock->tuple.local_ip_l, sock->tuple.remote_ip_l, TCP, &tcp, sizeof(tcp), (uint8_t *)&payload, payload_len);

    if (sent < 0)
        return sent;

    // 6) update seq/state (SYN consumes 1 sequence number)
    sock->seq_local += 1;
    sock->tuple.state = NET_STATE_SYN_SENT;
    // optionally set retry_count, timeouts, etc.
    // sock->send_retry_count = 0;

    sock->tuple.last_updated_time_us = get_time_us();
    return 0;
}
bool tcp_send_fin(uint64_t socket_id)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[socket_id];
    if (!sock || sock->tuple.version == 0)
        return false;

    
    if (sock->tuple.state != NET_STATE_ESTABLISHED &&
        sock->tuple.state != NET_STATE_CLOSE_WAIT)
        return false;

    
    struct tcp_hdr tcp;
    memset(&tcp, 0, sizeof(tcp));

    tcp.src_port = htons(sock->tuple.local_port);
    tcp.dst_port = htons(sock->tuple.remote_port);
    tcp.seq = htonl(sock->seq_local);
    tcp.ack_seq = htonl(sock->seq_remote);
    tcp.data_offset_reserved = (5 << 4); // header length = 20 bytes
    tcp.flags = TCP_FIN | TCP_ACK;       
    tcp.window = htons(65535);

    
    uint8_t *payload = NULL;
    uint64_t payload_len = 0;

    
    tcp.checksum = 0;
    tcp.checksum = tcp_checksum(&tcp, sizeof(tcp),
                                sock->tuple.local_ip_l, sock->tuple.remote_ip_l,
                                payload, payload_len);

    
    bool sent = nic_send_ipv4(sock->tuple.local_ip_l, sock->tuple.remote_ip_l,
                              TCP, &tcp, sizeof(tcp), payload, payload_len);
    if (!sent)
        return false;

    
    sock->seq_local += 1;

    if (sock->tuple.state == NET_STATE_ESTABLISHED)
        sock->tuple.state = NET_STATE_FIN_WAIT_1;
    else if (sock->tuple.state == NET_STATE_CLOSE_WAIT)
        sock->tuple.state = NET_STATE_LAST_ACK;

    
    sock->send_retry_count = 0;

    sock->tuple.last_updated_time_us = get_time_us();
    return true;
}
bool read_socket_tcp(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple_tcp *lis = (struct net_tuple_tcp *)socket_global[socket_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct net_tuple_tcp));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    uint64_t rs = fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, payload_len, payload);
    if (rs != 0)
    {
        return true;
    }
    sleep_by_resource((uint64_t)&lis->tuple.recv_wait_flag, THREAD_BLOCKED_NETWORK);
    rs = fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, payload_len, payload);
    return rs != 0;
}
bool recv_socket_append_tcp(uint64_t socket_id, uint8_t *payload, uint64_t payload_index, uint64_t payload_len)
{
    struct net_tuple_tcp *lis = (struct net_tuple_tcp *)socket_global[socket_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct net_tuple_tcp));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    return fifo_write8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, payload_len, payload) != 0;
}
bool handle_tcp_established_packet(uint32_t src_ip, uint32_t dst_ip, uint32_t src_port, uint32_t dst_port, uint8_t *payload, uint64_t payload_index, uint64_t payload_len)
{
    struct net_tuple tmp;
    tmp.local_ip_h = 0;
    tmp.local_ip_l = dst_ip;
    tmp.remote_ip_h = 0;
    tmp.remote_ip_l = src_ip;
    tmp.remote_port = src_port;
    tmp.local_port = dst_port;
    tmp.protocol = TCP;
    tmp.version = 4;
    uint64_t socket_id = find_socket_not_listen(&tmp);
    if (socket_id != -1)
    {
        struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[socket_id];
        sock->seq_remote += payload_len;
        recv_socket_append_tcp(socket_id, payload, payload_index, payload_len);
        tcp_send_ack(sock, payload_index, 0);
        // resume
        for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
        {
            if (threads_info_ptr[i] && threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t)&sock->tuple.recv_wait_flag)
                threads_info_ptr[i]->state = THREAD_READY;
        }
        return true;
    }
    // uint64_t listen_id = find_listen(&tmp);
    // if (listen_id != -1)
    // {
    //     udp_listen_append(socket_id, payload, payload_len);
    //     return true;
    // }
    return false;
}
bool handle_tcp_receive_ack_established(uint64_t sock_id, uint64_t ack_seq)
{
    struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[sock_id];
    if (!sock || sock->tuple.state != NET_STATE_ESTABLISHED || !sock->ack_est_blocked)
        return false;
    if (sock->seq_local + sock->send_payload_len != ack_seq)
        return false;
    sock->seq_local += sock->send_payload_len;
    sock->ack_est_blocked = false;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i] && threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t)&sock->send_wait_flag)
        {
            threads_info_ptr[i]->state = THREAD_READY;
        }
    }
    return true;
}
uint64_t push_backlog(uint64_t child_sockid, uint64_t listener_id)
{
    struct net_tuple_tcp *lis = (struct net_tuple_tcp *)socket_global[listener_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct net_tuple_tcp));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT + TCP_ADDITIONAL_SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    return fifo_write8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, 8, (uint8_t *)&child_sockid);
}
uint64_t pull_backlog(uint64_t listener_id)
{
    struct net_tuple_tcp *lis = (struct net_tuple_tcp *)socket_global[listener_id];
    uint8_t *start_buffer = (uint8_t *)(((uint64_t)lis) + sizeof(struct net_tuple_tcp));
    uint8_t *stop_buffer = (uint8_t *)(((uint64_t)lis) + 4096 * (SOCKET_BLOCK_COUNT + TCP_ADDITIONAL_SOCKET_BLOCK_COUNT));
    uint64_t buffer_size = stop_buffer - start_buffer;
    uint64_t child_sockid = -1;
    if (fifo_read8(start_buffer, &lis->tuple.tail, &lis->tuple.head, buffer_size, 8, (uint8_t *)&child_sockid))
        return child_sockid;
    else
        return -1;
}
void handle_tcp_v4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    struct ipv4_hdr *ip = (struct ipv4_hdr *)(buf + sizeof(struct eth_hdr));
    uint64_t ip_header_len = (ip->version_ihl & 0x0F) * 4;
    struct tcp_hdr *header = (struct tcp_hdr *)(buf + sizeof(struct eth_hdr) + sizeof(struct ipv4_hdr));
    // uint64_t ip_header_len = ;
    // if (buf_len < ip_header_len + tcp_header_len)
    

    struct tcp_hdr *tcp = header;

    uint64_t tcp_header_len = (tcp->data_offset_reserved >> 4) * 4;
    uint32_t src_ip = ntohl(ip->src);
    uint32_t dst_ip = ntohl(ip->dst);
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dst_port = ntohs(tcp->dst_port);
    uint16_t tcp_len = buf_len - sizeof(struct eth_hdr) - ip_header_len - tcp_header_len;
    uint32_t seq = ntohl(header->seq);
    uint64_t payload_index = seq;

    uint8_t flags = tcp->flags;
    uint32_t ack_seq = ntohl(tcp->ack_seq);
    uint8_t *payload = buf + sizeof(struct eth_hdr) + ip_header_len + tcp_header_len;
    uint64_t payload_len = tcp_len;

    struct net_tuple_tcp tmp;
    tmp.tuple.local_ip_h = 0;
    tmp.tuple.local_ip_l = (dst_ip);
    tmp.tuple.local_port = (dst_port);
    tmp.tuple.protocol = TCP;
    tmp.tuple.remote_ip_h = 0;
    tmp.tuple.remote_ip_l = (src_ip);
    tmp.tuple.remote_port = (src_port);
    tmp.tuple.version = 4;
    uint64_t sock_id = find_socket_not_listen((struct net_tuple *)&tmp);
    if (sock_id == -1)
        sock_id = find_socket((struct net_tuple *)&tmp);
    if (sock_id == -1)
        return;
    struct net_tuple_tcp *socket = (struct net_tuple_tcp *)socket_global[sock_id];
    switch (socket->tuple.state)
    {
    case NET_STATE_LISTEN:
        if (flags & TCP_SYN)
        {
            uint64_t listen_id = find_listen((struct net_tuple *)&tmp);
            if (listen_id == -1)
                break; // ignore because noone listening;
            // check if socket exists
            uint64_t find_sock = find_socket_not_listen((struct net_tuple *)&tmp);
            if (find_sock != -1)
            {
                tcp_send_synack((struct net_tuple_tcp *)socket_global[find_sock], seq, 0);
                break;
            }
            // create new socket to handle this listening incomming connection;
            uint64_t new_socket_id = create_socket_tcp(get_current_pid(), &tmp);
            struct net_tuple_tcp *new_socket = (struct net_tuple_tcp *)socket_global[new_socket_id];
            new_socket->listen_id = listen_id;
            new_socket->tuple.state = NET_STATE_SYN_RECEIVED;
            tcp_send_synack(new_socket, seq, 0);
            new_socket->tuple.last_updated_time_us = get_time_us();
        }
        break;

    case NET_STATE_SYN_SENT:
        if ((flags & TCP_SYN) && (flags & TCP_ACK))
        {
            socket->seq_remote = seq + 1;
            tcp_send_ack(socket, seq, 0);
            socket->tuple.state = NET_STATE_ESTABLISHED;
        }
        break;

    case NET_STATE_SYN_RECEIVED:
        if (flags & TCP_ACK)
        {
            if (!push_backlog(sock_id, socket->listen_id))
                mark_socket_as_deleted(sock_id);
            socket->tuple.state = NET_STATE_ESTABLISHED;
            resume_first_thread_pending_on_listen(socket->listen_id);
        }
        break;

    case NET_STATE_ESTABLISHED:
        if (flags & TCP_FIN)
        {
            socket->seq_remote++;
            tcp_send_ack(socket, seq, 0);
            socket->tuple.state = NET_STATE_CLOSE_WAIT;
        }
        if (flags & TCP_ACK)
        {
            handle_tcp_receive_ack_established(sock_id, ack_seq);
        }
        if (payload_len != 0)
        {
            if (socket->seq_remote && seq != socket->seq_remote)
                return;
            handle_tcp_established_packet(src_ip, dst_ip, src_port, dst_port, payload, seq, payload_len);
        }
        break;

    case NET_STATE_FIN_WAIT_1:
        if (flags & TCP_ACK)
            socket->tuple.state = NET_STATE_FIN_WAIT_2;
        else if (flags & TCP_FIN)
            socket->tuple.state = NET_STATE_CLOSING;
        break;

    case NET_STATE_FIN_WAIT_2:
        if (flags & TCP_FIN)
            socket->tuple.state = NET_STATE_TIME_WAIT;
        socket->send_retry_count = 0;
        break;

    case NET_STATE_CLOSE_WAIT:
        
        // socket->tuple.state = NET_STATE_LAST_ACK;
        break;

    case NET_STATE_LAST_ACK:
        if (flags & TCP_ACK)
            socket->tuple.state = NET_STATE_CLOSED;
        break;

    case NET_STATE_CLOSING:
        if (flags & TCP_ACK)
            socket->tuple.state = NET_STATE_TIME_WAIT;
        socket->send_retry_count = 0;
        break;

    case NET_STATE_TIME_WAIT:
        
        break;
    }
    socket->tuple.last_updated_time_us = get_time_us();
}
#endif // NETWORKS_TCP_H
