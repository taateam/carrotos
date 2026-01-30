# =========================
# Toolchains
# =========================

# Linux ELF loader
CC_LINUX = gcc

# Windows PE ntdll
CC_WIN   = x86_64-w64-mingw32-gcc
LD_WIN   = x86_64-w64-mingw32-ld
NASM     = nasm

# =========================
# Targets
# =========================

LOADER   = ninja
NTDLL    = ninja.dll

# =========================
# Loader (ELF)
# =========================

LOADER_OBJS = ninja.o 

LOADER_CFLAGS = \
	-O0 -g3 \
	-fno-omit-frame-pointer \
	-Wall -Wextra

LOADER_LDFLAGS = \
	-no-pie \
	-ldl

# =========================
# ntdll (PE)
# =========================

NTDLL_OBJS = ninja-dll.o

NTDLL_CFLAGS = \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-m64 \
	-Wall -Wextra

NTDLL_LDFLAGS = \
	-O0 -g3 \
	-shared \
	-nostdlib \
	-Wl,--entry,0 \
	-Wl,--subsystem,windows \
	-Wl,ntdll/ninja.def \
	-Wl,--enable-stdcall-fixup

# =========================
# Rules
# =========================

all: $(LOADER) $(NTDLL)

# ---- ELF loader ----
$(LOADER): $(LOADER_OBJS)
	$(CC_LINUX) -o $@ $^ $(LOADER_LDFLAGS)

%.o: %.c
	$(CC_LINUX) $(LOADER_CFLAGS) -c -o $@ $<

# ---- PE ntdll ----
ninja-dll.o: ntdll/ninja_dll.c
	$(CC_WIN) $(NTDLL_CFLAGS) -c -o $@ $<

$(NTDLL): $(NTDLL_OBJS)
	$(CC_WIN) $(NTDLL_LDFLAGS) -o $@ $^

# ======================================
WINAPP = winapp.exe
debug: $(LOADER)
	gdb --args ./$(LOADER) $(WINAPP)

run: $(LOADER)
	./$(LOADER) $(WINAPP)
# =========================
# Clean
# =========================

clean:
	rm -f $(LOADER) $(NTDLL) *.o
