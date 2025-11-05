obj-$(CONFIG_KERNEL_HACK) += khack.o

khack-y := entry.o memory.o process.o

ccflags-y := -I$(src) -Wno-unused-function -Wno-unused-variable -Wno-declaration-after-statement -fno-stack-protector

# 移除 -Werror 以避免警告被视为错误
# ccflags-y += -Werror
