#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#define MAX_SM 512
#define SM_BLOCK_SIZE 4096
struct shared_mem
{
    uint64_t id;
    // uint64_t phys_addr;
    uint64_t virt_addr;
    uint64_t size;
    uint64_t ref_count;
    uint64_t owner_pid;
    uint64_t flags;
} __attribute__((packed));
extern struct shared_mem *sm_global[MAX_SM];
uint64_t create_sm(uint64_t virt_addr, uint64_t size, uint64_t flags)
{
    uint64_t pid = get_current_pid();
    if (is_paged(pid, virt_addr))
        return -EINVAL;
    uint64_t blocks_count = div_ceil(size, SM_BLOCK_SIZE);
    uint64_t id = get_a_free_slot((uint64_t)&sm_global, 0, MAX_SM);
    // uint64_t sm_starting_phys_addr = get_continuous_free_blocks(blocks_count);
    struct shared_mem *newsm = (struct shared_mem *)get_a_free_block_addr();
    newsm->id = id;
    // newsm->phys_addr = sm_starting_phys_addr;
    newsm->virt_addr = virt_addr;
    newsm->size = size;
    newsm->ref_count = 1;
    newsm->owner_pid = pid;
    newsm->flags = flags;
    sm_global[id] = newsm;
    for (uint64_t i = 0; i < blocks_count; i++)
    {
        increase_paging_of_process_with_virt_addr(pid, virt_addr + i * SM_BLOCK_SIZE);
    }
    // add to process sm list;
    process_info_t *proc = processes_info_ptr[pid];
    uint64_t sm_slot_id = get_a_free_slot((uint64_t)&proc->sm, 0, PROC_MAX_SM);
    proc->sm[sm_slot_id] = newsm;
    return id;
}
uint64_t fork_sm(uint64_t sm_id, uint64_t target_pid)
{
    uint64_t curr_pid = get_current_pid();
    struct shared_mem *sm = sm_global[sm_id];
    sm->ref_count++;
    return sm->ref_count;
}
uint64_t remove_sm(uint64_t sm_id)
{
    uint64_t pid = get_current_pid();
    struct shared_mem *sm = sm_global[sm_id];
    sm->ref_count--;
    uint64_t blocks_count = div_ceil(sm->size, SM_BLOCK_SIZE);
    for (uint64_t i = 0; i < blocks_count; i++)
    {
        decrease_paging_of_process_with_virt_addr(pid, sm->virt_addr + i * SM_BLOCK_SIZE); // also mark blocks at not used;
    }
    // delete process sm;
    struct shared_mem **proc_sm = processes_info_ptr[pid]->sm;
    for (uint64_t i = 0; i < PROC_MAX_SM; i++)
    {
        if (proc_sm[i] == sm)
            proc_sm[i] = 0;
    }
    // delete if no ref_count
    if (sm->ref_count == 0)
    {
        set_memory_block_used_when_boot((uint64_t)sm, false); // free struct shared_mem
        sm_global[sm_id] = NULL;                    // remove from global table
    }
    return 0;
}
#endif // SHARED_MEM_H
