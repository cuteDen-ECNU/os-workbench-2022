target remote localhost:1234
set architecture i386:x86-64
file /home/wendy/os-workbench-2022/abstract-machine/am/src/x86/qemu/boot/boot.o
file /home/wendy/os-workbench-2022/kernel/build/kernel-x86_64-qemu.elf
b 25
c
