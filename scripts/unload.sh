#!/bin/bash

OLD_MODULES="zd1211rw rc80211_simple"
MODULES="$OLD_MODULES ath5k iwl3945 zd1211rw-mac80211"
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
