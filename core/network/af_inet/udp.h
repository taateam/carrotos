#ifndef NETWORKS_UDP_H
#define NETWORKS_UDP_H

// =====================================================================
uint64_t find_socket(struct net_tuple *tuple)
{
    for (uint64_t i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        struct net_tuple *tmp = socket_global[i];
        if (net_tuple_match_v4(tuple, tmp))
            return i;
    }
    return -1; // error not found listen/soccket;
}
uint64_t find_socket_not_listen(struct net_tuple *tuple)
{
    for (uint64_t i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        struct net_tuple *tmp = socket_global[i];
        if (net_tuple_equal_v4(tuple, tmp))
            return i;
    }
    return -1; // error not found listen/soccket;
}
uint64_t find_listen(struct net_tuple *tuple)
{
    for (uint64_t i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        struct net_tuple *tmp = listen_global[i];
        if (!tmp)
            continue;
        if (tmp->state == NET_STATE_LISTEN && net_tuple_match_v4(tuple, tmp))
            return i;
    }
    return -1; // error not found listen/soccket;
}
int64_t read_socket_udp_non_blocking(struct net_tuple *t, uint8_t *payload, uint64_t payload_len)
{
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct net_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct net_tuple);
    memset(payload, 0, payload_len);
    buf_len = div_ceil(buf_len, UDP_MTU) * UDP_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    uint64_t len = len_records[t->head / UDP_MTU];
    uint8_t tmp_buf[UDP_MTU];
    memset(tmp_buf, 0, UDP_MTU);
    uint64_t rs = fifo_read8((uint8_t*)buf_start_addr, &t->tail, &t->head, buf_len, UDP_MTU, tmp_buf);
    memcpy(payload, tmp_buf, payload_len);
    if (rs && len)
        return (int64_t)len > payload_len ? payload_len : len;
}
int64_t read_socket_udp_blocking(struct net_tuple *t, uint8_t *payload, uint64_t payload_len)
{
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct net_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct net_tuple);
    memset(payload, 0, payload_len);
    buf_len = div_ceil(buf_len, UDP_MTU) * UDP_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    uint64_t len = len_records[t->head / UDP_MTU];
    uint8_t tmp_buf[UDP_MTU];
    memset(tmp_buf, 0, UDP_MTU);
    uint64_t rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, UDP_MTU, tmp_buf);
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
    rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, UDP_MTU, tmp_buf);
    memcpy(payload, tmp_buf, payload_len);
    return (int64_t)len > payload_len ? payload_len : len;
}
int64_t read_socket_udp(struct net_tuple *t, uint8_t *payload, uint64_t payload_len, bool flags_non_blocking)
{
    if (payload_len > UDP_MTU)
        return -EINVAL;
    if (t->receive_blocking && !flags_non_blocking)
        return read_socket_udp_blocking(t, payload, payload_len);
    else
        return read_socket_udp_non_blocking(t, payload, payload_len);
}
int64_t read_socket_by_id(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    // uint64_t socket_id = find_socket(tuple);
    struct net_tuple *t = socket_global[socket_id];
    if (!t)
        return -EBADF;
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct net_tuple);
    uint64_t buf_len = (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096 - sizeof(struct net_tuple);
    memset(payload, 0, payload_len);
    buf_len = div_ceil(buf_len, UDP_MTU) * UDP_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    uint64_t len = len_records[t->head / UDP_MTU];
    uint64_t rs = fifo_read8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, UDP_MTU, payload);
    return len;
}
int64_t udp_socket_append(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple *t = socket_global[socket_id];
    if (!t)
        return -EBADF;
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct net_tuple);
    uint64_t buf_len = (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096 - sizeof(struct net_tuple);
    buf_len = div_ceil(buf_len, UDP_MTU) * UDP_MTU;
    uint64_t *len_records = (uint64_t *)(((uint64_t)t) + (SOCKET_BLOCK_COUNT + UDP_ADDITIONAL_SOCKET_BLOCK_COUNT - 1) * 4096);
    len_records[t->head / UDP_MTU] = payload_len > UDP_MTU ? UDP_MTU : payload_len;
    uint64_t rs = fifo_write8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, UDP_MTU, payload);
    //=====================================
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i] && threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t) &t->recv_wait_flag)
        {
            threads_info_ptr[i]->state = THREAD_READY;
        }
    }
    //=====================================
    return (int64_t)rs;
}
int64_t udp_listen_append(uint64_t listen_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple *t = listen_global[listen_id];
    if (!t)
        return -EBADF;
    uint64_t buf_start_addr = ((uint64_t)t) + sizeof(struct net_tuple);
    uint64_t buf_len = SOCKET_BLOCK_COUNT * 4096 - sizeof(struct net_tuple);
    uint64_t rs = fifo_write8((uint8_t *)buf_start_addr, &t->tail, &t->head, buf_len, payload_len, payload);
    return (int64_t)rs;
}
bool handle_udp_packet(uint32_t src_ip, uint32_t dst_ip, uint32_t src_port, uint32_t dst_port, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple tmp;
    tmp.local_ip_h = 0;
    tmp.local_ip_l = src_ip;
    tmp.remote_ip_h = 0;
    tmp.remote_ip_l = dst_ip;
    tmp.remote_port = src_port;
    tmp.local_port = dst_port;
    tmp.protocol = UDP;
    tmp.version = 4;
    uint64_t socket_id = find_socket(&tmp);
    if (socket_id != -1)
    {
        udp_socket_append(socket_id, payload, payload_len);
        return true;
    }
    uint64_t listen_id = find_listen(&tmp);
    if (listen_id != -1)
    {
        udp_listen_append(socket_id, payload, payload_len);
        return true;
    }
    return false;
}
void handle_udp_v4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    struct eth_hdr *eth_header = (struct eth_hdr *)buf;
    struct ipv4_hdr *ip = (struct ipv4_hdr *)(buf + sizeof(struct eth_hdr));
    struct udp_hdr *header = (struct udp_hdr *)(buf + sizeof(struct eth_hdr) + sizeof(struct ipv4_hdr));
    uint64_t ip_header_len = ntohs(header->length);
    if (buf_len < ip_header_len + sizeof(struct udp_hdr))
        return; 

    struct udp_hdr *udp = header;

    uint32_t src_ip = ntohl(ip->src);
    uint32_t dst_ip = ntohl(ip->dst);
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dst_port = ntohs(udp->dst_port);
    uint16_t udp_len = ntohs(udp->length);

    uint8_t *payload = buf + +sizeof(struct eth_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr);
    uint64_t payload_len = udp_len - sizeof(struct udp_hdr);

    
    handle_udp_packet(src_ip, dst_ip, src_port, dst_port, payload, payload_len);
    return;
}
bool nic_send_udp_package(uint64_t socket_id, uint8_t *payload, uint64_t payload_len)
{
    struct net_tuple *sock = socket_global[socket_id];
    struct udp_hdr udp;
    memset(&udp, 0, sizeof(udp));

    udp.src_port = htons(sock->local_port);
    udp.dst_port = htons(sock->remote_port);
    udp.length = htons(sizeof(struct udp_hdr) + payload_len);
    udp.checksum = 0;

    if (!sock->local_ip_l)
        sock->local_ip_l = nic_global[1]->ip[0].addr_l;
    if (!sock->local_port)
        sock->local_port = allocate_random_high_port(UDP);
    
    
    uint32_t sum = 0;
    uint32_t src_ip = htonl(sock->local_ip_l);
    uint32_t dst_ip = htonl(sock->remote_ip_l);

    // pseudo header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += (src_ip) & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += (dst_ip) & 0xFFFF;
    sum += htons(UDP_PROTOCOL_NUMBER);
    sum += udp.length;

    // udp header
    uint16_t *ptr = (uint16_t *)&udp;
    uint64_t len = sizeof(struct udp_hdr);
    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len)
        sum += *((uint8_t *)ptr);

    // payload
    ptr = (uint16_t *)payload;
    len = payload_len;
    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len)
        sum += *((uint8_t *)ptr);

    
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    udp.checksum = ~sum;
    if (udp.checksum == 0)
        udp.checksum = 0xFFFF; 

    // udp.checksum =0;
    
    return nic_send_ipv4(sock->local_ip_l, sock->remote_ip_l, UDP_PROTOCOL_NUMBER, &udp, sizeof(udp), payload, payload_len);
}
#endif // NETWORKS_TCP_H
