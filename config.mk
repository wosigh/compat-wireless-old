export

## NOTE
## Make sure to have each variable declaration start
## in the first column, no whitespace allowed.

include $(KLIB)/.config

# Wireless subsystem stuff
CONFIG_MAC80211=m

CONFIG_MAC80211_RC_DEFAULT=pid
CONFIG_MAC80211_RC_PID=y
# Comment above and uncomment below if you are having issues with
# the rate pid control algorithm.
#CONFIG_MAC80211_RC_DEFAULT=simple
#CONFIG_MAC80211_RC_SIMPLE=y

# enable mesh networking too
CONFIG_MAC80211_MESH=y

CONFIG_CFG80211=m
CONFIG_NL80211=y

# PCI Drivers
ifneq ($(CONFIG_PCI),)

CONFIG_ATH5K=m
CONFIG_ATH5K_DEBUG=n
CONFIG_IWL3945=m
CONFIG_IWL4965=m
CONFIG_B43=m
# B43 uses PCMCIA only for Compact Flash. The Cardbus cards uses PCI
# Example, bcm4318:
# http://www.multicap.biz/wireless-lan/indoor-wlan-hardware/sdc-cf10g-80211g-compact-flash-module
CONFIG_B43_PCMCIA=y
CONFIG_B43_DMA=y
CONFIG_B43_PIO=y
CONFIG_B43_DMA_AND_PIO_MODE=y
CONFIG_B43_PCI_AUTOSELECT=y
CONFIG_B43_PCICORE_AUTOSELECT=y
#CONFIG_B43_RFKILL=n
CONFIG_B43_LEDS=y
# CONFIG_B43_DEBUG is not set

CONFIG_B43LEGACY=m
CONFIG_B43LEGACY_PCI_AUTOSELECT=y
CONFIG_B43LEGACY_PCICORE_AUTOSELECT=y
CONFIG_B43LEGACY_DMA=y
CONFIG_B43LEGACY_PIO=y
CONFIG_B43LEGACY_DMA_AND_PIO_MODE=y

# The Intel ipws
CONFIG_IPW2100=m
CONFIG_IPW2200=m
NEED_IEEE80211=y

CONFIG_P54_PCI=m

CONFIG_SSB_PCIHOST_POSSIBLE=y
CONFIG_SSB_PCIHOST=y
CONFIG_SSB_DRIVER_PCICORE_POSSIBLE=y
CONFIG_SSB_DRIVER_PCICORE=y

CONFIG_RTL8180=m
CONFIG_ADM8211=m

ifneq ($(CONFIG_CRC_ITU_T),)
CONFIG_RT2X00_LIB_PCI=m
CONFIG_RT2400PCI=m
CONFIG_RT2500PCI=m
CONFIG_RT61PCI=m
NEED_RT2X00=y
endif

endif
## end of PCI

# This is required for some cards
CONFIG_EEPROM_93CX6=m

# USB Drivers
ifneq ($(CONFIG_USB),)
CONFIG_ZD1211RW=m

# Sorry, it uses cancel_work_sync which is new and can't be done in compat...
ifeq ($(shell test $(shell sed 's/^SUBLEVEL = //;t;d' < $(KLIB)/Makefile) -gt 21 && echo yes),yes)

# Wireless RNDIS USB support (RTL8185 802.11g) A-Link WL54PC
# All of these devices are based on Broadcom 4320 chip which
# is only wireless RNDIS chip known to date.
# Note: this depends on CONFIG_USB_NET_RNDIS_HOST and CONFIG_USB_NET_CDCETHER
# it also requires a new RNDIS_HOST module which we add
CONFIG_USB_NET_RNDIS_HOST=m
CONFIG_USB_NET_RNDIS_WLAN=m

endif

CONFIG_P54_USB=m
CONFIG_RTL8187=m

ifneq ($(CONFIG_CRC_ITU_T),)
CONFIG_RT2X00_LIB_USB=m
CONFIG_RT2500USB=m
CONFIG_RT73USB=m
NEED_RT2X00=y
endif

endif

# rt2x00
ifeq ($(NEED_RT2X00),y)
CONFIG_RT2X00=m
CONFIG_RT2X00_LIB=m
CONFIG_RT2X00_LIB_FIRMWARE=y
# CONFIG_RT2X00_LIB_DEBUGFS is not set
# CONFIG_RT2X00_DEBUG is not se
endif

# p54
CONFIG_P54_COMMON=m

# Sonics Silicon Backplane
CONFIG_SSB_POSSIBLE=y
CONFIG_SSB=m

ifneq ($(CONFIG_PCMCIA),)
CONFIG_SSB_PCMCIAHOST=y
endif

# These two are for mips
CONFIG_SSB_DRIVER_MIPS=n
CONFIG_SSB_PCICORE_HOSTMODE=n
# CONFIG_SSB_DEBUG is not set
# CONFIG_SSB_DRIVER_EXTIF=y

ifeq ($(NEED_IEEE80211),y)
# Old ieee80211 "stack"
# Note: old softmac is scheduled for removal so we
# ignore that stuff
CONFIG_IEEE80211=m
CONFIG_IEEE80211_CRYPT_CCMP=m
CONFIG_IEEE80211_CRYPT_TKIP=m
CONFIG_IEEE80211_CRYPT_WEP=m
CONFIG_IEEE80211_SOFTMAC=n
# Old drivers which use the old stack
endif

# Libertas uses the old stack but not fully, it will soon 
# be cleaned.
ifneq ($(CONFIG_USB),)
CONFIG_LIBERTAS_USB=m
NEED_LIBERTAS=y
endif
ifneq ($(CONFIG_PCMCIA),)
CONFIG_LIBERTAS_CS=m
NEED_LIBERTAS=y
endif
ifeq ($(NEED_LIBERTAS),y)
CONFIG_LIBERTAS=m
endif
