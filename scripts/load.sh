#!/bin/bash
MODULES="iwl3945 zd1211rw-mac80211"
for i in $MODULES; do
	echo Loading $i...
	modprobe $i
done
# For ath5k we must be sure to unload MadWifi first
athload ath5k
