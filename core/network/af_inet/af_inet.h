#ifndef AF_INET_H
#define AF_INET_H
#include <stdint.h>


#define TCP_FIN 0x01 
#define TCP_SYN 0x02 
#define TCP_RST 0x04 
#define TCP_PSH 0x08 
#define TCP_ACK 0x10 
#define TCP_URG 0x20 
#define TCP_ECE 0x40 
#define TCP_CWR 0x80 
#define TCP_NS 0x100 

#define INADDR_ANY ((uint32_t)0x00000000)       // 0.0.0.0
#define INADDR_BROADCAST ((uint32_t)0xFFFFFFFF) // 255.255.255.255

#define MAX_SOCKETS_COUNT 5120
#define MAX_LISTENS_COUNT 5120
#define TCP_MAX_RETRIES 4
#define UDP_MTU 1472
#define TCP_MTU 1500
#define TCP_TIMEWAIT_TIMEOUT_SECONDS 60
typedef uint8_t netmask_t[4];
//==========================================================
bool net_tuple_match_v4(struct net_tuple *src, struct net_tuple *target)
{
    if (src == target)
        return true;
    if (!src || !target)
        return false;
    if (src->protocol != target->protocol)
        return false;
    if (src->local_ip_l != target->local_ip_l && target->local_ip_l)
        return false;
    if (src->local_port != target->local_port)
        return false;
    if (src->remote_ip_l != target->remote_ip_l && target->remote_ip_l)
        return false;
    if (src->remote_port != target->remote_port && target->remote_port)
        return false;
    if (src->version != target->version)
        return false;

    return true;
}
bool net_tuple_equal_v4(struct net_tuple *src, struct net_tuple *target)
{
    if (src == target)
        return true;
    if (!src || !target)
        return false;
    if (src->state == NET_STATE_LISTEN)
        return false;
    if (src->protocol != target->protocol)
        return false;
    if (src->local_ip_l != target->local_ip_l)
        return false;
    if (src->local_port != target->local_port)
        return false;
    if (src->remote_ip_l != target->remote_ip_l)
        return false;
    if (src->remote_port != target->remote_port)
        return false;
    if (src->version != target->version)
        return false;

    return true;
}
uint32_t find_1st_ipv4(struct nic_t *this_nic)
{
    struct ip_t *ips = this_nic->ip;
    for (uint64_t i = 0; i < NETWORKS_KSIZE; i++)
    {
        if (ips[i].ver == 4)
            return (uint32_t)ips[i].addr_l;
    }
    return 0; // error;
};
uint64_t register_nic(uint8_t mac_input[6])
{
    uint64_t nic_id = get_a_free_slot((uint64_t)&nic_global, 1, MAX_NIC_COUNT);
    if (nic_id == -1)
        return -1;
    struct nic_t *nic = (struct nic_t *)get_a_free_block_addr();
    memcpy(nic->mac, mac_input, 6);
    nic->type = 1;
    nic_global[nic_id] = nic;
    // nic->ip[0] = nic;
    // memcpy((uint8_t *)nic_global[nic_id], (uint8_t *)&nic, sizeof(struct nic_t));
    return nic_id;
}
// routes =====================================================
uint32_t char_to_netmask(uint8_t n);
uint32_t find_first_gateway();
struct route_entry *find_route(uint32_t dest_ip);
bool in_l2(uint32_t ip_a, uint32_t ip_b, uint32_t netmask)
{
    return (ip_a & netmask) == (ip_b & netmask);
}
bool find_mac_of_ipv4_l2(uint32_t ip, uint64_t nic_id, uint8_t *mac)
{
    for (uint64_t i = 0; i < Z40KB / 168; i++)
    {
        if (mac_cache_v4[i].ip == ip && mac_cache_v4[i].timestamp > get_epoch_seconds() - MAC_CACHE_TIMEOUT / 1000ULL)
        {
            memcpy(mac, mac_cache_v4[i].mac, 6);
            return true;
        }
    }
    send_arp(ip, nic_id, true);
    return false;
}
bool find_mac_of_ipv4_l3(uint32_t ip, uint64_t nic_id, uint8_t *mac)
{
    struct route_entry *route = find_route(ip);
    if (route)
        return find_mac_of_ipv4_l2(route->gateway, nic_id, mac);
    uint32_t gateway = find_first_gateway();
    if (!gateway)
        return false;
    return find_mac_of_ipv4_l2(gateway, nic_id, mac);
}
bool find_mac_of_ipv4(uint32_t ip, uint64_t nic_id, uint8_t *mac)
{
    struct nic_t *nic = nic_global[nic_id];
    uint32_t netmask = char_to_netmask(nic->ip[0].masklen);
    if (in_l2(ip, (uint32_t)nic->ip[0].addr_l, netmask))
    {
        return find_mac_of_ipv4_l2(ip, nic_id, mac);
    }
    else
    {
        return find_mac_of_ipv4_l3(ip, nic_id, mac);
    };
}
uint64_t find_nic_id_by_ipv4(uint32_t src_ip)
{
    if (!src_ip)
        return 1;
    for (uint64_t i = 0; i < Z40KB / 168; i++)
    {
        if (!nic_global[i])
            continue;
        struct ip_t *ip = nic_global[i]->ip;
        for (uint64_t j = 0; j < MAX_IP_PER_NIC; j++)
        {
            if (!ip[j].addr_l)
                continue;
            struct ip_t an_ip = ip[j];
            if (src_ip == an_ip.addr_l && an_ip.ver == 4)
                return i;
        };
    }
    return -1;
}
//! routes =====================================================

uint16_t network_checksum(uint8_t *buf, size_t len)
{
    uint16_t *data = (uint16_t *)buf;
    uint32_t sum = 0;

    
    while (len > 1)
    {
        sum += *data++;
        len -= 2;
    }

    
    if (len == 1)
    {
        sum += *((uint8_t *)data);
    }

    
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum; // 1’s complement
}
extern struct net_tuple *listen_global[MAX_LISTENS_COUNT];
extern struct net_tuple *socket_global[MAX_SOCKETS_COUNT];
struct net_tuple_tcp **socket_global1 = (struct net_tuple_tcp **)socket_global;
uint64_t find_ipv4_of_nic(uint64_t nic_id, uint32_t res_ip)
{
    for (uint64_t i = 0; i < MAX_IP_PER_NIC; i++)
    {
        uint32_t tmp = (uint32_t)(nic_global[nic_id]->ip[i].addr_l);
        if (tmp == res_ip)
            return i;
    }
    return -1;
}
uint32_t get_ipv4(uint64_t nic_id, uint64_t ip_id)
{
    struct nic_t *this_nic = (struct nic_t *)&nic_global[nic_id];
    if (this_nic->type == 0)
        return 0;
    struct ip_t *this_ip = &this_nic->ip[ip_id];
    if (this_ip->status == IP_NOT_EXISTS)
        return 0;
    if (this_ip->status == IP_ASSIGNED)
        return (uint32_t)this_ip->addr_l;
    // status == IP_PENDING;
    struct timeval current_time = get_time_us();
    struct timeval timedout_time = add_time_us(this_ip->init_time, ARP_WAIT_TIME);
    if (_later(timedout_time, current_time))
    {
        return 0; // do nothing;
    }
    else
    {
        this_ip->status = IP_ASSIGNED;
        return (uint32_t)this_ip->addr_l;
    };
}
bool check_ipv4(uint64_t nic_id, uint32_t ip)
{
    uint64_t matched_id = find_ipv4_of_nic(nic_id, ip);
    if (matched_id == -1)
        return false; 

    struct timeval current_time = get_time_us();
    if (nic_global[nic_id]->ip[matched_id].status == IP_ASSIGNED)
        return true;
    // case IP_PENDING;
    struct timeval expired_time = add_time_us(nic_global[nic_id]->ip[matched_id].init_time, ARP_WAIT_TIME);
    if (_later(expired_time, current_time))
    {
        return false; 
    }
    
    nic_global[nic_id]->ip[matched_id].status = IP_ASSIGNED;
    return true;
};
uint64_t add_cache_mac_v4_line(uint64_t i, uint32_t ip, uint8_t *mac, uint64_t nic_id)
{
    mac_cache_v4[i].ip = ip;
    memcpy(mac_cache_v4[i].mac, mac, 6);
    mac_cache_v4[i].nic_id = nic_id;
    mac_cache_v4[i].timestamp = get_epoch_seconds();
}
uint64_t remove_cache_mac_v4(uint64_t i)
{
    mac_cache_v4[i].ip = 0;
    memset(mac_cache_v4[i].mac, 0, 6);
    mac_cache_v4[i].nic_id = 0;
    mac_cache_v4[i].timestamp = 0;
}
uint64_t add_cache_mac_v4(uint32_t ip, uint8_t *mac, uint64_t nic_id)
{
    for (uint64_t i = 0; i < Z40KB / 168; i++)
    {
        if (mac_cache_v4[i].timestamp <= subtract_time_us(get_time_us(), MAC_CACHE_TIMEOUT).tv_sec)
        {
            remove_cache_mac_v4(i);
        }
        else if (mac_cache_v4[i].ip == ip && memcmp(&mac_cache_v4[i].mac, mac, 6) == 0)
        {
            return -1;
        }
    };
    for (uint64_t i = 0; i < Z40KB / 168; i++)
    {
        if (mac_cache_v4[i].ip == 0)
        {
            add_cache_mac_v4_line(i, ip, mac, nic_id);
            return i;
        };
    };
    uint64_t earliest_timestamp = get_epoch_seconds();
    uint64_t earliest_i = 0;
    for (uint64_t i = 0; i < Z40KB / 168; i++)
    {
        if (mac_cache_v4[i].timestamp < earliest_timestamp)
        {
            earliest_i = i;
            earliest_timestamp = mac_cache_v4[i].timestamp;
        }
    }
    add_cache_mac_v4_line(earliest_i, ip, mac, nic_id);
    return earliest_i;
}
uint32_t ip_str_to_int(const char *ip_str)
{
    uint32_t result = 0;
    uint32_t octet = 0;
    int dots = 0;

    while (*ip_str)
    {
        char c = *ip_str++;
        if (c >= '0' && c <= '9')
        {
            octet = octet * 10 + (c - '0');
            if (octet > 255)
            {
                return 0; // invalid
            }
        }
        else if (c == '.')
        {
            result = (result << 8) | octet;
            octet = 0;
            dots++;
            if (dots > 3)
            {
                return 0; 
            }
        }
        else
        {
            return 0; 
        }
    }

    if (dots != 3)
    {
        return 0; 
    }

    result = (result << 8) | octet;
    return result;
}
//=========================================================================
uint64_t resume_first_thread_pending_on_listen(uint64_t listen_id)
{
    thread_info_t **tmp = threads_info_ptr;
    uint64_t flag_addr = (uint64_t)&((struct net_tuple_tcp **)socket_global)[listen_id]->listen_wait_flag;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *pointed_thread = tmp[i];
        if (pointed_thread && pointed_thread->state == THREAD_BLOCKED_NETWORK && pointed_thread->sleep_flag_addr == flag_addr)
        {
            tmp[i]->state = THREAD_READY;
            return i;
        }
    }
    return -1;
}
#define HIGH_PORT_START 49152
#define HIGH_PORT_END 65535

static uint16_t next_ephemeral_port = HIGH_PORT_START;

uint64_t port_in_use(uint32_t ip, uint16_t port, enum protocol protocol)
{
    for (int i = 0; i < NETWORKS_KSIZE; i++)
    {
        struct net_tuple *t = (struct net_tuple *)socket_global[i];
        if (t == 0)
            continue;

        if (t->protocol == protocol &&
            t->local_port == port &&
            (t->local_ip_l == ip || t->local_ip_l == INADDR_ANY))
        {
            return 1; // found conflict
        }
    }
    return 0;
}
uint16_t allocate_random_high_port(enum protocol protocol)
{
    for (int i = 0; i < (HIGH_PORT_END - HIGH_PORT_START); i++)
    {
        uint16_t port = next_ephemeral_port++;

        if (next_ephemeral_port > HIGH_PORT_END)
            next_ephemeral_port = HIGH_PORT_START;

        if (!port_in_use(INADDR_ANY, port, protocol))
            return port;
    }

    
    return 0; 
}
uint64_t pull_backlog(uint64_t listener_id);
int64_t find_ready_connection(uint64_t listen_id)
{
    uint64_t rs = pull_backlog(listen_id);
    return rs;
}
uint64_t count_sockets_backlog(uint64_t listen_id)
{
    struct net_tuple **tmp = socket_global;
    struct net_tuple *tmp1 = (struct net_tuple *)&listen_global[listen_id];
    uint64_t count = 0;
    for (uint64_t i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        if (tmp[i]->local_ip_l == tmp1->local_ip_l && tmp[i]->remote_ip_l == tmp1->remote_ip_l &&
            tmp[i]->local_port == tmp1->local_port && tmp[i]->remote_port == tmp1->remote_port &&
            tmp[i]->protocol == tmp1->protocol && tmp[i]->state == NET_STATE_SYN_RECEIVED)
            count++;
    }
    return count;
}
uint64_t register_ipv4(uint64_t nic_id, char *ip)
{
    uint32_t *ips = (uint32_t *)nic_global[nic_id]->ip;
    uint64_t i = 0;
    for (; i < NET_BLOCK_SIZE / IP4_SIZE; i++)
    {
        if (ips[i] != 0)
            continue;
        uint32_t ip_int = ip_str_to_int(ip);
        ips[i] = ip_int;
        break;
    }
    return i;
}
uint64_t create_listen(uint64_t pid, struct net_tuple *tuple)
{
    uint64_t i = 0;
    for (; i < MAX_LISTENS_COUNT; i++)
    {
        if (listen_global[i] == 0)
            continue;
        struct net_tuple *tmp = listen_global[i];
        if (memcmp(tmp, tuple, sizeof(struct net_tuple)) == 0)
            return -1; // error duplicate listen/soccket;
    }
    for (i = 0; i < MAX_LISTENS_COUNT / 8; i++)
    {
        if (listen_global[i] != 0)
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(2) * 4096 + HM;
        listen_global[i] = (struct net_tuple *)new_free_block_addr;
        uint64_t *new_free_block_ptr = (uint64_t *)new_free_block_addr;
        memcpy((uint8_t *)new_free_block_ptr, (uint8_t *)tuple, sizeof(struct net_tuple));
        break;
    }
    return i;
}
uint64_t create_socket_udp(uint64_t pid, struct net_tuple *tuple)
{
    uint64_t i;
    for (i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        if (socket_global[i] == 0)
            continue;
        struct net_tuple *tmp = socket_global[i];
        if (memcmp(tmp, tuple, sizeof(struct net_tuple)) == 0)
            return -1; // error duplicate listen/socket;
    }
    for (i = 0; i < MAX_SOCKETS_COUNT / 8; i++)
    {
        if (socket_global[i] && socket_global[i]->version != 0)
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(SOCKET_BLOCK_COUNT) * 4096 + HM;
        socket_global[i] = (struct net_tuple *)new_free_block_addr;
        uint64_t *new_free_block_ptr = (uint64_t *)new_free_block_addr;
        memcpy((uint8_t *)new_free_block_ptr, (uint8_t *)tuple, sizeof(struct net_tuple));
        socket_global[i]->state = NET_STATE_ESTABLISHED;
        socket_global[i]->protocol = UDP;
        socket_global[i]->version = 4;
        socket_global[i]->head = 0;
        socket_global[i]->tail = 0;
        socket_global[i]->receive_blocking = true;
        break;
    }
    return i;
}
uint64_t create_socket_tcp(uint64_t pid, struct net_tuple_tcp *original_tcp_tuple)
{
    uint64_t i = 0;
    for (; i < MAX_SOCKETS_COUNT; i++)
    {
        if (socket_global[i] == 0)
            continue;
        struct net_tuple_tcp *tmp = (struct net_tuple_tcp *)socket_global[i];
        if (memcmp(tmp, original_tcp_tuple, sizeof(struct net_tuple_tcp)) == 0)
            return -1; // error duplicate listen/soccket;
    }
    for (i = 0; i < MAX_SOCKETS_COUNT; i++)
    {
        if (socket_global[i])
            continue;
        uint64_t new_free_block_addr = get_continuous_free_blocks(SOCKET_BLOCK_COUNT + TCP_ADDITIONAL_SOCKET_BLOCK_COUNT) * 4096 + HM;
        struct net_tuple_tcp *new_tcp_tuple = (struct net_tuple_tcp *)new_free_block_addr;
        socket_global[i] = (struct net_tuple *)new_tcp_tuple;
        // uint64_t *new_free_block_ptr = (uint64_t *)new_free_block_addr;
        memcpy((uint8_t *)new_tcp_tuple, (uint8_t *)original_tcp_tuple, sizeof(struct net_tuple_tcp));
        new_tcp_tuple->tuple.state = NET_STATE_CLOSED;
        new_tcp_tuple->tuple.protocol = TCP;
        new_tcp_tuple->tuple.version = 4;
        new_tcp_tuple->tuple.head = 0;
        new_tcp_tuple->tuple.tail = 0;
        new_tcp_tuple->tuple.receive_blocking = true;
        new_tcp_tuple->seq_local = 1000000000ULL * rand_range(1, 3) + rand_range(1, 2000000000ULL);
        new_tcp_tuple->ack_est_blocked = false;
        break;
    }
    return i;
}
uint64_t delete_listen(uint64_t pid, struct net_tuple *tuple)
{
    uint64_t i = 0;
    for (; i < MAX_LISTENS_COUNT; i++)
    {
        if (listen_global[i] == 0)
            continue;
        struct net_tuple *tmp = socket_global[i];
        uint64_t tmp_addr = (uint64_t)tmp;
        if (memcmp(tmp, tuple, sizeof(struct net_tuple)) == 0)
        {
            erase_block((uint64_t *)tmp_addr);
            erase_block((uint64_t *)(tmp_addr + MAX_LISTENS_COUNT));
            listen_global[i] = 0;
            return 0; // found listen/socket;
        }
    }
    return -1;
}
void mark_socket_as_deleted(uint64_t id)
{
    struct net_tuple *t = socket_global[id];
    t->state = NET_STATE_CLOSE_WAIT;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t)&t->recv_wait_flag)
            threads_info_ptr[i]->state == THREAD_READY;
    }
    if (t->protocol != TCP)
    {
        return;
    }
    struct net_tuple_tcp *tcp = (struct net_tuple_tcp *)socket_global[id];
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i]->state == THREAD_BLOCKED_NETWORK && threads_info_ptr[i]->sleep_flag_addr == (uint64_t)&tcp->send_wait_flag)
            threads_info_ptr[i]->state == THREAD_READY;
    }
}
uint64_t delete_socket(uint64_t pid, struct net_tuple *tuple)
{
    uint64_t i = 0;
    for (; i < MAX_SOCKETS_COUNT; i++)
    {
        if (socket_global[i]->version == 0)
            continue;
        struct net_tuple *tmp = socket_global[i];
        uint64_t tmp_addr = (uint64_t)tmp;
        if (memcmp(tmp, tuple, sizeof(struct net_tuple)) == 0)
        {
            for (int j = 0; j < SOCKET_BLOCK_COUNT; j++)
            {
                erase_block((uint64_t *)(tmp_addr + j * NET_BLOCK_SIZE));
                set_memory_block_used_when_boot(tmp_addr / 4096 + j, false);
            }
            socket_global[i] = 0;
            return 0; // found listen/socket;
        }
    }
    return -1;
}
uint64_t delete_socket_i(uint64_t id)
{
    if (socket_global[id] == 0)
        return -1;
    struct net_tuple *tmp = socket_global[id];
    uint64_t tmp_addr = (uint64_t)tmp;
    for (int j = 0; j < SOCKET_BLOCK_COUNT; j++)
    {
        erase_block((uint64_t *)(tmp_addr + j * NET_BLOCK_SIZE));
        set_memory_block_used_when_boot(tmp_addr / 4096 + j, false);
    }
    socket_global[id] = 0;
    return 0; // found listen/socket;
}
//=======================
#include "lo.h"
#include "udp.h"
#include "tcp.h"
//=======================

void handle_ipv4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len);
void handle_ping_v4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len);
void handle_arp(uint64_t nic_id, uint8_t *buf, uint64_t buf_len);
void nic_send_dispatch(uint64_t nic_id, void *data, uint64_t len)
{
    if (nic_id == 0)
    {
        handle_ipv4(nic_id, data, len);
    }
    else
    {
        e1000_send(nic_global[nic_id]->mmio_base, data, len);
    };
};
void handle_ipv4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    struct eth_hdr *eth = (struct eth_hdr *)(buf);
    struct ipv4_hdr *ip = (struct ipv4_hdr *)(buf + sizeof(struct eth_hdr));

    add_cache_mac_v4(ntohl(ip->src), eth->src, 1);
    // TODO: nic_id update later;
    if (ip->protocol == ICMP_PROTOCOL_NUMBER)
        return handle_ping_v4(nic_id, buf, buf_len);

    if (ip->protocol == UDP_PROTOCOL_NUMBER)
    {
        return handle_udp_v4(nic_id, buf, buf_len);
    }
    if (ip->protocol == TCP_PROTOCOL_NUMBER)
    {
        return handle_tcp_v4(nic_id, buf, buf_len);
    };
}
void handle_ipv6(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    ;
}
void nic_isr(uint64_t nic_id)
{
    uint64_t mmio_base = nic_global[nic_id]->mmio_base;
    uint8_t buf[2048];
    int len = e1000_recv(mmio_base, buf, sizeof(buf));
    if (len <= 0)
        return;

    struct eth_hdr *eth = (struct eth_hdr *)buf;
    uint16_t eth_type = ntohs(eth->type);

    switch (eth_type)
    {
    case 0x0806: // ARP
        handle_arp(nic_id, buf,
                   len);
        break;

    case 0x0800: // IPv4
        handle_ipv4(nic_id, buf,
                    len);
        break;

    case 0x86DD: // IPv6
        handle_ipv6(nic_id, buf + sizeof(struct eth_hdr),
                    len - sizeof(struct eth_hdr));
        break;

    default:
        // unknown or unsupported
        break;
    };
}
void nic0_isr()
{
    uint64_t nic_id = 1;
    nic_isr(nic_id);
    // e1000_lapic_eoi(nic_id);
}
uint8_t netmask_to_char(netmask_t n)
{
    uint32_t tmp = *(uint32_t *)&n[0];
    uint8_t i = 0;
    for (; i < 32; i++)
    {
        if (!(tmp & (1 << i)))
            break;
    }
    return i;
}
uint32_t char_to_netmask(uint8_t n)
{
    uint32_t tmp = 0;
    uint8_t i = 0;
    for (; i < n; i++)
    {
        tmp |= (1 << i);
    }
    return tmp;
}
bool add_route(uint32_t dest, uint32_t netmask, uint32_t gateway, uint64_t nic_id)
{
    uint64_t id = get_a_free_slot_with_size((uint64_t)&route_table_v4, 0, Z40KB / sizeof(struct route_entry), sizeof(struct route_entry));
    struct route_entry *r = (struct route_entry *)&route_table_v4[id];
    r->dest = dest;
    r->netmask = netmask;
    r->gateway = gateway;
    r->nic_id = nic_id;
    return true;
}

bool del_route(uint32_t dest, uint32_t netmask)
{
    for (uint32_t i = 0; i < Z40KB / sizeof(struct route_entry); i++)
    {
        struct route_entry *r = &route_table_v4[i];
        if (r->dest == dest && r->netmask == netmask)
        {
            erase_mem8((uint64_t)&route_table_v4[i], sizeof(struct route_entry) / 8);
            return true;
        }
    }
    return false;
}


struct route_entry *find_route_n_gate(uint32_t dest_ip)
{
    struct route_entry *best = NULL;
    uint32_t best_mask = 0;

    for (uint32_t i = 0; i < Z40KB / sizeof(struct route_entry); i++)
    {
        struct route_entry *r = &route_table_v4[i];
        if (r->dest && r->netmask && (dest_ip & r->netmask) == (r->dest & r->netmask))
        {
            if (r->netmask >= best_mask)
            {
                best = r;
                best_mask = r->netmask;
            }
        }
    }

    
    if (!best)
    {
        for (uint32_t i = 0; i < Z40KB / sizeof(struct route_entry); i++)
        {
            struct route_entry *r = &route_table_v4[i];
            if (r->dest == 0 && r->netmask == 0)
            {
                return r;
            }
        }
    }
    return best;
}
struct route_entry *find_route(uint32_t dest_ip)
{
    struct route_entry *best = NULL;
    uint32_t best_mask = 0;

    for (uint32_t i = 0; i < Z40KB / sizeof(struct route_entry); i++)
    {
        struct route_entry *r = &route_table_v4[i];
        if (r->dest && r->netmask && (dest_ip & r->netmask) == (r->dest & r->netmask))
        {
            if (r->netmask >= best_mask)
            {
                best = r;
                best_mask = r->netmask;
            }
        }
    }

    return best;
}
uint32_t find_first_gateway()
{
    for (uint64_t i = 0; i < MAX_NIC_COUNT; i++)
    {
        if (nic_global[i] && nic_global[i]->gateway)
            return nic_global[i]->gateway;
    }
    return 0;
}
uint32_t get_next_hop(uint32_t dest_ip)
{
    struct route_entry *r = find_route(dest_ip);
    if (r)
        return r->gateway; // no route
    uint32_t gateway = find_first_gateway();
    if (gateway != 0)
        return gateway; // indirect route
    return dest_ip;     // direct (same subnet)
}
void send_arp(uint32_t ip, uint64_t nic_id, bool ip_registered)
{
    struct nic_t *this_nic = nic_global[nic_id];
    uint8_t my_mac[6];
    memcpy(my_mac, this_nic->mac, 6);
    // uint64_t mmio_base = this_nic->mmio_base;
    uint8_t buf[64]; 
    struct eth_hdr *eth = (struct eth_hdr *)buf;
    struct arp_pkt *arp = (struct arp_pkt *)(buf + sizeof(struct eth_hdr));

    
    uint32_t target_ip = get_next_hop(ip);
    if (target_ip == 0)
    {
        return;
    }
    // Ethernet header
    memset(eth->dst, 0xFF, 6);   // broadcast
    memcpy(eth->src, my_mac, 6); // source MAC
    eth->type = htons(0x0806);   // ARP

    // ARP payload
    arp->htype = htons(1);      // Ethernet
    arp->ptype = htons(0x0800); // IPv4
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(1); // ARP request
    memcpy(arp->sha, my_mac, 6);
    arp->tpa = htonl(target_ip);
    // arp->tpa = htonl(ip);
    memset(arp->tha, 0x00, 6); // target MAC unknown
    if (ip_registered)
        arp->spa = htonl(find_1st_ipv4((struct nic_t *)this_nic));
    else
        arp->spa = htonl(0); 

    nic_send_dispatch(nic_id, buf, sizeof(struct eth_hdr) + sizeof(struct arp_pkt));
}
void handle_arp(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    struct eth_hdr *eth = (struct eth_hdr *)buf;
    struct arp_pkt *arp = (struct arp_pkt *)(buf + sizeof(struct eth_hdr));

    uint32_t source_ip = ntohl(arp->spa);
    uint32_t target_ip = ntohl(arp->tpa);

    int oper = ntohs(arp->oper);
    // response;
    if (oper == 2)
    {
        add_cache_mac_v4(source_ip, (uint8_t *)&eth->src, nic_id);
        return;
    }

    // not support other than 1, 2;
    if (oper != 1)
        return;

    // request;
    uint64_t matched_id = find_ipv4_of_nic(nic_id, target_ip);
    if (matched_id == -1)
    {
        add_cache_mac_v4(target_ip, (uint8_t *)&eth->src, nic_id);
        return;
    } 

    struct timeval current_time = get_time_us();
    struct timeval expired_time = add_time_us(nic_global[nic_id]->ip[matched_id].init_time, ARP_WAIT_TIME);
    if (_later(expired_time, current_time))
    {
        nic_global[nic_id]->ip[matched_id].addr_h = 0;
        nic_global[nic_id]->ip[matched_id].addr_l = 0;
        nic_global[nic_id]->ip[matched_id].init_time.tv_sec = 0;
        nic_global[nic_id]->ip[matched_id].init_time.tv_microsec = 0;
        nic_global[nic_id]->ip[matched_id].status = IP_NOT_EXISTS;
        nic_global[nic_id]->ip[matched_id].ver = 0;
    }
    else
    {
        nic_global[nic_id]->ip[matched_id].status = IP_ASSIGNED;
        
        uint8_t reply[64];
        struct eth_hdr *reth = (struct eth_hdr *)reply;
        struct arp_pkt *rarp = (struct arp_pkt *)(reply + sizeof(struct eth_hdr));

        // Ethernet
        memcpy(reth->dst, arp->sha, 6);
        memcpy(reth->src, (uint8_t *)nic_global[nic_id]->mac, 6);
        reth->type = htons(0x0806);

        // ARP payload
        rarp->htype = htons(1);
        rarp->ptype = htons(0x0800);
        rarp->hlen = 6;
        rarp->plen = 4;
        rarp->oper = htons(2); // reply
        memcpy(rarp->sha, (uint8_t *)nic_global[nic_id]->mac, 6);
        rarp->spa = htonl(target_ip);
        memcpy(rarp->tha, arp->sha, 6);
        rarp->tpa = arp->spa;

        nic_send_dispatch(nic_id, reply, sizeof(struct eth_hdr) + sizeof(struct arp_pkt));
    }
    return;
}
void handle_ping_v4(uint64_t nic_id, uint8_t *buf, uint64_t buf_len)
{
    struct icmp_hdr *icmp = (struct icmp_hdr *)(buf + sizeof(struct ipv4_hdr) + sizeof(struct eth_hdr));

    
    if (icmp->type != 8)
        return;

    
    uint8_t reply_buf[2048];
    memcpy(reply_buf, buf, buf_len);

    struct eth_hdr *reth = (struct eth_hdr *)reply_buf;
    struct ipv4_hdr *rip = (struct ipv4_hdr *)(reply_buf + sizeof(struct eth_hdr));
    struct icmp_hdr *ricmp = (struct icmp_hdr *)(reply_buf + sizeof(struct eth_hdr) + sizeof(struct ipv4_hdr));

    // swap MAC
    uint8_t tmp_mac[6];
    memcpy(tmp_mac, reth->dst, 6);
    memcpy(reth->dst, reth->src, 6);
    memcpy(reth->src, tmp_mac, 6);

    // swap IP
    uint32_t tmp_ip = rip->dst;
    rip->dst = rip->src;
    rip->src = tmp_ip;

    // ICMP type = 0 (Echo Reply)
    ricmp->type = 0;
    ricmp->checksum = 0;
    ricmp->checksum = network_checksum((uint8_t *)ricmp, buf_len - sizeof(struct eth_hdr) - sizeof(struct ipv4_hdr));

    
    nic_send_dispatch(nic_id, reply_buf, buf_len);
}
//============================================================
void kernel_thread_network()
{
    while (1)
    {
        kernel_thread_sleep(1000000ULL);
        // int a2 = 1 + 1;
        for (uint64_t i = 0; i < MAX_NIC_COUNT; i++)
        {
            struct nic_t *nic = nic_global[i];
            if (!nic)
                continue;
            struct ip_t *ips = nic->ip;
            for (uint64_t j = 0; j < MAX_IP_PER_NIC; j++)
            {
                struct ip_t *ip = &ips[j];
                struct timeval now = get_time_us();
                struct timeval expired_time = add_time_us((*ip).init_time, ARP_WAIT_TIME);
                if (ip->status == IP_PENDING && ip->addr_l && _later(now, expired_time))
                {
                    send_arp((uint32_t)ip->addr_l, i, true);
                    ip->status = IP_ASSIGNED;
                }
            }
        }

        for (uint64_t i = 0; i < MAX_SOCKETS_COUNT; i++)
        {
            struct net_tuple *tuple = (struct net_tuple *)socket_global[i];
            if (!tuple)
                continue;
            if (tuple->protocol != TCP)
                continue;
            struct net_tuple_tcp *tuple_tcp = (struct net_tuple_tcp *)socket_global[i];
            if (tuple_tcp->tuple.state == NET_STATE_SYN_SENT)
            {
                // if (tuple_tcp->send_retry_count == 0)
                // {
                //     continue;
                // }
                // else
                if (tuple_tcp->send_retry_count > TCP_MAX_RETRIES)
                {
                    mark_socket_as_deleted(i);
                }
                else //(tuple_tcp->retry_count > 0 && tuple_tcp->retry_count <= TCP_MAX_RETRIES)
                {
                    tuple_tcp->send_retry_count++;
                    tcp_send_syn(i);
                }
            }
            else if (tuple_tcp->tuple.state == NET_STATE_ESTABLISHED)
            {
                if (!tuple_tcp->ack_est_blocked)
                {
                    continue;
                }
                else if (tuple_tcp->send_retry_count > TCP_MAX_RETRIES)
                {
                    mark_socket_as_deleted(i);
                }
                else //(tuple_tcp->retry_count > 0 && tuple_tcp->retry_count <= TCP_MAX_RETRIES)
                {
                    tcp_send_established(i);
                    tuple_tcp->send_retry_count++;
                }
            }
            else if (tuple_tcp->tuple.state == NET_STATE_TIME_WAIT)
            {
                tuple_tcp->send_retry_count++;
                if (_later(subtract_time_us(get_time_us(), TCP_TIMEWAIT_TIMEOUT_SECONDS), tuple_tcp->tuple.last_updated_time_us))
                {
                    tuple_tcp->tuple.state = NET_STATE_CLOSED;
                    mark_socket_as_deleted(i); 
                }
            }

            
            // else if (tuple_tcp->tuple.state == NET_STATE_CLOSED)
            // {
            //     delete_socket_i(i);
            // };
        };
    };
}
//=============================================================
/* bool dhcp_init(uint64_t nic_id)
{
    struct nic_t *this_nic = nic_global[nic_id];

    // socket
    struct net_tuple dhcp_sock;
    dhcp_sock.local_ip_h = 0;
    dhcp_sock.local_ip_l = ip_str_to_int("0.0.0.0");
    dhcp_sock.remote_ip_h = 0;
    dhcp_sock.remote_ip_l = ip_str_to_int("255.255.255.255");
    dhcp_sock.protocol = UDP;
    dhcp_sock.local_port = 68;
    dhcp_sock.remote_port = 67;
    dhcp_sock.last_updated_time_ns = get_time_ns();
    uint64_t sock_id = create_socket_udp(0, dhcp_sock);

    // packet;
    uint8_t dhcp_pkt[300];
    erase_mem8(dhcp_pkt, sizeof(dhcp_pkt));

    // op = BOOTREQUEST
    dhcp_pkt[0] = 1; // op
    dhcp_pkt[1] = 1; // htype = Ethernet
    dhcp_pkt[2] = 6; // hlen = MAC length
    dhcp_pkt[3] = 0; // hops

    // transaction ID (4 bytes)
    uint32_t xid = random_xid();
    dhcp_pkt[4] = (xid >> 24) & 0xFF;
    dhcp_pkt[5] = (xid >> 16) & 0xFF;
    dhcp_pkt[6] = (xid >> 8) & 0xFF;
    dhcp_pkt[7] = xid & 0xFF;
    dhcp_sock.xid = xid;

    // client MAC
    memcpy(&dhcp_pkt[28], this_nic->mac, 6);

    // DHCP options
    int opt = 236;
    dhcp_pkt[opt++] = 53;  // DHCP Message Type
    dhcp_pkt[opt++] = 1;   // length
    dhcp_pkt[opt++] = 1;   // DHCPDISCOVER
    dhcp_pkt[opt++] = 255; // end option

    // send
    nic_send_udp_package(sock_id, &dhcp_pkt, opt); // OPTs;
}
void kernel_thread_dhcp_handle()
{
    while (true)
    {
        struct net_tuple *sock_dhcp = 0x0;
        for (int i = 0; i < MAX_SOCKETS_COUNT; i++)
        {
            struct net_tuple *sock = &socket_global[i];

            if (sock->local_port != 68 || sock->remote_port != 67 || sock->local_ip_l != ip_str_to_int("0.0.0.0") || sock->remote_ip_l != ip_str_to_int("255.255.255.255"))
                continue;
            enum net_state_t *state = sock->state;
            if (state != NET_STATE_TIME_WAIT)
                continue;

            uint8_t buf[300];
            uint64_t len = read_socket(sock, buf, sizeof(buf));
            if (len == 0)
                continue;

            // check transaction ID
            uint32_t xid = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
            if (xid != sock->xid)
                continue;

            uint8_t *pkt_mac = &buf[28];
            uint32_t yiaddr = *(uint32_t *)&buf[16]; // IP server offer

            
            uint64_t nic_id = find_nic_id_by_mac(pkt_mac);
            assign_ip_to_nic_if_there_is_no_ip(nic_id, yiaddr);

            sock->state = NET_STATE_ESTABLISHED;
            sock_dhcp = sock;
        }
        kernel_thread_sleep_on_network((uint64_t)&dhcp_lock); 
    }
}
*/
#endif // AF_INET_H
