#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>

static uint8_t kb_modifiers_byte = 0;
void pour_keys_and_resume_cli();
void pour_keys_and_resume_raw();
void wait_key_current_thread(struct tty *tty)
{
    // spin_lock(&threads_info_lock1);
    thread_info_t *this_thread = threads_info_ptr[get_current_thread_id()];
    this_thread->sleep_flag_addr = (uint64_t)&tty->locked;
    this_thread->state = THREAD_BLOCKED_KEY;
    scheduler_yeild();
    // spin_unlock(&threads_info_lock1);
}
void wake_thread_by_input_event(uint8_t *lock_addr)
{
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (threads_info_ptr[i] && threads_info_ptr[i]->state == THREAD_BLOCKED_KEY &&
            threads_info_ptr[i]->sleep_flag_addr == (uint64_t)lock_addr)
            threads_info_ptr[i]->state = THREAD_READY;
    }
    // tick_interrupt_handler();
}
// extern void tick_interrupt_handler();
void wake_thread_by_input_event_tty0()
{
    wake_thread_by_input_event(&(tty_global[0]->locked));
}
bool is_modifier_key(uint8_t full_sc)
{
    bool res = full_sc == LSHIFT_SC ||
               full_sc == RSHIFT_SC ||
               full_sc == LCTRL_SC ||
               full_sc == RCTRL_SC ||
               full_sc == LALT_SC ||
               full_sc == RALT_SC ||
               full_sc == LWIN_SC ||
               full_sc == RWIN_SC;
    return res;
}
void pour_keys_and_resume_thread(uint16_t code)
{
    if (is_modifier_key((uint8_t)code))
        return;
    if (code == 0)
        return;
    if (tty_global[0]->mode == TTY_MODE_GUI)
        pour_keys_and_resume_raw();
    else if (tty_global[0]->mode == TTY_MODE_CLI || tty_global[0]->mode == TTY_MODE_PTY)
        pour_keys_and_resume_cli();
}
static bool kb_e0_prefix = false;
void system_reboot(int x);
bool handle_special_keys_combinations(uint16_t full_sc)
{
    int ctrl = kb_modifiers_byte & (MOD_LCTRL | MOD_RCTRL);
    int alt = kb_modifiers_byte & (MOD_LALT | MOD_RALT);

    if (!(ctrl))
        return false;

    if (!alt)
    {
        if (full_sc == C_SC)
        {
            processes_info_ptr[get_current_pid()]->signal = 2;
            return true;
        }
        else if (full_sc == Z_SC)
        {
            processes_info_ptr[get_current_pid()]->signal = 20;
            return true;
        }
        else
            return false;
    }
    else
    {
        switch (full_sc)
        {
        case DEL_SC:
            system_reboot(0);
            return true;
        case F1_SC:
            switch_to_tty(1);
            return true;
        case F2_SC:
            switch_to_tty(2);
            return true;
        case F3_SC:
            switch_to_tty(3);
            return true;
        case F4_SC:
            switch_to_tty(4);
            return true;
        case F5_SC:
            switch_to_tty(5);
            return true;
        case F6_SC:
            switch_to_tty(6);
            return true;
        case F7_SC:
            switch_to_tty(7);
            return true;
        case F8_SC:
            switch_to_tty(8);
            return true;
        case F9_SC:
            switch_to_tty(9);
            return true;
        case F10_SC:
            switch_to_tty(10);
            return true;
        case F11_SC:
            switch_to_tty(11);
            return true;
        case F12_SC:
            switch_to_tty(12);
            return true;
        }
    };
    return false;
}
void isr_keyboard_handler()
{
    // return;
    uint8_t sc = inb(0x60); 
    if (sc == 0xE0)
    {
        kb_e0_prefix = 1;
        return;
    }

    uint16_t full_sc = sc;
    if (kb_e0_prefix)
    {
        full_sc = 0xE000 | sc; 
        kb_e0_prefix = 0;
    };
    int pressed = !(sc & 0x80);
    if (!pressed)
    {
        // int b = 9 + 0 + pressed;
        full_sc -= 0x80;
        sc -= 0x80;
    }
    char c = (sc);
    
    switch (full_sc)
    {
    case LSHIFT_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_LSHIFT) : (kb_modifiers_byte & ~MOD_LSHIFT);
        break;
    case RSHIFT_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_RSHIFT) : (kb_modifiers_byte & ~MOD_RSHIFT);
        break;
    case LCTRL_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_LCTRL) : (kb_modifiers_byte & ~MOD_LCTRL);
        break;
    case RCTRL_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_RCTRL) : (kb_modifiers_byte & ~MOD_RCTRL);
        break;
    case LALT_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_LALT) : (kb_modifiers_byte & ~MOD_LALT);
        break;
    case RALT_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_RALT) : (kb_modifiers_byte & ~MOD_RALT);
        break;
    case LWIN_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_LWIN) : (kb_modifiers_byte & ~MOD_LWIN);
        break;
    case RWIN_SC:
        kb_modifiers_byte = pressed ? (kb_modifiers_byte | MOD_RWIN) : (kb_modifiers_byte & ~MOD_RWIN);
        break;
    }
    if (handle_special_keys_combinations(full_sc))
        return;
    input_buffer[input_buffer_w_index].code = c ? c : full_sc;
    input_buffer[input_buffer_w_index].time.tv_sec = get_epoch_seconds();
    input_buffer[input_buffer_w_index].time.tv_microsec = get_time_ns_in_second();
    input_buffer[input_buffer_w_index].type = 0x01;                              // key press;
    input_buffer[input_buffer_w_index].action = pressed ? IA_PRESS : IA_RELEASE; // press=1, release=2;
    input_buffer_w_index = (input_buffer_w_index + 1) % MAX_INPUT_BUFFER_LEN;
    pour_keys_and_resume_thread(c ? c : full_sc);
};
bool access_tty_allowed(uint64_t tty_id, uint64_t pid)
{
    struct tty *tty = tty_global[tty_id];
    process_info_t *proc = processes_info_ptr[pid];
    
    if (!tty || !proc)
        return false;

    
    if (proc->sid != tty->sid)
        return false;

    
    
    if (tty->foreground_pgid != 0 && proc->pgid != tty->foreground_pgid)
        return false;

    
    return true;
}
uint64_t read_input_to_thread_buffer(struct tty *tty, struct input_key *buffer)
{
    uint64_t count_keys_read = fifo_read16((uint16_t *)tty->input_buf, &tty->in_tail, &tty->in_head, TTY_INPUT_SIZE, 1, (uint16_t *)buffer);
    if (count_keys_read == 0)
        wait_key_current_thread(tty);
    return count_keys_read;
}
uint64_t read_input_line_to_thread_buffer(struct tty *tty, struct input_key *buffer, uint64_t len)
{
    uint64_t i, j = 0;
    // find first enter
    bool found_enter = false;
    for (i = 0; i < TTY_INPUT_SIZE; i++)
    {
        j = (i + tty->in_tail) % TTY_INPUT_SIZE;
        if (j == tty->in_head)
        {
            found_enter = false;
            break;
        }
        if (tty->input_buf[j].key == 0x0A)
        {
            found_enter = true;
            break;
        }
    }
    if (!found_enter)
    {
        return 0;
    }
    uint64_t count_keys_read = i < len ? i : len;
    fifo_read16((uint16_t *)tty->input_buf, &tty->in_tail, &tty->in_head, TTY_INPUT_SIZE, count_keys_read, (uint16_t *)buffer);
    return count_keys_read;
}
uint64_t tty_read_key_halt(struct tty *tty, struct input_key *buffer, uint64_t buffer_size)
{
    if (!access_tty_allowed(tty->id, get_current_pid()))
        return -EPERM;
    uint64_t count = 0;

    if (tty->mode != TTY_MODE_CLI)
        count = read_input_to_thread_buffer(tty, buffer);
    else
    {
        while (true)
        {
            count = read_input_line_to_thread_buffer(tty, buffer, buffer_size);

            if (count == 0)
            {
                wait_key_current_thread(tty);
            }
            else
                break;
        }
    }
    return count;
};
int64_t tty_read_key_no_halt(struct tty *tty, struct input_key *buffer, uint64_t buffer_size)
{
    // return 0;
    if (!access_tty_allowed(tty->id, get_current_pid()))
        return -EPERM;
    uint64_t count = 0;
    if (tty->mode != TTY_MODE_CLI)
        count = read_input_to_thread_buffer(tty, buffer);
    else
        count = read_input_line_to_thread_buffer(tty, buffer, buffer_size);

    if (count == 0)
        return 0;
    return count;
}
void handle_backspace_input(struct tty *tty)
{
    struct input_key tmp_key;

    uint64_t tmp_head;
    
    tmp_head = (tty->in_head == 0) ? (TTY_INPUT_SIZE - 1) : (tty->in_head - 1);
    while (tmp_head != tty->in_tail)
    {
        
        tmp_key = tty->input_buf[tmp_head];
        
        if (is_printable(tmp_key))
        {
            tty->in_head = tmp_head; 
            break;
        }

        
        tmp_head = (tmp_head == 0) ? (TTY_INPUT_SIZE - 1) : (tmp_head - 1);
    }
    
    tmp_head = (tty->out_head == 0) ? (TTY_INPUT_SIZE - 1) : (tty->out_head - 1);
    if (!tty->echo)
        return;
    while (tmp_head != tty->out_tail)
    {
        
        tmp_key = tty->output_buf[tmp_head];
        
        if (is_printable(tmp_key))
        {
            tty->out_head = tmp_head; 
            break;
        }

        
        tmp_head = (tmp_head == 0) ? (TTY_INPUT_SIZE - 1) : (tmp_head - 1);
    }
    delete_last_char_on_screen();
}
uint64_t write_1_event_input_to_tty(struct tty *tty, struct input_key *data)
{
    // --- case backspace ---
    if (data->key == 0x08) // Backspace
    {
        handle_backspace_input(tty);
        return 0;
    }
    fifo_write16((uint16_t *)&tty->input_buf, &tty->in_tail, &tty->in_head, TTY_INPUT_SIZE, 1, (uint16_t *)data);
    if (tty->echo)
    {
        fifo_write16((uint16_t *)&tty->output_buf, &tty->out_tail, &tty->out_head, TTY_INPUT_SIZE, 1, (uint16_t *)data);
        echo_ascii(data->key);
    }
    if (tty->mode == TTY_MODE_CLI && data[1].key == 10)
    {
        // read_input_line_to_thread_buffer(tty, data, len);
        wake_thread_by_input_event(&tty->locked);
    }
    return 0;
    // if tty-> mode !=TTY_MODE_CLI
    // read_input_to_thread_buffer(tty, data);
    // wake_thread_by_input_event(&tty->locked);
}
int64_t tty_read_first_non_modifier_key_from_input_buffer(struct input_key *ev)
{
    struct input_event tmp;
    while (true)
    {
        uint64_t tmp1 = fifo_read_with_size((uint8_t *)input_buffer, &input_buffer_r_index, &input_buffer_w_index, MAX_INPUT_BUFFER_LEN, 1, (uint8_t *)&tmp, sizeof(struct input_event));
        if (!tmp1)
            return 0;
        ev->modifier = kb_modifiers_byte;
        if (is_modifier_key(tmp.code))
            continue;
        ev->key = scancode_to_ascii((uint8_t)tmp.code);
        return tmp.action;
    };
}
char process_shift(char c)
{
    
    if (c >= 'a' && c <= 'z')
        return c - 32;

    
    switch (c)
    {
    case '1':
        return '!';
    case '2':
        return '@';
    case '3':
        return '#';
    case '4':
        return '$';
    case '5':
        return '%';
    case '6':
        return '^';
    case '7':
        return '&';
    case '8':
        return '*';
    case '9':
        return '(';
    case '0':
        return ')';
    case '-':
        return '_';
    case '=':
        return '+';
    case '[':
        return '{';
    case ']':
        return '}';
    case '\\':
        return '|';
    case ';':
        return ':';
    case '\'':
        return '"';
    case ',':
        return '<';
    case '.':
        return '>';
    case '/':
        return '?';
    }

    return c;
}
void pour_keys_and_resume_cli()
{
    struct input_key ev[1];
    // while (true)
    // {
    int64_t tmp = tty_read_first_non_modifier_key_from_input_buffer(ev);
    if (!tmp)
        return;
    if (kb_modifiers_byte == MOD_RSHIFT || kb_modifiers_byte == MOD_LSHIFT)
        ev->key = process_shift(ev->key);
    
    if (tmp != IA_RELEASE)
        return;
    if (ev->key != 0)
    {
        uint8_t modifiers = ev->modifier;
        uint8_t key = ev->key; 
        struct tty *tty0 = tty_global[0];
        if (tty0->mode != TTY_MODE_NONE)
        {
            struct input_key data = {modifiers, key};
            write_1_event_input_to_tty(tty0, &data);
        }
    }
    // }
    // resume_keyboard_blocked_thread();
}
void pour_keys_and_resume_raw()
{
    struct input_key ev[1];
    while (tty_read_key_no_halt(tty_global[0], ev, 1) > 0)
    {
        
        if (ev->key == 0 && !is_modifier_key(ev->modifier))
        {
            uint8_t modifiers = ev->modifier;

            
            char c = scancode_to_ascii(ev->key);
            if (!c)
                continue;

            
            struct tty *tty0 = tty_global[0];
            if (tty0)
            {
                struct input_key data = {modifiers, (uint8_t)c};
                write_1_event_input_to_tty(tty0, &data);
            }
        }
    }
    wake_thread_by_input_event_tty0();
}
#endif // { KEYS_H }
