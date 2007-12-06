KMODDIR?=       updates
KMODDIR_ARG:=   "INSTALL_MOD_DIR=$(KMODDIR)"
ifneq ($(origin $(KLIB)), undefined)
KMODPATH_ARG:=  "INSTALL_MOD_PATH=$(KLIB)"
else
KLIB:=          /lib/modules/$(shell uname -r)
endif
KLIB_BUILD :=	$(KLIB)/build
MADWIFI=$(shell modprobe -l ath_pci)

ifneq ($(KERNELRELEASE),)

# Bug: $(src) is not working ??
include $(src)/config.mk
export $(COPTS)
COMPAT_WIRELESS=$(HOME)/devel/compat-wireless-2.6
EXTRA_CFLAGS += $(COPTS) 
#NOSTDINC_FLAGS := -I$(PWD)/include/
NOSTDINC_FLAGS := -I$(PWD)/include/ -include $(M)/include/net/compat.h $(CFLAGS)

#NOSTDINC_FLAGS := -I$(PWD)/include/ $(CFLAGS)
#NOSTDINC_FLAGS := -I$(PWD)/include/ -include $(M)/include/net/compat.h $(CFLAGS)

#NOSTDINC_FLAGS := -I$(PWD)/include/ -include $(M)/include/net/compat.h
#NOSTDINC_FLAGS := $(COPTS) $(CFLAGS)

obj-y := net/wireless/ net/mac80211/ net/ieee80211/ \
	drivers/ssb/ \
	drivers/misc/ \
	drivers/net/wireless/

else
export PWD :=	$(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KLIB_BUILD) M=$(PWD) modules

clean:
	$(MAKE) -C $(KLIB_BUILD) M=$(PWD) clean
	@rm -f *.symvers

install: modules
	@# Previous versions of compat installed stuff into different
	@# directories lets make sure we remove that suff for now.
	@rm -rf $(KLIB)/$(KMODDIR)/wireless/
	@rm -rf $(KLIB)/$(KMODDIR)/mac80211/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/ath5k/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/iwlwifi/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/b43/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/ssb/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/zd1211rw-mac80211/
	@$(MAKE) -C $(KLIB_BUILD) M=$(PWD) $(KMODDIR_ARG) $(KMODPATH_ARG) \
		modules_install
	@# All the scripts we can use
	@mkdir -p /usr/lib/compat-wireless/
	@install scripts/modlib.sh	/usr/lib/compat-wireless/
	@install scripts/madwifi-unload	/usr/sbin/
	@# This is to allow switching between drivers without blacklisting
	@install scripts/athenable	/usr/sbin/
	@install scripts/b43enable	/usr/sbin/
	@install scripts/athload	/usr/sbin/
	@install scripts/b43load	/usr/sbin/
	@if [ ! -z $(MADWIFI) ]; then \
		echo ;\
		echo -n "Note: madwifi detected, we're going to disable it. "  ;\
		echo "If you would like to enable it later you can run:"  ;\
		echo "    sudo athenable madwifi"  ;\
		echo ;\
		echo Running athenable ath5k...;\
		/usr/sbin/athenable ath5k ;\
	fi
	@depmod -ae
	@echo
	@echo "Currently detected wireless subsystem modules:"
	@echo 
	@modprobe -l mac80211
	@# rc80211_simple is a module only on 2.6.22 and 2.6.23
	@modprobe -l cfg80211
	@modprobe -l adm8211
	@modprobe -l ath5k
	@modprobe -l b43
	@modprobe -l b43legacy
	@modprobe -l ssb
	@modprobe -l iwl3945
	@modprobe -l iwl4965
	@modprobe -l ipw2100
	@modprobe -l ipw2200
	@modprobe -l ieee80211
	@modprobe -l ieee80211_crypt
	@modprobe -l libertas_cs
	@modprobe -l ub8xxx
	@modprobe -l p54pci
	@modprobe -l p54usb
	@modprobe -l rt2400pci
	@modprobe -l rt2500pci
	@modprobe -l rt2500usb
	@modprobe -l rt61pci
	@modprobe -l rt73usb
	@modprobe -l rtl8180
	@modprobe -l rtl8187
	@# rc80211_simple is no longer a module
	@#modprobe -l rc80211_simple
	@modprobe -l zd1211rw-mac80211
	@echo 
	@echo Now run: make load
	@echo
	@echo "   sudo make load"
	@echo

uninstall:
	@# New location, matches upstream
	@rm -rf $(KLIB)/$(KMODDIR)/net/mac80211/
	@rm -rf $(KLIB)/$(KMODDIR)/net/wireless/
	@rm -rf $(KLIB)/$(KMODDIR)/net/ieee80211/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/ssb/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/net/wireless/
	@# Lets only remove the stuff we are sure we are providing
	@# on the misc directory.
	@rm -f $(KLIB)/$(KMODDIR)/drivers/misc/eeprom_93cx6.ko
	@depmod -ae
	@/usr/sbin/athenable madwifi
	@/usr/sbin/b43enable bcm43xx
	@echo
	@echo "Your old wireless subsystem modules were left intact:"
	@echo 
	@modprobe -l mac80211
	@modprobe -l cfg80211
	@# rc80211_simple is a module on 2.6.22 and 2.6.23 though
	@modprobe -l adm8211
	@modprobe -l ath5k
	@modprobe -l b43
	@modprobe -l b43legacy
	@modprobe -l ssb
	@modprobe -l rc80211_simple
	@modprobe -l iwl3945
	@modprobe -l iwl4965
	@modprobe -l ipw2100
	@modprobe -l ipw2200
	@modprobe -l ieee80211
	@modprobe -l ieee80211_crypt
	@modprobe -l libertas_cs
	@modprobe -l mac80211
	@modprobe -l ub8xxx
	@modprobe -l p54pci
	@modprobe -l p54usb
	@modprobe -l rt2400pci
	@modprobe -l rt2500pci
	@modprobe -l rt2500usb
	@modprobe -l rt61pci
	@modprobe -l rt73usb
	@modprobe -l rtl8180
	@modprobe -l rtl8187
	@# rc80211_simple is no longer a module
	@#modprobe -l rc80211_simple
	@modprobe -l zd1211rw-mac80211
	@# Old kernels have ieee80211softmac, this will be removed soon :)
	@modprobe -l ieee80211softmac
	@
	@echo 

unload:
	@./scripts/unload.sh

load: unload
	@./scripts/load.sh

.PHONY: all clean install uninstall unload load

endif
