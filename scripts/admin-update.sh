#!/bin/bash
# 
# Copyright 2007	Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
#
# Use this to update compat-wireless-2.6 to the lates wireless-2.6.git tree you have.
#
# Usage: you should have the latest pull of wireless-2.6.git
# git://git.kernel.org/pub/scm/linux/kernel/git/linville/wireless-2.6.git
# We assume you have it on your ~/devel/wireless-2.6/ directory. If you do,
# just run this script from the compat-wireless-2.6 directory.

INCLUDE_LINUX="ieee80211.h nl80211.h wireless.h pci_ids.h bitops.h"

INCLUDE_NET="cfg80211.h ieee80211_radiotap.h iw_handler.h"
INCLUDE_NET="$INCLUDE_NET mac80211.h wext.h wireless.h"

NET_DIRS="wireless mac80211"
GIT_TREE="/home/$USER/devel/wireless-2.6"

DRIVERS="drivers/net/wireless/ath5k"
DRIVERS="$DRIVERS drivers/ssb drivers/net/wireless/b43"
DRIVERS="$DRIVERS drivers/net/wireless/iwlwifi"
DRIVERS="$DRIVERS drivers/net/wireless/zd1211rw-mac80211"

mkdir -p include/linux/ include/net/ \
	net/mac80211/ net/wireless/ \
	drivers/ssb/ \
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

# net/wireless and net/mac80211
for i in $NET_DIRS; do
	echo "Copying $GIT_TREE/net/$i/*.[ch]"
	cp $GIT_TREE/net/$i/*.[ch] net/$i/
	cp $GIT_TREE/net/$i/Makefile net/$i/
	rm -f net/$i/*.mod.c
done

# drivers
for i in $DRIVERS; do
	mkdir -p $i
	cp $GIT_TREE/$i/*.[ch] $i/
	cp $GIT_TREE/$i/Makefile $i/
	rm -f net/$i/*.mod.c
done

# Compat stuff
cp compat/compat.c net/mac80211/
cp compat/compat.h include/net/

patch -p1 < compat/compat.diff
DIR="$PWD"
cd $GIT_TREE && git-describe > $DIR/git-describe && cd $DIR
echo "Updated ${GIT_TREE##*/}, git-describe says:"
cat git-describe
if [ -d ./.git ]; then
	git-describe > compat-release
fi
echo "This is compat-release:"
cat compat-release
