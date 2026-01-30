int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    // memset();
    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
            return (int)p1[i] - (int)p2[i];
    }
    return 0;
}
void memcpy(uint8_t *dest, uint8_t *src, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++)
    {
        dest[i] = src[i];
    }
}
void memcpy1(uint64_t dest_addr, uint64_t src_addr, uint64_t size)
{
    memcpy((uint8_t *)dest_addr, (uint8_t *)src_addr, size);
}
void memset(void const *s, int c, uint64_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
    {
        *p++ = (unsigned char)c;
    }
    return;
}
uint64_t fifo_read8(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint8_t *out)
{
    uint64_t out_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == *head)
        {
            break;
        }
        out[i] = buf[*tail];
        *tail = ((*tail) + 1) % buf_size;
        out_size++;
    }
    return out_size;
}
uint64_t fifo_write8(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint8_t *in)
{
    uint64_t in_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == (*head + 1) % buf_size)
        {
            break;
        }
        buf[*head] = in[i];
        *head = ((*head) + 1) % buf_size;
        in_size++;
    }
    return in_size;
}
uint64_t fifo_read16(uint16_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint16_t *out)
{
    uint64_t out_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == *head)
        {
            break;
        }
        out[i] = buf[*tail];
        *tail = ((*tail) + 1) % buf_size;
        out_size++;
    }
    return out_size;
}
uint64_t fifo_write16(uint16_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint16_t *in)
{
    uint64_t in_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == (*head + 1) % buf_size)
        {
            break;
        }
        buf[*head] = in[i];
        *head = ((*head) + 1) % buf_size;
        in_size++;
    }
    return in_size;
}

uint64_t fifo_read24(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint8_t *out)
{
    uint64_t out_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == *head)
        {
            break;
        }
        memcpy(&out[i * 3], &buf[(*tail) * 3], 3);
        *tail = ((*tail) + 1) % buf_size;
        out_size++;
    }
    return out_size;
}
uint64_t fifo_write24(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_size, uint64_t len, uint8_t *in)
{
    uint64_t in_size = 0;
    for (uint64_t i = 0; i < len; i++)
    {
        if (*tail == (*head + 1) % buf_size)
        {
            break;
        }
        memcpy(&buf[(*head) * 3], &in[i * 3], 3);
        *head = ((*head) + 1) % buf_size;
        in_size++;
    }
    return in_size;
}
uint64_t fifo_read_with_size(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_units_count, uint64_t copy_units_count, uint8_t *out, uint8_t unit_bytes_size)
{
    uint64_t read_units = 0;
    // uint64_t buf_units_count = buf_bytes_size / unit_bytes_size;
    for (uint64_t i = 0; i < copy_units_count; i++)
    {
        if (*tail == *head)
        {
            break;
        }
        memcpy(out + i * unit_bytes_size, buf + (*tail * unit_bytes_size), unit_bytes_size);
        *tail = ((*tail) + 1) % buf_units_count;
        read_units++;
    }
    return read_units;
}
uint64_t fifo_write_with_size(uint8_t *buf, uint64_t *tail, uint64_t *head, uint64_t buf_units_count, uint64_t copy_units_count, uint8_t *in, uint8_t unit_bytes_size)
{
    uint64_t written_units = 0;
    // uint64_t buf_units_count = buf_bytes_size / unit_bytes_size;

    for (uint64_t i = 0; i < copy_units_count; i++)
    {
        uint64_t next_head = (*head + 1) % buf_units_count;
        if (next_head == *tail) // full?
            break;

        memcpy(buf + (*head * unit_bytes_size),
               in + i * unit_bytes_size,
               unit_bytes_size);

        *head = next_head;
        written_units++;
    }

    return written_units; 
}
void erase_mem8(uint64_t map_addr, uint64_t len)
{
    // erase 1-bytes block * len
    uint8_t *map_int = (uint8_t *)map_addr;
    for (uint64_t i = 0; i < len; i++)
    {
        map_int[i] = 0;
    }
}
void erase_mem64(uint64_t map_addr, uint64_t len)
{
    // erase 8-bytes block * len
    uint64_t *map_int = (uint64_t *)map_addr;
    for (uint64_t i = 0; i < len; i++)
    {
        map_int[i] = 0;
    }
    // panic("No free slot!!!");
}
uint16_t htons(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

uint32_t htonl(uint32_t x)
{
    return ((x & 0x000000FF) << 24) |
           ((x & 0x0000FF00) << 8) |
           ((x & 0x00FF0000) >> 8) |
           ((x & 0xFF000000) >> 24);
}
uint16_t ntohs(uint16_t x)
{
    return htons(x); 
}
uint32_t ntohl(uint32_t x)
{
    return htonl(x);
}
