#!/bin/bash
# 
# Copyright 2007, 2008	Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
#
# Use this to update compat-wireless-2.6 to the latest
# wireless-testing.git tree you have.
#
# Usage: you should have the latest pull of wireless-2.6.git
# git://git.kernel.org/pub/scm/linux/kernel/git/linville/wireless-testing.git
# We assume you have it on your ~/devel/wireless-testing/ directory. If you do,
# just run this script from the compat-wireless-2.6 directory.
# You can specify where your GIT_TREE is by doing:
#
# export GIT_TREE=/home/mcgrof/wireless-testing/
# 
# for example
#
GIT_URL="git://git.kernel.org/pub/scm/linux/kernel/git/linville/wireless-testing.git"

INCLUDE_LINUX="ieee80211.h nl80211.h wireless.h"
INCLUDE_LINUX="$INCLUDE_LINUX pci_ids.h bitops.h eeprom_93cx6.h pm_qos_params.h"

# For rndis_wext
INCLUDE_LINUX_USB="usbnet.h rndis_host.h"

# Stuff that should die or be merged, only ipw uses it
INCLUDE_NET_OLD="ieee80211.h ieee80211_crypt.h"
# The good new yummy stuff
INCLUDE_NET="$INCLUDE_NET_OLD cfg80211.h ieee80211_radiotap.h iw_handler.h"
INCLUDE_NET="$INCLUDE_NET mac80211.h wext.h wireless.h"

NET_DIRS="wireless mac80211 ieee80211"
# User exported this variable
if [ -z $GIT_TREE ]; then
	GIT_TREE="/home/$USER/devel/wireless-testing/"
	if [ ! -d $GIT_TREE ]; then
		echo "Please tell me where your wireless-testing git tree is."
		echo "You can do this by exporting its location as follows:"
		echo
		echo "  export GIT_TREE=/home/$USER/wireless-testing/"
		echo
		echo "If you do not have one you can clone the repository:"
		echo "  git-clone $GIT_URL"
		exit 1
	fi
else
	echo "You said your wireless-testing git tree is: $GIT_TREE"
fi
# Drivers that have their own directory
DRIVERS="drivers/net/wireless/ath5k"
DRIVERS="$DRIVERS drivers/ssb"
DRIVERS="$DRIVERS drivers/net/wireless/b43"
DRIVERS="$DRIVERS drivers/net/wireless/b43legacy"
DRIVERS="$DRIVERS drivers/net/wireless/iwlwifi"
DRIVERS="$DRIVERS drivers/net/wireless/rt2x00"
DRIVERS="$DRIVERS drivers/net/wireless/zd1211rw"
DRIVERS="$DRIVERS drivers/net/wireless/libertas"
DRIVERS="$DRIVERS drivers/net/wireless/p54"

# Drivers that belong the the wireless directory
DRIVER_FILES="rtl818x.h rtl8180_sa2400.h rtl8180_max2820.h"
DRIVER_FILES="$DRIVER_FILES rtl8180_sa2400.h rtl8180_sa2400.c"
DRIVER_FILES="$DRIVER_FILES rtl8180_max2820.h rtl8180_max2820.c"
DRIVER_FILES="$DRIVER_FILES rtl8180.h rtl8180_rtl8225.h"
DRIVER_FILES="$DRIVER_FILES rtl8180_dev.c rtl8180_rtl8225.c"
DRIVER_FILES="$DRIVER_FILES rtl8187.h rtl8187_rtl8225.h"
DRIVER_FILES="$DRIVER_FILES rtl8180_grf5101.c rtl8180_grf5101.h"
DRIVER_FILES="$DRIVER_FILES rtl8187_dev.c rtl8187_rtl8225.c"
DRIVER_FILES="$DRIVER_FILES adm8211.c  adm8211.h"
DRIVER_FILES="$DRIVER_FILES ipw2100.h ipw2100.c"
DRIVER_FILES="$DRIVER_FILES ipw2200.h ipw2200.c"
DRIVER_FILES="$DRIVER_FILES rndis_wlan.c"
DRIVER_FILES="$DRIVER_FILES at76_usb.h at76_usb.c"

mkdir -p include/linux/ include/net/ include/linux/usb \
	net/mac80211/ net/wireless/ net/ieee80211/ \
	drivers/ssb/ \
	drivers/net/usb/ \
	drivers/net/wireless/

# include/linux
DIR="include/linux"
for i in $INCLUDE_LINUX; do
	echo "Copying $GIT_TREE/$DIR/$i"
	cp "$GIT_TREE/$DIR/$i" $DIR/
done

cp -a $GIT_TREE/include/linux/ssb include/linux/

# include/net
DIR="include/net"
for i in $INCLUDE_NET; do
	echo "Copying $GIT_TREE/$DIR/$i"
	cp "$GIT_TREE/$DIR/$i" $DIR/
done

DIR="include/linux/usb"
for i in $INCLUDE_LINUX_USB; do
	echo "Copying $GIT_TREE/$DIR/$i"
	cp $GIT_TREE/$DIR/$i $DIR/
done

# net/wireless and net/mac80211
for i in $NET_DIRS; do
	echo "Copying $GIT_TREE/net/$i/*.[ch]"
	cp $GIT_TREE/net/$i/*.[ch] net/$i/
	cp $GIT_TREE/net/$i/Makefile net/$i/
	rm -f net/$i/*.mod.c
done

# Drivers in their own directory
for i in $DRIVERS; do
	mkdir -p $i
	echo "Copying $GIT_TREE/$i/*.[ch]"
	cp $GIT_TREE/$i/*.[ch] $i/
	cp $GIT_TREE/$i/Makefile $i/
	rm -f $i/*.mod.c
done

# For rndis_wlan, we need a new rndis_host
DIR="drivers/net/usb"
echo "Copying $GIT_TREE/$DIR/rndis_host.c"
cp $GIT_TREE/$DIR/rndis_host.c $DIR/
echo "Copying $GIT_TREE/$DIR/Makefile"
cp $GIT_TREE/$DIR/Makefile $DIR/

# Misc
mkdir -p drivers/misc/
cp $GIT_TREE/drivers/misc/eeprom_93cx6.c drivers/misc/
cp $GIT_TREE/drivers/misc/Makefile drivers/misc/

DIR="drivers/net/wireless"
# Drivers part of the wireless directory
for i in $DRIVER_FILES; do
	cp $GIT_TREE/$DIR/$i $DIR/
done
# Top level wireless driver Makefile
cp $GIT_TREE/$DIR/Makefile $DIR

# Compat stuff
cp compat/compat.c net/wireless/
cp compat/compat.h include/net/

patch -p1 -N -t < compat/compat.diff
RET=$?
if [[ $RET -ne 0 ]]; then
	echo "Patching compat.diff failed, update it"
	exit $RET
fi
DIR="$PWD"
cd $GIT_TREE && git-describe > $DIR/git-describe && cd $DIR
echo "Updated from ${GIT_TREE}, git-describe says:"
cat git-describe
if [ -d ./.git ]; then
	git-describe > compat-release
fi
echo "This is compat-release:"
cat compat-release
