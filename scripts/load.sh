#!/bin/bash
MODULES="ath5k iwl3945 zd1211rw-mac80211"
for i in $MODULES; do
	echo Loading $i...
	modprobe $i
done

