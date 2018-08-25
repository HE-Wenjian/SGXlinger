obj-m += sgxlinger.o
KERNEL_VERSION = $(shell uname -r)
ccflags-y := -O2 -mtune=native

all:
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) clean

load:
	sudo insmod sgxlinger.ko core_id=3
