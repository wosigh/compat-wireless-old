export

## NOTE
## Make sure to have each variable declaration start
## in the first column, no whitespace allowed.

ifeq ($(wildcard $(KLIB_BUILD)/.config),)
# These will be ignored by compat autoconf
 CONFIG_PCI=y
 CONFIG_USB=n
 CONFIG_PCMCIA=n
else
include $(KLIB_BUILD)/.config
endif

# Wireless subsystem stuff
CONFIG_MAC80211=n

# Enable QOS for 2.6.22, we'll do some hacks here to enable it.
# You will need this for HT support (802.11n) and WME (802.11e).
# If you are >= 2.6.23 we'll only warn when you don't have MQ support
# or NET_SCHED enabled.
#
# We could consider just quiting if MQ and NET_SCHED is disabled
# as I suspect all users of this package want 802.11e (WME) and
# 802.11n (HT) support.
ifeq ($(shell test -e $(KLIB_BUILD)/Makefile && echo yes),yes)
KERNEL_SUBLEVEL = $(shell $(MAKE) -C $(KLIB_BUILD) kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
ifeq ($(shell test $(KERNEL_SUBLEVEL) -lt 23 && echo yes),yes)
CONFIG_MAC80211_QOS=y
else

# we're in a kernel >= 2.6.23

# we're in kernel >= 2.6.27
ifeq ($(shell test $(KERNEL_SUBLEVEL) -gt 26 && echo yes),yes)
$(error "ERROR: You should use another tree/tarball for newer kernels, this one is for kenrels <= 2.6.26")
endif

ifneq ($(KERNELRELEASE),) # This prevents a warning

ifeq ($(CONFIG_NETDEVICES_MULTIQUEUE),) # checks MQ first
 QOS_REQS_MISSING+=CONFIG_NETDEVICES_MULTIQUEUE
endif

ifeq ($(CONFIG_NET_SCHED),)
 QOS_REQS_MISSING+=CONFIG_NET_SCHED
endif

ifeq ($(QOS_REQS_MISSING),) # if our dependencies match for MAC80211_QOS
ifneq ($(CONFIG_MAC80211_QOS),) # Your kernel has CONFIG_MAC80211_QOS defined already, too bad
$(error "ERROR: CONFIG_MAC80211_QOS is somehow enabled in your kernel, how did that happen if it wasn't an option in your kernel? Please report this to the linux-wireless mailing list!")
endif
CONFIG_MAC80211_QOS=n
else
# Complain about our missing dependencies, at this point we know
# CONFIG_MAC80211_QOS is not enabled by this kernel so
# we can move on and compile this package without WME at all.
# mac80211 WME requires:
#
# CONFIG_NETDEVICES_MULTIQUEUE
# CONFIG_NET_SCHED
#
# on kernels 2.6.23..2.6.26. If on older kernels (<= 2.6.22) we
# provide our own hacked up WME without needing these two kernel
# configuration options.
$(warning "WARNING: You are running a kernel >= 2.6.23, you should enable in it $(QOS_REQS_MISSING) for 802.11[ne] support")
endif

endif # In build module mode

endif # kernel release check
endif # kernel Makefile check

CONFIG_MAC80211_RC_DEFAULT=pid
CONFIG_MAC80211_RC_PID=n

# enable mesh networking too
CONFIG_MAC80211_MESH=n

CONFIG_CFG80211=n
CONFIG_NL80211=n

# mac80211 test driver
CONFIG_MAC80211_HWSIM=n

# PCI Drivers
ifneq ($(CONFIG_PCI),)

CONFIG_ATH5K=n
CONFIG_ATH5K_DEBUG=n

# For now we build ath9k only on kernel 2.6.26
ifeq ($(shell test -e $(KLIB_BUILD)/Makefile && echo yes),yes)
KERNEL_SUBLEVEL = $(shell $(MAKE) -C $(KLIB_BUILD) kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
ifeq ($(shell test $(KERNEL_SUBLEVEL) -gt 25 && echo yes),yes)
endif
CONFIG_ATH9K=n
endif

# Required for older kernels which still use this flag.

CONFIG_IWL3945=n
CONFIG_IWL3945_DEBUG=n
CONFIG_IWL3945_LEDS=n
# CONFIG_IWL3945_RFKILL=y
CONFIG_IWL3945_SPECTRUM_MEASUREMENT=n
CONFIG_IWL4965=n
CONFIG_IWL5000=n
CONFIG_IWLAGN=n
CONFIG_IWLAGN_LEDS=n
CONFIG_IWLAGN_SPECTRUM_MEASUREMENT=n
CONFIG_IWLCORE=n
CONFIG_IWLWIFI=n
CONFIG_IWLWIFI_DEBUG=n
CONFIG_IWLWIFI_LEDS=n
# CONFIG_IWLWIFI_RFKILL=y

CONFIG_B43=n
# B43 uses PCMCIA only for Compact Flash. The Cardbus cards uses PCI
# Example, bcm4318:
# http://www.multicap.biz/wireless-lan/indoor-wlan-hardware/sdc-cf10g-80211g-compact-flash-module
ifneq ($(CONFIG_PCMCIA),)
CONFIG_B43_PCMCIA=n
endif
CONFIG_B43_PIO=n
# B43_PIO selects SSB_BLOCKIO
CONFIG_SSB_BLOCKIO=n
CONFIG_B43_PCI_AUTOSELECT=n
# CONFIG_B43_RFKILL=y
CONFIG_B43_LEDS=n
CONFIG_B43_PHY_LP=n
CONFIG_B43_NPHY=n
CONFIG_B43_DEBUG=n

CONFIG_B43LEGACY=n
CONFIG_B43LEGACY_PCI_AUTOSELECT=n
CONFIG_B43LEGACY_DMA=n
CONFIG_B43LEGACY_PIO=n

# The Intel ipws
CONFIG_IPW2100=n
CONFIG_IPW2100_DEBUG=n
CONFIG_IPW2100_MONITOR=n
CONFIG_IPW2200=n
CONFIG_IPW2200_MONITOR=n
CONFIG_IPW2200_RADIOTAP=n
CONFIG_IPW2200_PROMISCUOUS=n
# The above enables use a second interface prefixed 'rtap'.
#           Example usage:
#
# % modprobe ipw2200 rtap_iface=1
# % ifconfig rtap0 up
# % tethereal -i rtap0
#
# If you do not specify 'rtap_iface=1' as a module parameter then
# the rtap interface will not be created and you will need to turn
# it on via sysfs:
#
# % echo 1 > /sys/bus/pci/drivers/ipw2200/*/rtap_iface
CONFIG_IPW2200_QOS=n

NEED_IEEE80211=n

CONFIG_P54_PCI=n

CONFIG_SSB_PCIHOST=n
CONFIG_SSB_DRIVER_PCICORE=n
CONFIG_SSB_B43_PCI_BRIDGE=n
ifeq ($(shell test $(KERNEL_SUBLEVEL) -gt 22 && echo yes),yes)
# b44 is not ported to 2.6.22
CONFIG_B44=n
endif

CONFIG_RTL8180=n
CONFIG_ADM8211=n

CONFIG_RT2X00_LIB_PCI=n
CONFIG_RT2400PCI=n
CONFIG_RT2500PCI=n
NEED_RT2X00=n

# Two rt2x00 drivers require firmware: rt61pci and rt73usb. They depend on
# CRC to check the firmware. We check here first for the PCI
# driver as we're in the PCI section.
ifneq ($(CONFIG_CRC_ITU_T),)
CONFIG_RT61PCI=n
NEED_RT2X00_FIRMWARE=n
endif

endif
## end of PCI

# This is required for some cards
CONFIG_EEPROM_93CX6=n

# USB Drivers
ifneq ($(CONFIG_USB),)
CONFIG_ZD1211RW=n

# support for USB Wireless devices using Atmel at76c503,
# at76c505 or at76c505a chips.
CONFIG_USB_ATMEL=n

# Stuff here things which depend on kernel versions for USB
ifeq ($(shell test -e $(KLIB_BUILD)/Makefile && echo yes),yes)
KERNEL_SUBLEVEL = $(shell $(MAKE) -C $(KLIB_BUILD) kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
ifeq ($(shell test $(KERNEL_SUBLEVEL) -gt 21 && echo yes),yes)

# Sorry, rndis_wlan uses cancel_work_sync which is new and can't be done in compat...

# Wireless RNDIS USB support (RTL8185 802.11g) A-Link WL54PC
# All of these devices are based on Broadcom 4320 chip which
# is only wireless RNDIS chip known to date.
# Note: this depends on CONFIG_USB_NET_RNDIS_HOST and CONFIG_USB_NET_CDCETHER
# it also requires new RNDIS_HOST and CDC_ETHER modules which we add
CONFIG_USB_NET_RNDIS_HOST=n
CONFIG_USB_NET_RNDIS_WLAN=n
CONFIG_USB_NET_CDCETHER=n

endif
endif

CONFIG_P54_USB=n
CONFIG_RTL8187=n

# RT2500USB does not require firmware
CONFIG_RT2500USB=n
CONFIG_RT2X00_LIB_USB=n
NEED_RT2X00=n
# RT73USB requires firmware
ifneq ($(CONFIG_CRC_ITU_T),)
CONFIG_RT73USB=n
NEED_RT2X00_FIRMWARE=n
endif

# we're in kernel >= 2.6.24, this has been only tested on 2.6.24.
# If you add/test backport to older kernels please expand reduce this
# or remove it if its backported down to 2.6.22 or 2.6.21.
ifeq ($(shell test $(KERNEL_SUBLEVEL) -gt 23 && echo yes),yes)
CONFIG_AR9170=n
endif

endif # end of USB driver list

# Common rt2x00 requirements
ifeq ($(NEED_RT2X00),y)
CONFIG_RT2X00=n
CONFIG_RT2X00_LIB=n
# CONFIG_RT2X00_LIB_DEBUGFS is not set
# CONFIG_RT2X00_DEBUG is not set
endif

ifeq ($(NEED_RT2X00_FIRMWARE),y)
CONFIG_RT2X00_LIB_FIRMWARE=n
endif

# p54
CONFIG_P54_COMMON=n

# Sonics Silicon Backplane
CONFIG_SSB_POSSIBLE=n
CONFIG_SSB=n
CONFIG_SSB_SPROM=n

ifneq ($(CONFIG_PCMCIA),)
CONFIG_SSB_PCMCIAHOST=n
endif

# These two are for mips
CONFIG_SSB_DRIVER_MIPS=n
CONFIG_SSB_PCICORE_HOSTMODE=n
# CONFIG_SSB_DEBUG is not set
# CONFIG_SSB_DRIVER_EXTIF=y

CONFIG_LIBERTAS_SDIO=m
NEED_LIBERTAS=y
ifeq ($(NEED_LIBERTAS),y)
CONFIG_LIBERTAS=m
# Libertas uses the old stack but not fully, it will soon 
# be cleaned.
NEED_IEEE80211=y
endif

ifeq ($(NEED_IEEE80211),y)
# Old ieee80211 "stack"
# Note: old softmac is scheduled for removal so we
# ignore that stuff
CONFIG_IEEE80211=m
CONFIG_IEEE80211_CRYPT_CCMP=m
CONFIG_IEEE80211_CRYPT_TKIP=m
CONFIG_IEEE80211_CRYPT_WEP=m
CONFIG_IEEE80211_SOFTMAC=n
endif
