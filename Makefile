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

-include $(src)/config.mk

# This is a hack! But hey.. it works, got any better ideas, send a patch ;)
NOSTDINC_FLAGS := -I$(PWD)/include/ $(CFLAGS)
NOSTDINC_FLAGS := -I$(PWD)/include/ -include $(M)/compat/compat.h $(CFLAGS

obj-y := net/wireless/ net/mac80211/  \
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

install:
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
	@if [ ! -z $(MADWIFI) ]; then \
		echo ;\
		echo -n "Note: madwifi detected, we're going to disable it. "  ;\
		echo "If you would like to enable it later you can run:"  ;\
		echo "    sudo ./scripts/athenable madwifi"  ;\
		echo ;\
		echo Running scripts/athenable ath5k...;\
		./scripts/athenable ath5k ;\
	fi
	@depmod -ae
	@echo
	@echo "Currently detected wireless subsystem modules:"
	@echo 
	@modprobe -l mac80211
	@# rc80211_simple is no longer a module
	@#modprobe -l rc80211_simple
	@modprobe -l cfg80211
	@modprobe -l ath5k
	@modprobe -l ssb
	@modprobe -l b43
	@modprobe -l iwl3945
	@# This needs testing before we add it
	@#modprobe -l iwl4965
	@modprobe -l zd1211rw-mac80211
	@# All the scripts we can use
	@mkdir -p /usr/lib/compat-wireless/
	@install scripts/modlib.sh	/usr/lib/compat-wireless/
	@install scripts/madwifi-unload	/usr/sbin/
	@# This is to allow switching between drivers without blacklisting
	@install scripts/athenable	/usr/sbin/
	@install scripts/b43enable	/usr/sbin/
	@install scripts/athload	/usr/sbin/
	@install scripts/b43load	/usr/sbin/
	@echo 
	@echo Now run: make load
	@echo
	@echo "   sudo make load"
	@echo

uninstall:
	@# New location, matches upstream
	@rm -rf $(KLIB)/$(KMODDIR)/net/mac80211/
	@rm -rf $(KLIB)/$(KMODDIR)/net/wireless/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/ssb/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/net/wireless/
	@depmod -ae
	@/usr/sbin/athenable madwifi
	@/usr/sbin/b43enable bcm43xx
	@echo
	@echo "Your old wireless subsystem modules were left intact:"
	@echo 
	@modprobe -l mac80211
	@# rc80211_simple is a module on 2.6.22 and 2.6.23 though
	@modprobe -l rc80211_simple
	@modprobe -l cfg80211
	@modprobe -l ath5k
	@modprobe -l ssb
	@modprobe -l b43
	@modprobe -l b43legacy
	@modprobe -l iwl3945
	@modprobe -l iwl4965
	@modprobe -l ipw3945
	@modprobe -l ath_pci
	@modprobe -l ath_hal
	@modprobe -l zd1211rw-mac80211
	@modprobe -l bcm43xx
	@echo 

unload:
	@./scripts/unload.sh

load: unload
	@./scripts/load.sh

.PHONY: all clean install uninstall unload load

endif
