#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}
static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void pci_write(uint32_t addr)
{
    asm volatile("outl %0, %1" ::"a"(addr), "Nd"(PCI_CONFIG_ADDRESS));
}

uint32_t pci_read(void)
{
    uint32_t data;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(PCI_CONFIG_DATA));
    return data;
}

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t address = (1u << 31) // enable bit
                       | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
    pci_write(address);
    return pci_read(); 
}
void pci_write32(uint8_t bus, uint8_t dev, uint8_t func,
                 uint8_t offset, uint32_t value)
{
    uint32_t address = (1u << 31) // enable bit
                       | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
    pci_write(address);
    asm volatile("outl %0, %1" ::"a"(value), "Nd"(PCI_CONFIG_DATA));
}
void pci_write16(uint8_t bus, uint8_t dev, uint8_t func,
                 uint8_t offset, uint16_t value)
{
    uint32_t old = pci_read32(bus, dev, func, offset & ~3);
    uint32_t shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFFu << shift;
    uint32_t newval = (old & ~mask) | ((uint32_t)value << shift);
    pci_write32(bus, dev, func, offset & ~3, newval);
}
void pci_write8(uint8_t bus, uint8_t dev, uint8_t func,
                uint8_t offset, uint8_t value)
{
    uint32_t old = pci_read32(bus, dev, func, offset & ~3);
    uint32_t shift = (offset & 3) * 8;
    uint32_t mask = 0xFFu << shift;
    uint32_t newval = (old & ~mask) | ((uint32_t)value << shift);
    pci_write32(bus, dev, func, offset & ~3, newval);
}

uint16_t pci_read_vendor(uint8_t bus, uint8_t dev, uint8_t func)
{
    uint32_t addr = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8);
    pci_write(addr);
    return (uint16_t)(pci_read() & 0xFFFF);
}

uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint8_t offset_aligned = offset & ~3;
    uint32_t v = pci_read32(bus, dev, func, offset_aligned);
    uint16_t rs = v >> ((offset - offset_aligned) * 8);
    return rs;
}

uint8_t pci_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t v = pci_read32(bus, dev, func, offset & ~3);
    return (v >> ((offset & 3) * 8)) & 0xFF;
}

uint16_t pci_config_read16(uint8_t bus, uint8_t device,
                           uint8_t function, uint8_t offset)
{
    uint32_t address = (uint32_t)((1 << 31) |
                                  ((uint32_t)bus << 16) |
                                  ((uint32_t)device << 11) |
                                  ((uint32_t)function << 8) |
                                  (offset & 0xFC) // aligned to 32 bits
    );

    outl(0xCF8, address);

    uint32_t value = inl(0xCFC);

    return (uint16_t)((value >> ((offset & 2) * 8)) & 0xFFFF);
}
uint8_t pci_config_read8(uint8_t bus, uint8_t device,
                         uint8_t function, uint8_t offset)
{
    uint32_t address = (uint32_t)((1 << 31) |
                                  ((uint32_t)bus << 16) |
                                  ((uint32_t)device << 11) |
                                  ((uint32_t)function << 8) |
                                  (offset & 0xFC));

    outl(0xCF8, address);

    uint32_t value = inl(0xCFC);

    return (uint8_t)((value >> ((offset & 3) * 8)) & 0xFF);
};
