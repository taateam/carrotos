all: kernel.elf testapp iso 

boot.o: boot.asm
	mkdir -p out/
	nasm -f elf64 -g -F dwarf boot.asm -o out/boot.o
	nasm -f elf64 -g -F dwarf asm/scheduler.asm -o out/scheduler.o
	nasm -f elf64 -g -F dwarf asm/syscall.asm -o out/syscall.o
	nasm -f elf64 -g -F dwarf asm/interrupt.asm -o out/interrupt.o
	nasm -f bin asm/core_start.asm -o out/core_start.bin
	nasm -f elf64 -g -F dwarf asm/core_start_stage2.asm -o out/core_start_stage2.o

kernel.o: kernel.c
	mkdir -p out/
	x86_64-elf-gcc -mno-red-zone -mcmodel=large -c \
	-fno-stack-protector \
    -fno-pic -fno-pie \
    -ffreestanding \
	-mno-mmx -mno-sse -mno-sse2 \
    -m64 -O0 -g kernel.c -o out/kernel.o

kernel.elf: boot.o kernel.o linker.ld
	mkdir -p out/put
	cp out/core_start.bin out/put/
	x86_64-elf-ld -n -o out/put/kernel.elf -T linker.ld out/kernel.o \
		out/boot.o out/scheduler.o out/syscall.o out/interrupt.o out/core_start_stage2.o 


iso: kernel.elf grub/grub.cfg
	mkdir -p isodir/boot/grub
	cp out/put/* isodir/boot/
	cp grub/grub.cfg isodir/boot/grub/
	grub-mkrescue -o out/carrot.iso isodir

testapp:
	pushd testapp
	bash testapp/build.sh #
	popd # g

run:
	qemu-system-x86_64 -vga std -smp 1 -cpu Haswell,phys-bits=47,+avx,+xsave -d int -m 1G \
	    -cdrom out/carrot.iso -boot d -drive file=disk.img,format=raw,if=ide \
		-netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device e1000e,netdev=net0 \
		-no-shutdown -no-reboot

debug:
	qemu-system-x86_64 -vga std -smp 1 -cpu Haswell,phys-bits=47,+avx,+xsave -d int -m 1G \
		-cdrom out/carrot.iso -boot d -drive file=disk.img,format=raw,if=ide \
		-netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device e1000e,netdev=net0 \
		-s -S -no-shutdown -no-reboot

clean:
	rm -rf out *.o *.bin isodir *.elf *.iso
