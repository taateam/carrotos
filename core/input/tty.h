#ifndef TTY_H
#define TTY_H

#include <stdint.h>
enum tty_mode
{
    TTY_MODE_NONE = 0,
    TTY_MODE_PTY = 1,
    TTY_MODE_CLI = 2,
    TTY_MODE_GUI = 3
} __attribute__((mode(DI)));
struct tty
{
    uint64_t id;              // 0x00
    uint64_t foreground_pgid; // 0x1C–0x23
    uint64_t sid;
    uint64_t in_head;                             // 0x08
    uint64_t in_tail;                             // 0x0C
    uint64_t out_head;                            // 0x10
    uint64_t out_tail;                            // 0x14
    uint8_t is_active;                            // 0x18
    uint8_t echo;                                 // 0x19
    uint8_t locked;                               // 0x1A
    enum tty_mode mode;                           // 0x1B
    struct input_key input_buf[TTY_INPUT_SIZE];   // 0x24–0x24+2030
    struct input_key output_buf[TTY_OUTPUT_SIZE]; // 0x820..0xFFF
};
extern struct tty *tty_global[MAX_TTY_COUNT];
void init_tty_with_id(uint64_t id)
{
    uint64_t new_tty_addr = get_a_free_block_addr();
    struct tty *this_tty = (struct tty *)new_tty_addr;
    this_tty->id = id;
    this_tty->in_head = 0;
    this_tty->in_tail = 0;
    this_tty->out_head = 0;
    this_tty->out_tail = 0;
    this_tty->is_active = 0;
    this_tty->locked = 0;
    this_tty->echo = 1;
    this_tty->sid = 2;
    if (id < 13)
        this_tty->mode = TTY_MODE_CLI;
    else
        this_tty->mode = TTY_MODE_PTY;
    erase_mem8((uint64_t)&this_tty->input_buf, TTY_INPUT_SIZE);
    erase_mem8((uint64_t)&this_tty->output_buf, TTY_OUTPUT_SIZE);
    tty_global[id] = (struct tty *)new_tty_addr;
}
bool is_printable(struct input_key key)
{
    
    if (key.key < 0x20 || key.key > 0x7E)
        return false;

    
    uint8_t mod = key.modifier & ~(MOD_LSHIFT | MOD_RSHIFT);
    if (mod != 0)
        return false;

    return true;
}
uint64_t init_tty()
{
    uint64_t id = get_a_free_slot((uint64_t)&tty_global, 0, MAX_TTY_COUNT);
    init_tty_with_id(id);
    return id;
};
uint64_t assign_tty(uint64_t pid, uint64_t tty_id)
{
    process_info_t *current_proc = processes_info_ptr[get_current_pid()];
    process_info_t *proc = processes_info_ptr[pid];
    struct tty *tty = tty_global[tty_id];
    if (!tty)
        return -ENODEV;
    if (!proc)
        return -ESRCH;
    if (current_proc->pgid != tty->foreground_pgid || proc->sid != tty->sid)
        return -EPERM;
    tty->foreground_pgid = proc->pgid;
    return 0;
}
void echo_ascii(uint8_t n)
{
    char tmp[2];
    char c = ascii_to_char(n);
    char_to_string(c, tmp);
    echo(tmp);
};
uint64_t echo_tty(struct input_key *buf, uint64_t tail, uint64_t head)
{
    if (!buf)
        return 0;

    uint64_t count = 0;
    uint64_t i = tail;

    
    while (i != head)
    {
        struct input_key k = buf[i];
        if (is_printable(k))
        {
            if (k.key >= 'a' && k.key <= 'z')
                echo_ascii(k.key - 32); 
            else
                echo_ascii(k.key); 
        }
        else
        {
            echo_ascii(k.key);
        }
        count++;

        i++;
        
        if (i >= TTY_OUTPUT_SIZE)
            i = 0;
    }

    return count; 
}
bool redraw_tty()
{
    struct tty *tty = tty_global[0]; 
    if (!tty)
        return false;

    clear_screen();
    echo_tty((struct input_key *)&tty->output_buf, tty->out_tail, tty->out_head);
    return true;
}
bool switch_to_tty(uint64_t tty_id)
{
    if (tty_id < 1 || tty_id > 12)
        return false;
    for (uint64_t i = 1; i < 13; i++)
    {
        tty_global[i]->is_active = 0;
    }
    if (!tty_global[tty_id])
        return false;
    tty_global[tty_id]->is_active = 1;
    tty_global[0] = tty_global[tty_id];
    redraw_tty();
    return true;
}
void prepare_tty_when_boot()
{
    erase_mem8((uint64_t)&tty_global, sizeof(tty_global));
    for (uint64_t i = 1; i < 13; i++)
    {
        init_tty_with_id(i);
    }
    switch_to_tty(1);
    enable_cursor();
}
void delete_tty(uint64_t id)
{
    if (id >= MAX_TTY_COUNT || id < 13)
        return; // invalid ID
    struct tty *this_tty = tty_global[id];
    if (!this_tty)
        return; // already freed or never existed
    set_memory_block_used_when_boot((uint64_t)tty_global[id], false);
    tty_global[id] = (struct tty *)0;
}
#define PIPE_BUF_SIZE 4070
#define MAX_PIPES_COUNT 5120
struct pipe
{
    uint64_t head;
    uint64_t tail;
    uint8_t closed_read;
    uint8_t closed_write;
    uint8_t buf[PIPE_BUF_SIZE];
};
extern struct pipe *pipe_global[MAX_PIPES_COUNT];
bool access_tty_allowed(uint64_t tty_id, uint64_t pid);
void wake_thread_by_input_event(uint8_t *lock_addr);
uint64_t tty_write_output(struct tty *tty, struct input_key *data, uint64_t len)
{
    uint64_t pid = get_current_pid();
    if (!access_tty_allowed(tty->id, pid))
        return -EIO; 

    // Write ra buffer output
    fifo_write16((uint16_t *)&tty->output_buf, &tty->out_tail, &tty->out_head, TTY_OUTPUT_SIZE, len, (uint16_t *)data);

    
    if (tty->mode == TTY_MODE_PTY)
    {
        wake_thread_by_input_event(&tty->locked); 
        return 0;
    }

    
    if (tty->mode == TTY_MODE_CLI)
    {
        uint64_t i;
        for (i = 0; i < len; i++)
        {
            if (!data[i].key)
                break;
            char tmp[2];
            char_to_string(data[i].key, tmp);
            echo(tmp); 
        }
        return i + 1;
    }

    
    if (tty->mode == TTY_MODE_GUI)
    {
        // tty_send_gui_event(tty, data, len);
        return 0;
    }
}
#endif // { TTY_H }
