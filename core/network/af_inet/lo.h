#ifndef NETWORKS_LO_H
#define NETWORKS_LO_H
void register_lo()
{
    uint64_t new_block_addr = get_a_free_block();
    uint64_t *nic0 = (uint64_t *)&nic_global;
    *nic0 = new_block_addr;
    struct ip_t *local_ip = (struct ip_t *)new_block_addr;
    local_ip->addr_h = 0;
    local_ip->addr_l = ip_str_to_int("127.0.0.1");
    local_ip->init_time = get_time_us();
    local_ip->status = IP_ASSIGNED;
    local_ip->ver = 4;
}
#endif // NETWORKS_H
