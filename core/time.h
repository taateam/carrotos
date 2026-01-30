#ifndef TIME_H
#define TIME_H
struct timeval
{
    uint64_t tv_sec;      /* seconds from Epoch (1970-01-01 00:00:00 UTC) */
    uint32_t tv_microsec; /* microseconds (0..999999) */
    uint32_t pad;
};
struct timespec
{
    uint64_t tv_sec;     // seconds
    uint64_t tv_nanosec; // nanoseconds
};
static uint8_t cmos_read(uint8_t reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

int bcd_to_bin(int val)
{
    return (val & 0x0F) + ((val >> 4) * 10);
}
int bin_to_bcd(int val)
{
    int tens = val / 10;
    int units = val % 10;
    return (tens << 4) | units;
}
static bool rtc_update_in_progress()
{
    outb(0x70, 0x0A);
    return inb(0x71) & 0x80;
}

static int is_leap(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static const int days_in_month[12] =
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static uint64_t days_since_epoch(int year, int month, int day)
{
    
    uint64_t days = 0;

    for (int y = 1970; y < year; y++)
        days += is_leap(y) ? 366 : 365;

    for (int m = 1; m < month; m++)
    {
        days += days_in_month[m - 1];
        if (m == 2 && is_leap(year))
            days += 1;
    }

    days += (day - 1);
    return days;
}

struct timeval timespec_to_timeval(struct timespec ts)
{
    struct timeval tv;
    tv.tv_sec = ts.tv_sec;
    tv.tv_microsec = ts.tv_nanosec / 1000ULL;
    return tv;
}
uint64_t get_epoch_seconds()
{
    
    while (rtc_update_in_progress())
        ;

    int sec = (cmos_read(0x00));
    int min = (cmos_read(0x02));
    int hour = (cmos_read(0x04));
    int day = (cmos_read(0x07));
    int mon = (cmos_read(0x08));
    int year = (cmos_read(0x09));
    int cent = 20; 

    
    outb(0x70, 0x0B);
    uint8_t regB = inb(0x71);

    if (!(regB & 0x04))
    { // BCD mode
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        year = bcd_to_bin(year);
    }

    
    uint8_t cent_reg = cmos_read(0x32);
    if (cent_reg != 0)
    {
        cent = bcd_to_bin(cent_reg);
    }

    int full_year = cent * 100 + year;

    // 4. Days since 1970
    uint64_t days = days_since_epoch(full_year, mon, day);

    // 5. Seconds
    uint64_t seconds = days * 86400ULL +
                       hour * 3600ULL +
                       min * 60ULL +
                       sec;

    return seconds;
}
// --- CPUID ---
static inline void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ __volatile__("cpuid"
                         : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                         : "a"(leaf));
}

// --- RDTSC ---
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}


int has_invariant_tsc()
{
    uint32_t a, b, c, d;
    cpuid(0x80000007, &a, &b, &c, &d);
    return (d >> 8) & 1;
}


uint64_t get_tsc_hz()
{
    uint32_t a, b, c, d;

    // CPUID leaf 0x15
    cpuid(0x15, &a, &b, &c, &d);
    if (a && b)
    {
        uint64_t crystal = c ? c : 24000000ULL; // crystal Hz, fallback 24 MHz
        return (crystal * b) / a;
    }

    
    cpuid(0x16, &a, &b, &c, &d);
    if (a)
        return (uint64_t)a * 1000000ULL; // MHz -> Hz

    
    return 3000000000UL;
}


uint64_t get_time_ns_in_second()
{
    // return 0;
    static uint64_t tsc_hz = 0;
    if (tsc_hz < 10)
        tsc_hz = get_tsc_hz();

    uint64_t cycles = rdtsc() * 100;
    uint64_t ns = (cycles * 1000000000ULL) / tsc_hz; // total ns
    return ns % 1000000000ULL;                       
}
struct timeval get_time_us()
{
    struct timeval tmp;
    tmp.tv_sec = get_epoch_seconds();
    tmp.tv_microsec = get_time_ns_in_second() / 1000ULL;
    return tmp;
}
bool later(struct timeval a, struct timeval b)
{
    if (a.tv_sec > b.tv_sec)
        return true;
    if (a.tv_sec < b.tv_sec)
        return false;
    return a.tv_microsec > b.tv_microsec;
}
bool _later(struct timeval a, struct timeval b)
{
    if (a.tv_sec > b.tv_sec)
        return true;
    if (a.tv_sec < b.tv_sec)
        return false;
    return a.tv_microsec >= b.tv_microsec;
}
bool earlier(struct timeval a, struct timeval b)
{
    if (a.tv_sec > b.tv_sec)
        return false;
    if (a.tv_sec < b.tv_sec)
        return true;
    return a.tv_microsec < b.tv_microsec;
}
bool _earlier(struct timeval a, struct timeval b)
{
    if (a.tv_sec > b.tv_sec)
        return false;
    if (a.tv_sec < b.tv_sec)
        return true;
    return a.tv_microsec <= b.tv_microsec;
}
bool same_time(struct timeval a, struct timeval b)
{
    return a.tv_sec = b.tv_sec && a.tv_microsec == b.tv_microsec;
}
struct timeval add_time(struct timeval a, struct timeval b)
{
    struct timeval rs;
    rs.tv_sec = a.tv_sec + b.tv_sec;
    uint32_t milisec_total = a.tv_microsec + b.tv_microsec;
    if (milisec_total / 1000000ULL)
        rs.tv_sec++;
    rs.tv_microsec = milisec_total % 1000000ULL;
    return rs;
}
struct timeval subtract_time(struct timeval a, struct timeval b)
{
    struct timeval tmp = {0};
    if (earlier(a, b))
    {
        return tmp;
    }
    tmp.tv_sec = a.tv_sec - b.tv_sec;
    int64_t milisec_subtracted = a.tv_microsec - b.tv_microsec;
    if (milisec_subtracted < 0)
    {
        tmp.tv_sec--;
        tmp.tv_microsec = 1000000ULL + milisec_subtracted;
    }
    else
    {
        tmp.tv_microsec = milisec_subtracted;
    }
    return tmp;
}
struct timeval add_time_us(struct timeval a, uint32_t add_us)
{
    struct timeval b;
    b.tv_sec = add_us / 1000000ULL;
    b.tv_microsec = add_us % 1000000ULL;
    struct timeval rs = add_time(a, b);
    return rs;
}
struct timeval subtract_time_us(struct timeval a, uint32_t sub_us)
{
    struct timeval b;
    b.tv_sec = sub_us / 1000000ULL;
    b.tv_microsec = sub_us % 1000000ULL;
    return subtract_time(a, b);
}
uint64_t get_time_s()
{
    return get_epoch_seconds();
}
void rtc_write_datetime(int year, int mon, int day, int hour, int min, int sec)
{
    
    outb(0x70, 0x0B);
    uint8_t regB = inb(0x71);

    bool is_binary = regB & 0x04;

    
    outb(0x70, 0x0B);
    outb(0x71, regB | 0x80);

    
    int cent = year / 100;
    int y2 = year % 100;

    
    if (!is_binary)
    {
        sec = bin_to_bcd(sec);
        min = bin_to_bcd(min);
        hour = bin_to_bcd(hour);
        day = bin_to_bcd(day);
        mon = bin_to_bcd(mon);
        y2 = bin_to_bcd(y2);
        cent = bin_to_bcd(cent);
    }

    
    outb(0x70, 0x00);
    outb(0x71, sec);
    outb(0x70, 0x02);
    outb(0x71, min);
    outb(0x70, 0x04);
    outb(0x71, hour);
    outb(0x70, 0x07);
    outb(0x71, day);
    outb(0x70, 0x08);
    outb(0x71, mon);
    outb(0x70, 0x09);
    outb(0x71, y2);

    
    // outb(0x70, 0x32); outb(0x71, cent);

    
    outb(0x70, 0x0B);
    outb(0x71, regB & ~0x80);
}
#define GRND_NONBLOCK 0x0001
#define GRND_RANDOM 0x0002

static uint64_t rand64()
{
    uint64_t tsc = rdtsc();
    uint64_t time = get_epoch_seconds();
    uint64_t mix = tsc ^ (time << 32);

    
    mix ^= (mix >> 33);
    mix *= 0xff51afd7ed558ccdULL;
    mix ^= (mix >> 33);
    mix *= 0xc4ceb9fe1a85ec53ULL;
    mix ^= (mix >> 33);
    return mix;
}

long sys_getrandom(void *buf, size_t buflen, unsigned int flags)
{
    if (!buf)
        return -1;

    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < buflen; i += 8)
    {
        uint64_t r = rand64();
        size_t chunk = (buflen - i >= 8) ? 8 : (buflen - i);
        for (size_t j = 0; j < chunk; j++)
            p[i + j] = ((uint8_t *)&r)[j];
    }
    return buflen;
}
static uint32_t rand32()
{
    return (uint32_t)rand64();
}

static uint32_t rand()
{
    return rand32();
}
static uint64_t rand_range(uint64_t min, uint64_t max)
{
    if (max <= min)
        return min;
    uint64_t r = rand64();
    return min + (r % (max - min + 1));
}
#endif // time.h