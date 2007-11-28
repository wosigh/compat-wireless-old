#!/bin/bash

# The old stack drivers and the mac80211 rc80211_simple modules 
# which is no longer on recent kernels (its internal)
OLD_MODULES="rc80211_simple zd1211rw bcm43xx"
MODULES="$OLD_MODULES"
MODULES="$MODULES ieee80211softmac ieee80211_crypt ieee80211"
MODULES="$MODULES ipw2100 ipw2200"
MODULES="$MODULES libertas libertas_cs ub8xxx"
MODULES="$MODULES adm8211"
MODULES="$MODULES b43 b43legacy"
MODULES="$MODULES iwl3945 iwl4965"
MODULES="$MODULES ath5k zd1211rw-mac80211"
MODULES="$MODULES p54pci p54usb p54common"
MODULES="$MODULES rt2400pci rt2500pci rt61pci"
MODULES="$MODULES rt2500usb rt73usb"
MODULES="$MODULES rt2x00usb rt2x00lib"
MODULES="$MODULES rtl8180 rtl8187"
# eeprom_93cx6 is used by rt2x00 (rt61pci, rt2500pci, rt2400pci) 
# and Realtek drivers ( rtl8187, rtl8180)
MODULES="$MODULES eeprom_93cx6"
MODULES="$MODULES mac80211 cfg80211"
MADWIFI_MODULES="ath_pci ath_rate_sample wlan_scan_sta wlan ath_hal"
IPW3945D="/sbin/ipw3945d-`uname -r`"

if [ -f $IPW3945D ]; then
	$IPW3945D --isrunning
	if [ ! $? ]; then 
		echo -n "Detected ipw3945 daemon loaded we're going to "
		echo "shut the daemon down now and remove the module."
		modprobe -r --ignore-remove ipw3945
	fi
fi

grep ath_pci /proc/modules 2>&1 > /dev/null
if [ $?  -eq 0 ]; then
	echo "MadWifi driver is loaded, going to try to unload it..."
	./scripts/madwifi-unload
fi

for i in $MODULES; do
	echo Unloading $i...
	modprobe -r --ignore-remove $i
done
