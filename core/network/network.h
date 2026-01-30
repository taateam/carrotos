#ifndef NETWORKS_H
#define NETWORKS_H
enum ip_status
{
    IP_NOT_EXISTS,
    IP_PENDING,
    IP_ASSIGNED
};
#define MSG_DONTWAIT 0x40
#define NETWORKS_KSIZE 4096
#define MAX_NIC_COUNT NETWORKS_KSIZE / 8
#define MAX_IP_PER_NIC 101
#define NET_BLOCK_SIZE 4096
#define SOCKET_BLOCK_COUNT 5
#define UDP_ADDITIONAL_SOCKET_BLOCK_COUNT 3
#define TCP_ADDITIONAL_SOCKET_BLOCK_COUNT 1
#define Z40KB 40960
#define IP4_SIZE 32
#define ARP_WAIT_TIME 1000000ULL
#define TCP_PROTOCOL_NUMBER 6
#define UDP_PROTOCOL_NUMBER 17
#define ICMP_PROTOCOL_NUMBER 1

#define MAC_CACHE_TIMEOUT 60000ULL

extern struct nic_t *nic_global[MAX_NIC_COUNT];

struct ipv4_hdr
{
    uint8_t version_ihl;       // version (4 bits) + IHL (4 bits)
    uint8_t tos;               // type of service
    uint16_t total_length;     // total length
    uint16_t id;               // identification
    uint16_t flags_fragoffset; // flags + fragment offset
    uint8_t ttl;               // time to live
    uint8_t protocol;          // protocol
    uint16_t checksum;         // header checksum
    uint32_t src;              // source IP
    uint32_t dst;              // destination IP
} __attribute__((packed));
struct ip_t
{
    uint64_t addr_h;
    uint64_t addr_l;
    uint8_t ver;
    uint8_t masklen;
    enum ip_status status;
    struct timeval init_time;
};
// 40 bytes;
struct nic_t
{
    uint16_t type;
    uint8_t mac[6];
    struct ip_t ip[MAX_IP_PER_NIC]; // 102 fit block 4096 bytes;
    uint32_t gateway;
    uint64_t mmio_base;
};
enum protocol
{
    NONE = 0,
    ICMP = 1,
    TCP = 6,
    UDP = 17
};
struct eth_hdr
{
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed));
struct arp_pkt
{
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} __attribute__((packed));
struct icmp_hdr
{
    uint8_t type;      // 8 = Echo Request, 0 = Echo Reply
    uint8_t code;      
    uint16_t checksum; // ICMP checksum
    uint16_t id;       // identifier
    uint16_t seq;      // sequence number
    
} __attribute__((packed));
struct udp_hdr
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
};
struct tcp_hdr
{
    uint16_t src_port;            // source port
    uint16_t dst_port;            // destination port
    uint32_t seq;                 // sequence number
    uint32_t ack_seq;             // acknowledgment number
    uint8_t data_offset_reserved; // data offset (4 bits) + reserved (4 bits)
    uint8_t flags;                // control flags: SYN, ACK, FIN, etc
    uint16_t window;              // window size
    uint16_t checksum;            // checksum
    uint16_t urgent_ptr;          // urgent pointer
    
} __attribute__((packed));
enum net_state_t
{
    NET_STATE_CLOSED = 0,   
    NET_STATE_LISTEN,       
    NET_STATE_SYN_SENT,     
    NET_STATE_SYN_RECEIVED, 
    NET_STATE_ESTABLISHED,  
    NET_STATE_FIN_WAIT_1,   
    NET_STATE_FIN_WAIT_2,   
    NET_STATE_CLOSE_WAIT,   
    NET_STATE_CLOSING,      
    NET_STATE_LAST_ACK,     
    NET_STATE_TIME_WAIT     
};
struct net_tuple
{
    uint64_t local_ip_h;
    uint64_t local_ip_l;
    uint64_t remote_ip_h;
    uint64_t remote_ip_l;
    uint32_t local_port;
    uint32_t remote_port;
    uint16_t protocol;
    uint8_t version;        // ipv4 or ipv6;
    enum net_state_t state; // state;
    struct timeval last_updated_time_us;
    uint32_t xid;
    uint64_t recv_wait_flag;
    uint64_t head;
    uint64_t tail;
    bool receive_blocking;
};
struct net_tuple_tcp
{
    struct net_tuple tuple;
    // enum net_state_t state;
    uint8_t send_retry_count; // tcp state;
    uint16_t send_payload_len;
    bool ack_est_blocked;
    uint64_t seq_local; // tcp seq;
    uint64_t seq_remote;
    uint64_t backlog;
    uint64_t send_wait_flag;
    uint64_t listen_wait_flag;
    uint64_t listen_id;
};
struct mac_cache_v4_entry
{
    uint32_t ip;    // tcp state;
    uint8_t mac[6]; // tcp seq;
    uint64_t nic_id;
    uint64_t timestamp;
} __attribute__((packed));
extern struct mac_cache_v4_entry mac_cache_v4[Z40KB / 168];
struct route_entry
{
    uint32_t dest;    
    uint32_t netmask; // subnet mask
    uint32_t gateway; 
    uint64_t nic_id; 
};
extern struct route_entry route_table_v4[Z40KB / 192];
uint64_t ip_ident = 111111111ULL;
void send_arp(uint32_t ip, uint64_t nic_id, bool ip_registered);
#include "e1000.h"
#include "af_inet/af_inet.h"
#include "af_unix/af_unix.h"
#endif // NETWORKS_H
