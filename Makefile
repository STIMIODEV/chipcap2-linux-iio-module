KVER=$(shell uname -r | cut -f1-3 -d.)
KDIR=/usr/src/linux-headers-$(KVER)/
ccflags-y=-Wall -I$(KDIR)

# kernel/Documentation/kbuild/modules.txt
obj-m := chipcap2.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	@echo $(KVER) $(NKDIR)
	rm -rf *.o *.ko .*.cmd *.mod.* .cache.mk .tmp_versions modules.order Module.symvers
