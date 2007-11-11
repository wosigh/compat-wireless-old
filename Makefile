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

# This is a hack! But hey.. it works, got any better ideas, send a patch ;)
NOSTDINC_FLAGS := -I$(PWD)/include/ $(CFLAGS)

obj-y := wireless/ mac80211/  \
	drivers/ath5k/ \
	drivers/iwlwifi/ \
	drivers/zd1211rw-mac80211/ \
#	drivers/b43/ drivers/ssb/ \
# b43 needs a bit more compat work

else
PWD :=	$(shell pwd)
export PWD

all: modules

modules:
	$(MAKE) -C $(KLIB_BUILD) M=$(PWD) modules

clean:
	$(MAKE) -C $(KLIB_BUILD) M=$(PWD) clean

install:
	@$(MAKE) -C $(KLIB_BUILD) M=$(PWD) $(KMODDIR_ARG) $(KMODPATH_ARG) \
		modules_install
	@if [ ! -z $(MADWIFI) ]; then \
		echo ;\
		echo -n "Note: madwifi detected, rename the module "  ;\
		echo "to keep it out of our way, example:" ;\
		echo "   sudo mv $(MADWIFI) $(MADWIFI).ignore" ;\
		echo "   depmod -ae" ;\
		echo ;\
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
	@echo 

uninstall:
	@rm -rf $(KLIB)/$(KMODDIR)/mac80211/
	@rm -rf $(KLIB)/$(KMODDIR)/wireless/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/
	@depmod -ae
	@echo
	@echo "Your old wireless subsystem modules were left intact:"
	@echo 
	@modprobe -l mac80211
	@# It is on 2.6.22 and 2.6.23 though
	@modprobe -l rc80211_simple
	@modprobe -l cfg80211
	@modprobe -l ath5k
	@modprobe -l ssb
	@modprobe -l b43
	@modprobe -l iwl3945
	@modprobe -l iwl4965
	@modprobe -l ipw3945
	@modprobe -l ath_pci
	@modprobe -l ath_hal
	@modprobe -l zd1211rw-mac80211
	@echo 

unload:
	@./scripts/unload.sh

load: unload
	@./scripts/load.sh

.PHONY: all clean install uninstall unload load

endif
