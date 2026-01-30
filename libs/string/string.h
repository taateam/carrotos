char *strcat(char *dest, const char *src)
{
    char *d = dest;

    
    while (*d)
        d++;

    
    while ((*d++ = *src++))
        ;

    return dest;
}
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) == (unsigned char)(*s2);
}
uint64_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
    {
        p++;
    }
    return (uint64_t)(p - s);
}
uint64_t wstrlen(const uint16_t *s)
{
    const uint16_t *p = s;
    while (*p)
    {
        p++;
    }
    return (uint64_t)(p - s);
}
bool strequal(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}
char *strcpy(char *dest, char *src)
{
    char *d = dest;
    while ((*d++ = *src++))
    {
        ; 
    }
    return dest;
}
char *strconcat(char *dest, const char *s1, const char *s2)
{
    char *d = dest;

    // copy s1
    while (*s1)
    {
        *d++ = *s1++;
    }

    // copy s2
    while (*s2)
    {
        *d++ = *s2++;
    }

    // null terminate
    *d = '\0';
    return dest;
}
int64_t strpos(const char *s, char c)
{
    int index = 0;
    while (s[index])
    {
        if (s[index] == c)
        {
            return index;
        }
        index++;
    }
    return -1; 
}
char *substr(char *dest, const char *src, uint64_t start, uint64_t len)
{
    uint64_t i = 0;

    
    while (i < start && src[i])
    {
        i++;
    }

    uint64_t j = 0;
    while (j < len && src[i])
    {
        dest[j++] = src[i++];
    }

    dest[j] = '\0';
    return dest;
}
void char_to_string(char c, char *out)
{
    out[0] = c;
    out[1] = '\0';
}
char ascii_to_char(uint8_t code)
{
    if (code < 32)
    {
        
        
        switch (code)
        {
        case 0x0A:
            return '\n';
        case 0x09:
            return '\t';
        default:
            return 0; 
        }
    }

    if (code >= 32 && code < 127)
        return (char)code; 

    
    return '?';
}
int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        if (c1 != c2)
            return (int)c1 - (int)c2;

        if (c1 == '\0') 
            return 0;
    }
    return 0;
}
void num_to_str(long long num, char *buf)
{
    char tmp[32];
    int i = 0;
    int is_negative = 0;

    if (num == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    while (num > 0)
    {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (is_negative)
    {
        tmp[i++] = '-';
    }

    
    int j = 0;
    while (i > 0)
    {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}
uint64_t count_str_arr(char **arr)
{
    uint64_t i = 0;
    while (arr[i])
        i++;
    return i;
};
uint64_t total_memsize_str_arr(char **arr)
{
    uint64_t i = 0, total = 0;
    while (arr[i])
    {
        total += strlen(arr[i]) + 1;
        i++;
    }
    return total + 8;
};
void copy_str_arr(char **src, char *data, char **dest)
{
    uint64_t i = 0, pointer = 0;
    while (src[i])
    {
        strcpy(data + pointer, src[i]);
        dest[i] = data + pointer;
        pointer += strlen(src[i]) + 1;
        i++;
    }
    dest[i] = 0;
}
void int_to_str(int value, char *str)
{
    char tmp[16];
    int i = 0, j, sign = 0;

    if (value < 0)
    {
        sign = 1;
        value = -value;
    }

    
    do
    {
        tmp[i++] = (value % 10) + '0';
        value /= 10;
    } while (value > 0);

    if (sign)
    {
        tmp[i++] = '-';
    }

    
    for (j = 0; j < i; j++)
    {
        str[j] = tmp[i - j - 1];
    }
    str[i] = '\0';
}
int64_t str_to_int(char *str){
    long long res = 0;
    int neg = 0;
    if(*str == '-'){
        neg = 1;
        str++;
    }
    while(*str){
        res = res*10 + (*str - '0');
        str++;
    }
    return neg ? -res : res;
};
