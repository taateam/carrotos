#ifndef SYSCALL3_H
#define SYSCALL3_H

#define AF_UNSPEC 0      
#define AF_UNIX 1        // Local IPC (Unix domain socket)
#define AF_LOCAL AF_UNIX 
#define AF_INET 2        // IPv4 Internet protocols
#define AF_AX25 3        // Amateur radio AX.25
#define AF_IPX 4         // Novell IPX
#define AF_APPLETALK 5   // AppleTalk
#define AF_INET6 10      // IPv6 Internet protocols

#define SOCK_CLOEXEC 02000000  // close-on-exec
#define SOCK_NONBLOCK 00004000 // non-blocking mode


#define SOCK_STREAM 1 
#define SOCK_DGRAM 2  
#define SOCK_RAW 3    

// error codes
#define MAX_LISTEN_BACKLOG 128
uint64_t sys_socket(int domain, int type, int protocol, struct net_tuple **res)
{
    uint64_t res_id = 0;
    uint64_t pid = get_current_pid();
    // process_info_t **tmp = processes_info_ptr;
    struct file_inner *tmp1 = (struct file_inner *)processes_info_ptr[pid]->fd_addr;
    uint64_t fd_id = get_a_free_slot((uint64_t)tmp1, 0, MAX_FD_INNER);
    uint64_t global_fd_id = get_a_free_slot((uint64_t)&fd_global, 0, MAX_FD);
    if (domain == AF_INET)
    {
        if ((protocol == TCP || protocol == NONE) && type == SOCK_STREAM)
        {
            struct net_tuple_tcp tmp = {0};
            tmp.tuple.protocol = TCP;
            res_id = create_socket_tcp(pid, &tmp);
        }
        else if ((protocol == UDP || protocol == NONE) && type == SOCK_DGRAM)
        {
            struct net_tuple tmp = {0};
            tmp.protocol = TCP;
            res_id = create_socket_udp(pid, &tmp);
        }
        else
            return -EAFNOSUPPORT; 
        tmp1[fd_id].global_id = global_fd_id;
        tmp1[fd_id].offset = 0;
        tmp1[fd_id].type = FILE_SOCKET;
        tmp1[fd_id].res_id = res_id;
        fd_global[global_fd_id] = (struct file *)get_a_free_block_addr();
        fd_global[global_fd_id]->offset = 0;
        fd_global[global_fd_id]->type = FILE_SOCKET;
        return res_id;
    }
    else if (domain == AF_UNIX)
    {
        if (protocol == 0 && type == SOCK_STREAM)
        {
            struct uds_tuple_stream tmp = {0};
            tmp.tuple.type = UDS_STREAM;
            res_id = create_socket_stream(pid, &tmp);
        }
        else if (protocol == 0 && type == SOCK_DGRAM)
        {
            struct uds_tuple tmp = {0};
            tmp.type = UDS_DGRAM;
            res_id = create_socket_dgram(pid, &tmp);
        }
        else
            return -EAFNOSUPPORT; 
        tmp1[fd_id].global_id = global_fd_id;
        tmp1[fd_id].offset = 0;
        tmp1[fd_id].type = FILE_SOCKET_UDS;
        tmp1[fd_id].res_id = res_id;
        fd_global[global_fd_id] = (struct file *)get_a_free_block_addr();
        fd_global[global_fd_id]->offset = 0;
        fd_global[global_fd_id]->type = FILE_SOCKET_UDS;
        return fd_id;
    }
    else
        return -EAFNOSUPPORT;
}

struct sockaddr
{
    uint16_t sin_family; // AF_INET
    uint16_t sin_port;   // Port (network byte order - big endian)
    uint32_t sin_addr;   // IPv4 address (network byte order)
    char sin_zero[8];    
};
uint64_t sys_bind(uint64_t sockfd, const struct sockaddr *addr, uint64_t addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *proc = tmp[pid];
    struct file_inner *fd_table = (struct file_inner *)proc->fd_addr;

    if (sockfd >= MAX_FD || (fd_table[sockfd].type != FILE_SOCKET && fd_table[sockfd].type != FILE_SOCKET_UDS))
        return -EBADF;

    if (!addr)
        return -EINVAL;

    if (addrlen < sizeof(struct sockaddr) && addr->sin_addr == AF_INET)
        return -EINVAL;
    uint64_t res_id = fd_table[sockfd].res_id;
    // struct sockaddr_in *in = (struct sockaddr_in *)addr;

    if (addr->sin_family != AF_INET)
    {
        struct sockaddr_uds *addr_uds = (struct sockaddr_uds *)addr;
        struct uds_tuple *tuple = uds_socket_global[res_id];
        
        if (uds_addr_in_use(addr_uds->sun_path))
            return -EADDRINUSE;

        if (tuple->type == UDS_DGRAM)
            handle_dgram_bind(res_id, addr_uds->sun_path);
        else
            handle_stream_bind(res_id, addr_uds->sun_path);
        // tuple->is_bound = true;
    }
    else
    {
        
        struct net_tuple *tuple = socket_global[res_id];
        if (port_in_use(ntohl(addr->sin_addr), ntohs(addr->sin_port), tuple->protocol))
            return -EADDRINUSE;

        tuple->local_ip_l = ntohl(addr->sin_addr);
        tuple->local_port = ntohs(addr->sin_port);
        // tuple->is_bound = true;
    }
    return 0;
}

uint64_t sys_connect(uint64_t sockfd, struct sockaddr *addr, uint64_t addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *proc = tmp[pid];
    struct
        file_inner *fd_table = (struct file_inner *)proc->fd_addr;

    
    if (sockfd >= MAX_FD || fd_table[sockfd].type != FILE_SOCKET_UDS)
        return -EBADF;

    uint64_t res_id = fd_table[sockfd].res_id;
    struct net_tuple *tuple = (struct net_tuple *)uds_socket_global[res_id];

    if (!tuple)
        return -1;
    
    struct sockaddr *addr_in = (struct sockaddr *)addr;
    if (addr_in->sin_family == AF_UNIX)
    {
        struct sockaddr_uds *addr_uds = (struct sockaddr_uds *)addr;
        struct uds_tuple_stream *tuple = (struct uds_tuple_stream *)uds_socket_global[res_id];
        if (tuple->tuple.type == UDS_DGRAM)
            return handle_connect_dgram(res_id, (struct sockaddr_uds *)addr, addrlen);
        else if (tuple->tuple.type == UDS_STREAM)
        {
            return handle_connect_stream(res_id, (struct sockaddr_uds *)addr, addrlen);
        }
        else
            return -EPROTONOSUPPORT;
    }
    else if (addr_in->sin_family == AF_INET)
    {
        
        tuple->remote_ip_l = ntohl(addr_in->sin_addr);
        tuple->remote_port = ntohs(addr_in->sin_port);

        
        if (tuple->local_port == 0)
        {
            tuple->local_ip_l = INADDR_ANY;
            tuple->local_port = allocate_random_high_port(tuple->protocol);
        }

        
        if (tuple->protocol == TCP)
        {
            if (tuple->state == NET_STATE_CLOSED)
            {
                struct net_tuple_tcp *tcp_tuple = (struct net_tuple_tcp *)tuple;
                tcp_tuple->send_retry_count = 1;
                int64_t rs = tcp_send_syn(res_id);
                // if rs < 0 : sending arp;
                if (rs >= 0)
                    tuple->state = NET_STATE_SYN_SENT;
                return -EINPROGRESS;
            }
            else if (tuple->state == NET_STATE_SYN_SENT)
            {
                return -EINPROGRESS;
            }
            else if (tuple->state == NET_STATE_ESTABLISHED)
            {
                return 0;
            }
            else
                return -EISCONN;
            
        }

        
        else if (tuple->protocol == UDP)
        {
            tuple->state = NET_STATE_ESTABLISHED;
            return 0;
        }

        return -EPROTONOSUPPORT;
    }
    else
        return -EAFNOSUPPORT;
}
uint64_t sys_listen(uint64_t sockfd, int backlog)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *proc = tmp[pid];

    
    struct file_inner *fd_table = (struct file_inner *)proc->fd_addr;
    if (sockfd >= MAX_FD)
        return -EBADF;

    uint64_t res_id = fd_table[sockfd].res_id;
    if (fd_table[sockfd].type == FILE_SOCKET_UDS)
    {
        struct uds_tuple_stream *tuple = (struct uds_tuple_stream *)uds_socket_global[res_id];
        if (tuple->tuple.type != UDS_STREAM)
            return -EOPNOTSUPP; 
        return handle_stream_listen(res_id);
    }
    else if (fd_table[sockfd].type == FILE_SOCKET)
    {
        struct net_tuple_tcp *sock = (struct net_tuple_tcp *)socket_global[res_id];

        
        if (sock->tuple.protocol != TCP)
            return -EOPNOTSUPP; 

        
        if (sock->tuple.state != NET_STATE_CLOSED)
            return -EINVAL;

        
        if (backlog < 0)
            backlog = 0;
        else if (backlog > MAX_LISTEN_BACKLOG)
            backlog = MAX_LISTEN_BACKLOG;

        sock->backlog = backlog;
        sock->tuple.state = NET_STATE_LISTEN;

        uint64_t listen_id = create_listen(pid, &sock->tuple);
        sock->listen_id = listen_id;

        sock->tuple.last_updated_time_us = get_time_us();
        return 0;
    }
    else
        return -ENOTSOCK;
}
uint64_t sys_accept(uint64_t sockfd, struct sockaddr *addr, uint64_t *addrlen)
{
    // TODO: lock network
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *p = tmp[pid];
    struct file_inner *fdtab = (struct file_inner *)p->fd_addr;

    uint64_t listen_id = fdtab[sockfd].res_id;
    if (fdtab[sockfd].type == FILE_SOCKET_UDS)
    {
        struct sockaddr_uds *addr_uds = (struct sockaddr_uds *)addr;
        struct uds_tuple_stream *tuple = (struct uds_tuple_stream *)uds_socket_global[listen_id];
        if (tuple->tuple.type == UDS_DGRAM)
            return 0;
        else if (tuple->tuple.type == UDS_STREAM)
        {
            return handle_stream_accept(listen_id);
        }
        else
            return -EPROTONOSUPPORT;
    }
    else if (fdtab[sockfd].type == FILE_SOCKET)
    {
        // return -1;
        struct net_tuple_tcp *listener = (struct net_tuple_tcp *)socket_global[listen_id];

        if (listener->tuple.protocol != TCP)
            return -EINVAL;
        if (listener->tuple.state != NET_STATE_LISTEN)
            return -EINVAL;

        // sleep on socket;
        uint8_t *flag_ptr = (uint8_t *)&listener->listen_wait_flag;
        int64_t idx = find_ready_connection(listen_id);
        if (idx == -1)
        {
            sleep_by_resource((uint64_t)flag_ptr, THREAD_BLOCKED_NETWORK);
            
            idx = find_ready_connection(listen_id);
            if (idx == -1)
                return -EAGAIN; 
        }
        struct net_tuple *tmp1 = (struct net_tuple *)&socket_global;
        tmp1[idx].state = NET_STATE_ESTABLISHED;
        // struct net_tuple_tcp *child = listener->backlog_list[idx];
        // if(count_sockets_backlog(listen_id)==0)
        //     listener->wait_flag=0;
        uint64_t new_socket_fd_id = get_a_free_slot_with_size(tmp[pid]->fd_addr, 0, MAX_FD_INNER, sizeof(struct file_inner));
        struct file_inner *newfd = &fdtab[new_socket_fd_id];
        newfd->type = FILE_SOCKET;
        newfd->open_flags = fdtab[sockfd].open_flags; 
        newfd->res_id = idx;
        return new_socket_fd_id;
    }
    else
        return -ENOTSOCK;
}
uint64_t sys_sendto(uint64_t sockfd, uint8_t *buf, uint64_t len, int flags, const struct sockaddr *dest_addr, uint64_t addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *p = tmp[pid];
    struct file_inner *fd_sock = &((struct file_inner *)p->fd_addr)[sockfd];
    uint64_t res_id = fd_sock->res_id;
    if (fd_sock->type == FILE_SOCKET_UDS)
    {
        struct sockaddr_uds *ua = (struct sockaddr_uds *)dest_addr;
        if (ua)
        {
            return uds_send_dispatch(res_id, buf, len, ua->sun_path);
        }
        else
        {
            struct uds_tuple *sock = uds_socket_global[res_id];
            return uds_send_dispatch(res_id, buf, len, sock->remote_path);
        }
        // if (sock->state == UDS_STATE_CLOSED)
        //     return -1;
    }
    else if (fd_sock->type == FILE_SOCKET)
    {
        struct net_tuple *sock = socket_global[res_id];
        if (sock->state != NET_STATE_ESTABLISHED)
            return -1;
        if (sock->protocol == UDP)
        {
            if (sock->remote_ip_l && ntohl(dest_addr->sin_addr) != sock->remote_ip_l)
                return -1;
            if (sock->remote_port && ntohs(dest_addr->sin_port) != sock->remote_port)
                return -1;
            sock->remote_ip_l = ntohl(dest_addr->sin_addr);
            sock->remote_port = ntohs(dest_addr->sin_port);
            return nic_send_udp_package(res_id, buf, len);
        }
        else if (sock->protocol == TCP)
        {
            if (dest_addr && sock->remote_ip_l && ntohl(dest_addr->sin_addr) != sock->remote_ip_l)
                return -1;
            if (dest_addr && sock->remote_port && ntohs(dest_addr->sin_port) != sock->remote_port)
                return -1;
            if (dest_addr)
            {
                sock->remote_ip_l = ntohl(dest_addr->sin_addr);
                sock->remote_port = ntohs(dest_addr->sin_port);
            }
            return tcp_copy_payload_and_send_established(res_id, (uint8_t *)buf, len);
        }
        return len;
    }
    else
        return -ENOTSOCK;
}
uint64_t sys_recvfrom(uint64_t sockfd, uint8_t *buf, uint64_t len, int flags, struct sockaddr *src_addr, uint64_t *addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *p = tmp[pid];
    struct file_inner *fd_sock = &((struct file_inner *)p->fd_addr)[sockfd];
    uint64_t res_id = fd_sock->res_id;
    if (fd_sock->type == FILE_SOCKET_UDS)
    {
        struct uds_tuple *sock = uds_socket_global[res_id];
        if (sock->type == UDS_DGRAM)
        {
            read_socket_dgram(sock, buf, len, flags & MSG_DONTWAIT > 0);
        }
        else if (sock->type == UDS_STREAM)
        {
            read_socket_stream(res_id, buf, len);
        }
        return len;
    }
    else if (fd_sock->type == FILE_SOCKET)
    {
        struct net_tuple *sock = socket_global[res_id];
        if (sock->protocol == UDP)
        {
            read_socket_udp(sock, buf, len, flags & MSG_DONTWAIT > 0);
        }
        else if (sock->protocol == TCP)
        {
            read_socket_tcp(res_id, buf, len);
        }
        return len;
    }
    else
        return -ENOTSOCK;
}
#define SHUT_RD 0   
#define SHUT_WR 1   
#define SHUT_RDWR 2 
uint64_t sys_shutdown(uint64_t sockfd, int how)
{
    uint64_t pid = get_current_pid();
    process_info_t *p = processes_info_ptr[pid];

    if (sockfd >= MAX_FD_INNER)
        return -EBADF;

    struct file_inner *fd_sock = &((struct file_inner *)p->fd_addr)[sockfd];
    if (fd_sock->type != FILE_SOCKET)
        return -ENOTSOCK;

    uint64_t res_id = fd_sock->res_id;
    if (fd_sock->type == FILE_SOCKET_UDS)
    {
        struct uds_tuple *sock = uds_socket_global[res_id];
        struct file_inner zero = {0};
        *fd_sock = zero;
        *(uint64_t *)p->fd_addr = 0;
        return delete_socket_i_uds(res_id);
    }
    else if (fd_sock->type == FILE_SOCKET)
    {
        struct net_tuple *sock = socket_global[res_id];

        if (sock->protocol == TCP)
        {
            if (how == SHUT_RD)
            {
                // sock->can_read = false;
                
            }
            else if (how == SHUT_WR)
            {
                // sock->can_write = false;
                if (sock->state == NET_STATE_ESTABLISHED)
                    sock->state = NET_STATE_FIN_WAIT_1;
                else if (sock->state == NET_STATE_CLOSE_WAIT)
                    sock->state = NET_STATE_LAST_ACK;
                else
                    sock->state = NET_STATE_CLOSING; 
                tcp_send_fin(res_id);                
            }
            else if (how == SHUT_RDWR)
            {
                // sock->can_read = false;
                // sock->can_write = false;
                tcp_send_fin(res_id);
                if (sock->state == NET_STATE_ESTABLISHED)
                    sock->state = NET_STATE_FIN_WAIT_1;
                else if (sock->state == NET_STATE_CLOSE_WAIT)
                    sock->state = NET_STATE_LAST_ACK;
                else
                    sock->state = NET_STATE_CLOSING;
            }
            else
            {
                return -EINVAL;
            }
        }
        else if (sock->protocol == UDP)
        {
            
            if (how == SHUT_RD)
            {
                sock->state = NET_STATE_CLOSED;
                // sock->can_read = false;
            }
            else if (how == SHUT_WR)
            {
                sock->state = NET_STATE_CLOSED;
                // sock->can_write = false;
            }
            else if (how == SHUT_RDWR)
            {
                sock->state = NET_STATE_CLOSED;
                // sock->can_read = false;
                // sock->can_write = false;
            }
            else
                return -EINVAL;
        }
        else
        {
            return -EINVAL;
        }

        return 0;
    }
    else
        return -ENOTSOCK;
};
int sys_getsockname(uint64_t sockfd, struct sockaddr *addr, uint64_t *addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *p = tmp[pid];
    struct file_inner *fd_sock = &((struct file_inner *)p->fd_addr)[sockfd];
    uint64_t res_id = fd_sock->res_id;
    struct net_tuple *sock = &((struct net_tuple *)socket_global)[res_id];
    addr->sin_addr = sock->local_ip_l;
    addr->sin_port = sock->local_port;
    addr->sin_family = AF_INET;
    // addr->sin_zero;
    return 0;
};
int sys_getpeername(uint64_t sockfd, struct sockaddr *addr, uint64_t *addrlen)
{
    uint64_t pid = get_current_pid();
    process_info_t **tmp = processes_info_ptr;
    process_info_t *p = tmp[pid];
    struct file_inner *fd_sock = &((struct file_inner *)p->fd_addr)[sockfd];
    uint64_t res_id = fd_sock->res_id;
    struct net_tuple *sock = &((struct net_tuple *)socket_global)[res_id];
    addr->sin_addr = sock->remote_ip_l;
    addr->sin_port = sock->remote_port;
    addr->sin_family = AF_INET;
    // addr->sin_zero;
    return 0;
};
int sys_setsockopt(uint64_t sockfd, int level, int optname, const void *optval, uint64_t optlen)
{
    return 0;
};
uint64_t sys_time(uint64_t *user_buf)
{
    uint64_t t = get_time_s();

    if (user_buf)
        *user_buf = t;

    return t;
}
int sys_gettimeofday(struct timeval *tv, void *tz)
{
    if (!tv)
        return -1;

    struct timeval tmp = get_time_us();
    tv->tv_sec = tmp.tv_sec;
    tv->tv_microsec = tmp.tv_microsec;

    return 0;
}
int sys_clock_gettime(int clock_id, struct timespec *tp)
{
    if (!tp)
        return -1;

    struct timeval tmp = get_time_us();
    tp->tv_sec = tmp.tv_sec;
    tp->tv_nanosec = get_time_ns_in_second();

    return 0;
}
int sys_clock_getres(int clock_id, struct timespec *res)
{
    if (!res)
        return -1;

    res->tv_sec = 0;
    res->tv_nanosec = 1; 
    return 0;
}
int sys_clock_settime(int clock_id, const struct timespec *tp)
{
    if (!tp)
        return -1;

    uint64_t sec = tp->tv_sec;

    
    uint64_t days = sec / 86400ULL;
    uint64_t rem = sec % 86400ULL;

    int hour = rem / 3600;
    rem %= 3600;
    int min = rem / 60;
    int s = rem % 60;

    
    int year = 1970;
    while (1)
    {
        int diy = is_leap(year) ? 366 : 365;
        if (days < diy)
            break;
        days -= diy;
        year++;
    }

    int mon = 1;
    while (1)
    {
        int dim = days_in_month[mon - 1];
        if (mon == 2 && is_leap(year))
            dim++;
        if (days < dim)
            break;
        days -= dim;
        mon++;
    }

    int day = days + 1;

    rtc_write_datetime(year, mon, day, hour, min, s);
    return 0;
}
struct timex
{
    int modes;
    int64_t offset;
    int64_t freq;
    int64_t maxerror;
    int64_t esterror;
    int status;
    int64_t constant;
    int64_t precision;
    int64_t tolerance;
    struct
    {
        uint64_t tv_sec;
        uint64_t tv_usec;
    } time;
    int64_t tick;
};


int sys_adjtimex(struct timex *tx)
{
    if (!tx)
        return -1;

    
    tx->offset = 0;
    tx->freq = 0;
    tx->maxerror = 0;
    tx->esterror = 0;
    tx->status = 0;
    tx->constant = 0;
    tx->precision = 1;
    tx->tolerance = 0;
    tx->time.tv_sec = 0;
    tx->time.tv_usec = 0;
    tx->tick = 10000; 

    return 0;
}
#endif // SYSCALL3_H
