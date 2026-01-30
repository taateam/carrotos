// kernel_main.c
#include "libs/lib.h"
#include "core/errorno.h"
#include "core/vga.h"
#include "core/time.h"
#include "core/scheduler.h"
#include "core/driver/driver.h"
#include "core/filesytem/vfs.h"
#include "core/filesytem/filesystem.h"
#include "core/input/input.h"
#include "core/gui/output.h"
#include "core/elf/loader.h"
#include "core/exe/loader.h"
#include "core/ipc/sync.h"
#include "core/ipc/signal.h"
#include "core/ipc/shared_mem.h"
#include "core/network/network.h"
#include "core/user/user.h"
#include "core/syscall/syscall.h"

extern void *tss64_core0;
extern void *rsp0_core0_stack_bottom;
extern void *gdt64;
void kernel_main()
{
    // void *tmp = tss64_core0;
    // register_interrupt_handler(33, tick_interrupt_handler, 0, 0x8E);
    register_interrupt_handler(33, isr_keyboard_wrapper, 0, 0x8E);
    register_interrupt_handler(44, isr_mouse_wrapper, 0, 0x8E);
    register_interrupt_handler(32, tick_interrupt_handler, 0, 0x8E);
    register_interrupt_handler(1, dummy_interrupt_handler1, 0, 0x8E);
    register_interrupt_handler(3, dummy_interrupt_handler1, 0, 0x8E);
    register_interrupt_handler(6, invalid_opcode_handler, 0, 0x8E);
    // register_interrupt_handler(14, seg_fault_handler, 0, 0x8E);
    register_interrupt_handler(0, float_error_handler, 0, 0x8E);
    register_interrupt_handler(64, isr_e1000_wrapper, 0, 0x8E);
    // init_e1000(ip_str_to_int("192.168.70.3"));
    erase_mem64((uint64_t)&rsp0_core0_stack_bottom, 512);
    erase_mem64((uint64_t)&processes_info_ptr, MAX_PROCESSES_COUNT);
    erase_mem64((uint64_t)&processes_cr3_ptr, MAX_PROCESSES_COUNT);
    erase_mem64((uint64_t)&threads_info_ptr, MAX_THREADS_COUNT);
    setup_kernel_process();
    register_a_kernel_thread((uint64_t)kernel_thread_network);
    start_idle_process();
    start_process_seed_from_ram();
    change_cr3((uint64_t)&page_table_l4);
    init_driver_ide();
    init_vfs_fat32();
    start_flag = 1;
    //=================test;
    char *tmp[1];
    tmp[0] = 0;
    uint64_t pid = run_elf_process("/bin/testapp1.elf", tmp, tmp, false);
    // uint64_t pid1 = run_elf_process("/bin/testapp2.elf", tmp, tmp, false);
    // tty_global[1]->foreground_pgid = pid;
    // tty_global[1]->sid = pid;
    // processes_info_ptr[pid]->sid = pid;
    //======================
    // char * x = "";
    // run_exe_process("/bin/winapp.exe", x, x, x);
    //======================
    clear_screen();
    echo(" Carrot OS #: ");

    core0_prepared = 1;
    // echo_ascii(114);
    // echo_ascii(114);
    // echo_ascii(114);
    // echo_ascii(114);
    core_main();
}
