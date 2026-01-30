#!/bin/bash
set -e

#
echo "[*] Building testapp..."
x86_64-elf-gcc -ffreestanding -nostdlib -O0 -g -c testapp.c -o testapp.o
x86_64-elf-ld -T testapp.ld -o testapp.elf testapp.o
x86_64-elf-gcc -ffreestanding -nostdlib -O0 -g -c testapp1.c -o testapp1.o
x86_64-elf-ld -T testapp1.ld -o testapp1.elf testapp1.o
x86_64-elf-gcc -ffreestanding -nostdlib -O0 -g -c testapp2.c -o testapp2.o
x86_64-elf-ld -T testapp2.ld -o testapp2.elf testapp2.o 
x86_64-w64-mingw32-gcc winapp.c -O0 \
  -g \
  -fno-omit-frame-pointer \
  -fno-pic \
  -no-pie \
  -Wl,--disable-dynamicbase \
  -o winapp.exe

# 2. Mount FAT32 disk img
echo "[*] Mounting disk.img..."
sudo mkdir -p /mnt/carrot_disk
lo_name=$(sudo losetup -Pf --show ../disk.img)
sudo mount -o rw ${lo_name}p1 /mnt/carrot_disk

#
echo "[*] Copying testapp.elf to /bin..."
sudo rm -f /mnt/carrot_disk/bin/testapp.elf
sudo rm -f /mnt/carrot_disk/bin/testapp1.elf
sudo rm -f /mnt/carrot_disk/bin/testapp2.elf
sudo mkdir -p /mnt/carrot_disk/bin
sudo cp testapp.elf /mnt/carrot_disk/bin/testapp.elf
sudo cp testapp1.elf /mnt/carrot_disk/bin/testapp1.elf
sudo cp testapp2.elf /mnt/carrot_disk/bin/testapp2.elf
sudo cp winapp.exe /mnt/carrot_disk/bin/winapp.exe
sync

read
# 4. Unmount
echo "[*] Unmounting..."
sudo umount /mnt/carrot_disk
sudo losetup -d $lo_name
echo "[✓] Done! testapp.elf is now in disk.img under /bin/"

