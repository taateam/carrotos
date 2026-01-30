#ifndef NETWORKS_E1000_H
#define NETWORKS_E1000_H

#define E1000_CTRL 0x0000   // Device Control
#define E1000_STATUS 0x0008 // Device Status
#define E1000_TCTL 0x0400   // Transmit Control
#define E1000_TIPG 0x0410   // Transmit Inter Packet Gap
#define E1000_TDBAL 0x3800  // TX Descriptor Base Address Low
#define E1000_TDBAH 0x3804  // TX Descriptor Base Address High
#define E1000_TDLEN 0x3808  // TX Descriptor Ring Length
#define E1000_TDH 0x3810    // TX Descriptor Head
#define E1000_TDT 0x3818    // TX Descriptor Tail

#define E1000_RCTL 0x100   // Receive Control
#define E1000_RDBAL 0x2800 // RX Descriptor Base Address Low
#define E1000_RDBAH 0x2804 // RX Descriptor Base Address High
#define E1000_RDLEN 0x2808 // RX Descriptor Ring Length
#define E1000_RDH 0x2810   // RX Descriptor Head
#define E1000_RDT 0x2818   // RX Descriptor Tail

struct e1000_tx_desc
{
    uint64_t addr; 
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

struct e1000_rx_desc
{
    uint64_t addr; 
    uint16_t length;
    uint16_t csum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

#define NUM_TX_DESC 16
struct e1000_tx_desc tx_ring[NUM_TX_DESC] __attribute__((aligned(16)));
uint8_t tx_buffers[NUM_TX_DESC][2048];

void e1000_init_tx(uint64_t mmio_base)
{
    // setup ring
    for (int i = 0; i < NUM_TX_DESC; i++)
    {
        tx_ring[i].addr = (uint64_t)&tx_buffers[i] - HM;
        tx_ring[i].status = 1; // mark free
    }
    *(volatile uint32_t *)(mmio_base + E1000_TDBAL) = (uint32_t)(uint64_t)tx_ring;
    *(volatile uint32_t *)(mmio_base + E1000_TDBAH) = 0;
    *(volatile uint32_t *)(mmio_base + E1000_TDLEN) = sizeof(tx_ring);
    *(volatile uint32_t *)(mmio_base + E1000_TDH) = 0;
    *(volatile uint32_t *)(mmio_base + E1000_TDT) = 0;

    *(volatile uint32_t *)(mmio_base + E1000_TCTL) =
        (1 << 1) | (1 << 3) | (1 << 5) | (1 << 22); // EN | PSP | CT | COLD
    *(volatile uint32_t *)(mmio_base + E1000_TIPG) = 10 | (8 << 10) | (12 << 20);
}

static int tx_tail = 0;
void e1000_send(uint64_t mmio_base, void *data, size_t len)
{
    struct e1000_tx_desc *desc = &tx_ring[tx_tail];

    while (!(desc->status & 1))
        ; 

    memcpy((uint8_t *)&tx_buffers[tx_tail], (uint8_t *)data, len);
    desc->length = len;
    desc->cmd = (1 << 0) | (1 << 3); // EOP | RS
    desc->status = 0;

    tx_tail = (tx_tail + 1) % NUM_TX_DESC;
    *(volatile uint32_t *)(mmio_base + HM + E1000_TDT) = tx_tail;
}

uint16_t network_checksum(uint8_t *buf, size_t len);
uint64_t find_nic_id_by_ipv4(uint32_t src_ip);
bool find_mac_of_ipv4(uint32_t ip, uint64_t nic_id, uint8_t *mac);
bool mac_null(uint8_t mac[6])
{
    for (int i = 0; i < 6; i++)
        if (mac[i] != 0)
            return false;
    return true;
}
int64_t nic_send_ipv4(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, void *l4_hdr, size_t l4_len, uint8_t *payload, size_t payload_len)
{
    uint8_t buf[1514]; // MTU
    size_t offset = 0;

    uint8_t remote_mac[6];
    uint8_t local_mac[6];
    uint64_t src_nic_id = find_nic_id_by_ipv4(src_ip);
    memcpy(local_mac, nic_global[src_nic_id]->mac, 6);
    find_mac_of_ipv4(dst_ip, src_nic_id, remote_mac);
    if (mac_null(remote_mac))
        return -EAGAIN;
    // 1. Ethernet
    struct eth_hdr *eth = (struct eth_hdr *)buf;
    memcpy(eth->dst, remote_mac, 6); 
    memcpy(eth->src, local_mac, 6);
    eth->type = htons(0x0800); // IPv4
    offset += sizeof(*eth);

    // 2. IPv4
    struct ipv4_hdr *ip = (struct ipv4_hdr *)(buf + offset);
    ip->version_ihl = (4 << 4) | 5;
    ip->tos = 0;
    ip->total_length = htons(sizeof(*ip) + l4_len + payload_len);
    ip->id = htons(ip_ident++); // global counter
    ip->flags_fragoffset = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->src = htonl(src_ip);
    ip->dst = htonl(dst_ip);
    ip->checksum = 0;
    ip->checksum = network_checksum((uint8_t *)ip, sizeof(*ip));
    offset += sizeof(*ip);

    // 3. TCP/UDP header
    memcpy(buf + offset, (uint8_t *)l4_hdr, l4_len);
    offset += l4_len;

    // 4. Payload
    if (payload_len > 0)
    {
        memcpy(buf + offset, (uint8_t *)payload, payload_len);
        offset += payload_len;
    }

    // 5. Send to NIC
    e1000_send(nic_global[src_nic_id]->mmio_base, buf, offset);
    return 0;
}
#define NUM_RX_DESC 16
struct e1000_rx_desc rx_ring[NUM_RX_DESC] __attribute__((aligned(4096)));
uint8_t rx_buffers[NUM_RX_DESC][2048] __attribute__((aligned(4096)));

void e1000_init_rx(uint64_t mmio_base)
{
    // mmio_base += HM;
    for (int i = 0; i < NUM_RX_DESC; i++)
    {
        rx_ring[i].addr = (uint64_t)&rx_buffers[i] - HM;
        rx_ring[i].status = 0;
    }
    uint32_t debug = (uint32_t)((uint64_t)&rx_ring - HM);
    *(uint32_t *)(mmio_base + E1000_RDBAL) = (uint32_t)((uint64_t)&rx_ring - HM);
    *(uint32_t *)(mmio_base + E1000_RDBAH) = 0;
    *(uint32_t *)(mmio_base + E1000_RDLEN) = sizeof(rx_ring);
    *(uint32_t *)(mmio_base + E1000_RDH) = 0;
    *(uint32_t *)(mmio_base + E1000_RDT) = NUM_RX_DESC - 1;

    *(volatile uint32_t *)(mmio_base + E1000_RCTL) = (1 << 1) | (1 << 3) | (1 << 4) | (1 << 15) | (1 << 26); // EN | BAM | SECRC
}
static int rx_tail = 0;
int e1000_recv(uint64_t mmio_base, uint8_t *buf, size_t bufsize)
{
    uint32_t cause = *(uint32_t *)(mmio_base + HM + 0xc0);
    echoi(cause);
    echo("\n");
    struct e1000_rx_desc *desc; // = &rx_ring[rx_tail];
    uint64_t rx_id, tmp = 0;
    bool found_rx = false;
    for (; tmp < NUM_RX_DESC; tmp++)
    {
        rx_id = (rx_tail + tmp) % NUM_RX_DESC;
        desc = &rx_ring[rx_id];
        if ((desc->status & 1))
        {
            found_rx = true;
            break;
        }
    }
    if (!found_rx)
        return 0; // no packet
    size_t len = desc->length;
    if (len > bufsize)
        len = bufsize;
    memcpy(buf, (uint8_t *)&rx_buffers[rx_id], len);
    desc->status = 0;
    *(volatile uint32_t *)(mmio_base + HM + E1000_RDT) = rx_id;
    rx_tail = (rx_id + 1) % NUM_RX_DESC;
    return len;
}
bool find_first_e1000(uint8_t *found_bus, uint8_t *found_dev, uint16_t *found_vendor, uint16_t *found_device)
{
    for (uint8_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t dev = 0; dev < 32; dev++)
        {
            for (uint8_t func = 0; func < 8; func++)
            {
                uint16_t vendor = pci_read_vendor(bus, dev, func);
                if (vendor == 0xFFFF)
                    continue; 

                uint16_t device_id = pci_read16(bus, dev, func, 0x02);
                uint8_t class_code = pci_read8(bus, dev, func, 0x0B);
                uint8_t subclass = pci_read8(bus, dev, func, 0x0A);

                if (vendor == 0x8086 && class_code == 0x02 && subclass == 0x00)
                {
                    *found_bus = bus;
                    *found_dev = dev;
                    *found_vendor = vendor;
                    *found_device = device_id;
                    return true; 
                }
            }
        }
    }
    return false; 
}
bool e1000_msi_enabled(uint8_t bus, uint8_t dev)
{
    uint8_t func = 0; 

    
    uint16_t status = pci_read32(bus, dev, func, 0x04) >> 16; // offset 0x06
    if (!(status & (1 << 4)))                                 // bit 4 = Capabilities List
        return false;

    
    uint8_t cap_ptr = pci_read8(bus, dev, func, 0x34);
    while (cap_ptr != 0)
    {
        uint8_t cap_id = pci_read8(bus, dev, func, cap_ptr);
        if (cap_id == 0x05)
        { // MSI capability
            uint16_t msi_ctrl = pci_read32(bus, dev, func, cap_ptr + 2) & 0xFFFF;
            return (msi_ctrl & 0x1) != 0; // bit0 = MSI enable
        }
        cap_ptr = pci_read8(bus, dev, func, cap_ptr + 1); // next pointer
    }

    return false; 
}
void e1000_enable_msi(uint8_t bus, uint8_t dev, uint8_t func)
{
    
    uint8_t cap_ptr = pci_read8(bus, dev, func, 0x34);
    while (cap_ptr)
    {
        uint8_t cap_id = pci_read8(bus, dev, func, cap_ptr);
        if (cap_id == 0x05)
        { // MSI
            uint16_t msg_ctrl = pci_read16(bus, dev, func, cap_ptr + 2);

            
            pci_write32(bus, dev, func, cap_ptr + 4, 0xFEE00000);
            

            // Write data = vector 0x40
            pci_write16(bus, dev, func, cap_ptr + 8, 0x40);

            // enable MSI
            msg_ctrl |= 0x0001; // MSI Enable
            pci_write16(bus, dev, func, cap_ptr + 2, msg_ctrl);
            break;
        }
        cap_ptr = pci_read8(bus, dev, func, cap_ptr + 1); // next
    }
}
uint64_t get_mmio_base(uint8_t bus, uint8_t dev, uint8_t func)
{
    uint32_t bar0 = pci_read32(bus, dev, func, 0x10);
    
    return (uint64_t)(bar0 & ~0xF);
}
#define E1000_RAL0 0x5400
#define E1000_RAH0 0x5404
void get_mac(uint8_t bus, uint8_t dev, uint8_t *mac)
{
    uint64_t mmio = get_mmio_base(bus, dev, 0);

    uint32_t ral = *(volatile uint32_t *)(mmio + E1000_RAL0);
    uint32_t rah = *(volatile uint32_t *)(mmio + E1000_RAH0);

    mac[0] = (ral >> 0) & 0xFF;
    mac[1] = (ral >> 8) & 0xFF;
    mac[2] = (ral >> 16) & 0xFF;
    mac[3] = (ral >> 24) & 0xFF;
    mac[4] = (rah >> 0) & 0xFF;
    mac[5] = (rah >> 8) & 0xFF;
}
uint64_t register_nic(uint8_t mac_i[6]);
#define E1000_ICR 0xC0
#define E1000_IMS 0xD0
#define E1000_IMC 0xD8
void init_e1000(uint32_t ipv4, uint8_t masklen, uint32_t gateway)
{
    uint8_t bus, dev, fn = 0;
    uint16_t vendor, device;
    find_first_e1000(&bus, &dev, &vendor, &device);

    uint16_t command = pci_read16(bus, dev, 0, 0x04);
    command = (1 << 2) | (1 << 1) | (1 << 0);
    // command |= (1 << 10); // enable INTx
    pci_write16(bus, dev, 0, 0x04, command);

    uint32_t bar0 = pci_read32(bus, dev, 0, 0x10) & ~0xF;
    volatile uint32_t *e1000_regs = (volatile uint32_t *)(uint64_t)bar0;

    // reset
    e1000_regs[0] |= (1 << 26);
    while (e1000_regs[0] & (1 << 26))
        ;

    e1000_regs[0] &= ~(1 << 1);
    e1000_regs[0] &= ~(1 << 18);
    e1000_regs[0] |= (1 << 6);
    uint32_t cmd = pci_read16(bus, dev, fn, 0x04);
    uint32_t stt = pci_read16(bus, dev, fn, 0x06);
    uint8_t num = pci_read8(bus, dev, fn, 0x3C);
    uint8_t cap = pci_read8(bus, dev, fn, 0x34);

    while (cap)
    {
        uint8_t id = pci_read8(bus, dev, fn, cap + 0);
        uint8_t next = pci_read8(bus, dev, fn, cap + 1);

        if (id == 0x05)
        {
            break;
        }

        if (id == 0x11)
        {
            break;
        }

        cap = next;
    }
    if (!cap)
        panic("No MSI/MSI-X capability");

    // Message Address
    pci_write32(bus, dev, fn, cap + 4, 0xFEE00000);
    pci_write32(bus, dev, fn, cap + 8, 0x00000000);

    // Message Data
    pci_write16(bus, dev, fn, cap + 0x0C, 0x40);

    // Enable MSI
    uint16_t ctrl = pci_read16(bus, dev, fn, cap + 2);
    ctrl |= 1;
    pci_write16(bus, dev, fn, cap + 2, ctrl);

    // e1000_regs[0xD0 / 4] = 0x0000009C; // | (1 << 2) | (1 << 6) | (1 << 7);
    // e1000_regs[0] |= (1 << 30);

    e1000_init_tx((uint64_t)e1000_regs);
    e1000_init_rx((uint64_t)e1000_regs);
    //========================
    e1000_regs[0xc4 / 4] = 0;
    e1000_regs[0xe0 / 4] = 0;
    e1000_regs[0x2820 / 4] = 0;
    e1000_regs[0x282c / 4] = 0;
    e1000_regs[0xc4 / 4] = 0;
    //=========================
    // enable interrupts
    e1000_regs[E1000_IMS / 4] = 0x1F6DD; // IMS
    (void)e1000_regs[E1000_ICR / 4];     // clear pending

    uint8_t mac[6];
    get_mac(bus, dev, mac);

    uint64_t nic_id = register_nic(mac);
    nic_global[nic_id]->type = 1;
    nic_global[nic_id]->mmio_base = (uint64_t)e1000_regs;
    nic_global[nic_id]->ip[0].addr_l = ipv4;
    nic_global[nic_id]->ip[0].addr_h = 0;
    nic_global[nic_id]->ip[0].masklen = masklen;
    nic_global[nic_id]->gateway = gateway;
    nic_global[nic_id]->ip[0].ver = 4;
    nic_global[nic_id]->ip[0].status = IP_PENDING;
    nic_global[nic_id]->ip[0].init_time = get_time_us();

    send_arp(ipv4, nic_id, false);
    // nic_global[nic_id]->ip[0].status = IP_PENDING;
}

#endif // NETWORKS_E1000_H
