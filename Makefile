KMODDIR?=       updates
KMODDIR_ARG:=   "INSTALL_MOD_DIR=$(KMODDIR)"
ifneq ($(origin $(KLIB)), undefined)
KMODPATH_ARG:=  "INSTALL_MOD_PATH=$(KLIB)"
else
KLIB:=          /lib/modules/$(shell uname -r)
endif
KLIB_BUILD ?=	$(KLIB)/build
MADWIFI=$(shell modprobe -l ath_pci)


ifneq ($(KERNELRELEASE),)

include $(src)/$(COMPAT_CONFIG)

NOSTDINC_FLAGS := -I$(src)/include/ -include $(M)/include/net/compat.h $(CFLAGS)

obj-y := net/wireless/ net/mac80211/ net/ieee80211/ \
	drivers/ssb/ \
	drivers/misc/ \
	drivers/net/usb/ \
	drivers/net/wireless/

else

export PWD :=	$(shell pwd)

# These exported as they are used by the scripts
# to check config and compat autoconf
export COMPAT_CONFIG=config.mk
export CONFIG_CHECK=.$(COMPAT_CONFIG)_md5sum.txt
export COMPAT_AUTOCONF=include/linux/compat_autoconf.h
export CREL=$(shell cat $(PWD)/compat-release)
export CREL_CHECK:=.compat_autoconf_$(CREL)

include $(PWD)/$(COMPAT_CONFIG)

all: modules

modules: $(CREL_CHECK)
	@./scripts/check_config.sh
	$(MAKE) -C $(KLIB_BUILD) M=$(PWD) modules

# With the above and this we make sure we generate a new compat autoconf per
# new relase of compat-wireless-2.6 OR when the user updates the 
# $(COMPAT_CONFIG) file
$(CREL_CHECK):
	@# Force to regenerate compat autoconf
	@rm -f $(CONFIG_CHECK)
	@./scripts/check_config.sh
	@touch $@
	@md5sum $(COMPAT_CONFIG) > $(CONFIG_CHECK)

install: uninstall modules
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
	@modprobe -l p54_pci
	@modprobe -l p54_usb
	@modprobe -l rt2400pci
	@modprobe -l rt2500pci
	@modprobe -l rt2500usb
	@modprobe -l rt61pci
	@modprobe -l rt73usb
	@modprobe -l rndis_host
	@modprobe -l rndis_wlan
	@modprobe -l rtl8180
	@modprobe -l rtl8187
	@# rc80211_simple is no longer a module
	@#modprobe -l rc80211_simple
	@modprobe -l zd1211rw
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
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/net/usb/
	@rm -rf $(KLIB)/$(KMODDIR)/drivers/net/wireless/
	@# Lets only remove the stuff we are sure we are providing
	@# on the misc directory.
	@rm -f $(KLIB)/$(KMODDIR)/drivers/misc/eeprom_93cx6.ko
	@depmod -ae
	@if [ -x /usr/sbin/athenable ]; then /usr/sbin/athenable madwifi; fi
	@if [ -x /usr/sbin/b43enable ]; then /usr/sbin/b43enable bcm43xx; fi
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
	@modprobe -l rndis_host
	@modprobe -l rndis_wlan
	@modprobe -l rtl8180
	@modprobe -l rtl8187
	@# rc80211_simple is no longer a module
	@#modprobe -l rc80211_simple
	@modprobe -l zd1211rw
	@# Old kernels have ieee80211softmac, this will be removed soon :)
	@modprobe -l ieee80211softmac
	@
	@echo 

clean:
	@if [ -d net -a -d $(KLIB_BUILD) ]; then \
		$(MAKE) -C $(KLIB_BUILD) M=$(PWD) clean ;\
	fi
unload:
	@./scripts/unload.sh

load: unload
	@./scripts/load.sh

.PHONY: all clean install uninstall unload load

endif

clean-files += Module.symvers $(CREL_CHECK) $(CONFIG_CHECK)
