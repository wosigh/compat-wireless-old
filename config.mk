
# XXX: Wouldn't it be nice to have a menuconfig ?
#
# Note: these CONFIG options won't be picked up through the 
# all *.[ch] files themselves so you should still define required
# CONFIG_ items on compat.h, this is just a hack for
# build time.
export

# Wireless subsystem stuff
CONFIG_MAC80211=m
CONFIG_MAC80211_RCSIMPLE=y
CONFIG_CFG80211=m
# Not yet supported in this compat work
CONFIG_NL80211=n

# Drivers
CONFIG_ATH5K=m
CONFIG_IWL3945=m
# This guy just needs testing
CONFIG_IWL4965=n
CONFIG_ZD1211RW_MAC80211=m

# Not yet, set to m when ready
CONFIG_B43=n
CONFIG_B43_RFKILL=n
CONFIG_B43_LEDS=n
CONFIG_B43_PCMCIA=n
CONFIG_B43_DEBUG=n
CONFIG_B43_DMA=y
CONFIG_B43_PIO=y

CONFIG_SSB=n
CONFIG_SSB_PCIHOST=n
CONFIG_SSB_PCMCIAHOST=n
CONFIG_SSB_DRIVER_MIPS=n
CONFIG_SSB_DRIVER_EXTIF=n
CONFIG_SSB_DRIVER_PCICORE=n
CONFIG_SSB_PCIHOST=n
