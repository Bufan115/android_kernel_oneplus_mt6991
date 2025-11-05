obj-$(CONFIG_KERNEL_HACK) += khack.o

khack-y := entry.o memory.o process.o

ccflags-y := -I$(src) -Wno-unused-function -Wno-unused-variable -Wno-declaration-after-statement -fno-stack-protector
