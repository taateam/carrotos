#ifndef EXE_SEH_H
#define EXE_SEH_H

struct pdata_entry
{
    uint64_t rip_start;
    uint64_t rip_end;
    uint64_t pdata_start;
};
static struct pdata_entry pdata_table[256];
int64_t add_pdata_entry_when_load_image(struct data_directory *exception_directory, uint64_t image_base_after_relocation, uint64_t image_size)
{
    int64_t free_slot = get_a_free_slot_with_size((uint64_t)pdata_table, 0, sizeof(pdata_table), sizeof(struct pdata_entry));
    if (free_slot == -1)
    {
        return -1;
    }
    pdata_table[free_slot].rip_start = image_base_after_relocation;
    pdata_table[free_slot].rip_end = image_base_after_relocation + image_size;
    pdata_table[free_slot].pdata_start = exception_directory->virtual_address + image_base_after_relocation;
    return 0;
};
struct runtime_function
{
    uint32_t BeginAddress; // RVA: start function
    uint32_t EndAddress;   // RVA: end function (exclusive)
    uint32_t UnwindData;   // RVA -> UNWIND_INFO (.xdata)
} __attribute__((packed));
typedef struct unwind_code
{
    uint8_t CodeOffset; // offset trong prologue
    uint8_t UnwindOp : 4;
    uint8_t OpInfo : 4;
};
struct unwind_info
{
    uint8_t Version : 3;             // = 1
    uint8_t Flags : 5;               // UNW_FLAG_*
    uint8_t SizeOfProlog;            // bytes
    uint8_t CountOfCodes;            
    uint8_t FrameRegister : 4;       // RBP/RSP/... (0 = none)
    uint8_t FrameOffset : 4;         // *16
    struct unwind_code UnwindCode[]; // array, reverse order
    // padding to 4 bytes
    // optional:
    // uint32_t ExceptionHandlerRVA or ChainedFunctionRVA
};
struct pdata_entry *find_pdata(uint64_t rip)
{
    for (int64_t i = 0; i < sizeof(pdata_table) / sizeof(struct pdata_entry); i++)
    {
        if (rip >= pdata_table[i].rip_start && rip < pdata_table[i].rip_end)
        {
            return &pdata_table[i];
        }
    }
    return NULL;
};
struct trap_frame
{
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx;
    uint64_t rdx, rcx, rax;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
void KiDispatchException(uint64_t trap_frame_addr)
{
    struct trap_frame *tf = (struct trap_frame *)trap_frame_addr;
    uint64_t rip = tf->rip;
    struct pdata_entry *pdata_addr = find_pdata(rip);
    if (pdata_addr == NULL)
    {
        return;
    }
    do_rewind(tf, (struct runtime_function *)pdata_addr->pdata_start, pdata_addr->rip_start);
}
struct rewind_state
{
    uint64_t rip, rsp;
    uint64_t rbx, rdi, rsi, r12, r13, r14, r15;
};
void do_rewind_once_step(struct unwind_code *code, struct rewind_state *state, struct trap_frame *tf, uint64_t image_base_addr)
{
    switch (code->UnwindOp)
    {
    case 0: // UWOP_PUSH_NONVOL
    {
        uint64_t *src_addr = (uint64_t *)(state->rsp);
        uint64_t val = *src_addr;
        switch (code->OpInfo)
        {
        case 3:
            state->rbx = val;
            break;
        case 5:
            state->rsi = val;
            break;
        case 6:
            state->rdi = val;
            break;
        case 12:
            state->r12 = val;
            break;
        case 13:
            state->r13 = val;
            break;
        case 14:
            state->r14 = val;
            break;
        case 15:
            state->r15 = val;
            break;
        default:
            break;
        }
        state->rsp += 8;
        break;
    }
    case 1:
    { // UWOP_ALLOC_LARGE
        uint32_t alloc_size = (*(uint16_t *)(code + 1)) * 8;
        state->rsp += alloc_size;
        break;
    }
    case 2:
    { // UWOP_ALLOC_SMALL
        uint32_t alloc_size = (code->OpInfo * 8) + 8;
        state->rsp += alloc_size;
        break;
    }
    case 3: // UWOP_SET_FPREG
    {
        uint64_t size = 0;
        if (code->OpInfo == 0)
            size = *(uint16_t *)(code + 1) * 8;
        else
            size = *(uint32_t *)(code + 1);
        state->rsp += size;
        break;
    }
    case 4: // UWOP_SAVE_NONVOL
    {
        uint64_t *src_addr = (uint64_t *)(state->rsp);
        uint64_t val = *src_addr;
        switch (code->OpInfo)
        {
        case 3:
            state->rbx = val;
            break;
        case 5:
            state->rsi = val;
            break;
        case 6:
            state->rdi = val;
            break;
        case 12:
            state->r12 = val;
            break;
        case 13:
            state->r13 = val;
            break;
        case 14:
            state->r14 = val;
            break;
        case 15:
            state->r15 = val;
            break;
        default:
            break;
        }
        state->rsp += 8;
        break;
    }
    case 5: // UWOP_SAVE_NONVOL_FAR
    {
        uint32_t offset = *(uint32_t *)(code + 1);
        uint64_t *src_addr = (uint64_t *)(state->rsp);
        uint64_t val = *src_addr;
        switch (code->OpInfo)
        {
        case 3:
            state->rbx = val;
            break;
        case 5:
            state->rsi = val;
            break;
        case 6:
            state->rdi = val;
            break;
        case 12:
            state->r12 = val;
            break;
        case 13:
            state->r13 = val;
            break;
        case 14:
            state->r14 = val;
            break;
        case 15:
            state->r15 = val;
            break;
        default:
            break;
        }
        state->rip = *(uint64_t *)state->rsp;
        state->rsp += 8;
        break;
    }
    case 8: // UWOP_SAVE_XMM128
    {
        state->rsp += 16;
        break;
    }
    case 9: // UWOP_SAVE_XMM128_FAR
    {
        state->rsp += 16;
        break;
    }
    case 10: // UWOP_PUSH_MACHFRAME
    {
        if (code->OpInfo & 0x1)
        {
            state->rsp += 8 * 9;
        }
        else
        {
            state->rsp += 8 * 7;
        }
        break;
    }
    default:
        break;
    }
}
__attribute__((naked, noreturn)) 
void jump_with_state(uint64_t rsp, uint64_t rip)
{
    __asm__ volatile(
        "mov %rdi, %rsp\n"
        "jmp *%rsi\n");
}
void do_rewind(struct trap_frame *tf, struct runtime_function *run_func, uint64_t image_base_addr)
{
    struct unwind_info *unwind_info_ptr = (struct unwind_info *)(run_func->UnwindData + image_base_addr);
    uint32_t rip_rva = tf->rip - image_base_addr - run_func->BeginAddress;
    if (unwind_info_ptr->Version != 1)
        return;
    struct rewind_state state = {0};
    state.rsp = tf->rsp;
    state.rip = tf->rip;
    state.rbx = tf->rbx;
    state.rdi = tf->rdi;
    state.rsi = tf->rsi;
    state.r12 = tf->r12;
    state.r13 = tf->r13;
    state.r14 = tf->r14;
    state.r15 = tf->r15;
restart_unwind:
    if (unwind_info_ptr->Flags & 0x4)
    {
        struct runtime_function *chained =
            (struct runtime_function *)(&unwind_info_ptr->UnwindCode[unwind_info_ptr->CountOfCodes]);

        run_func = chained;
        goto restart_unwind;
    }
    for (int64_t i = unwind_info_ptr->CountOfCodes - 1; i >= 0; i--)
    {
        struct unwind_code *code = &unwind_info_ptr->UnwindCode[i];
        if (code->CodeOffset > rip_rva)
            continue;
        do_rewind_once_step(code, &state, tf, image_base_addr);
    }
    // restore register except rip;
    asm volatile(
        "mov %0, %%rbx\n"
        "mov %1, %%rdi\n"
        "mov %2, %%rsi\n"
        "mov %3, %%r12\n"
        "mov %4, %%r13\n"
        "mov %5, %%r14\n"
        "mov %6, %%r15\n"
        :
        : "r"(state.rbx), "r"(state.rdi), "r"(state.rsi), "r"(state.r12), "r"(state.r13), "r"(state.r14), "r"(state.r15)
        : "%rbx", "%rdi", "%rsi", "%r12", "%r13", "%r14", "%r15", "memory");
    // jump to rip;
    jump_with_state(state.rsp, state.rip);
};
#endif // { EXE_SEH_H }
