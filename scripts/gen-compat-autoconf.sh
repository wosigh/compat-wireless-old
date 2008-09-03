#!/bin/bash
# 
# Copyright 2007, 2008	Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
#
# Use this to parse a small .config equivalent looking file to generate
# our own autoconf.h. This file has defines for each config option
# just like the kernels include/linux/autoconf.h
#
# XXX: consider using scripts/kconfig/confdata.c instead.
# On the downside this would require the user to have libc though.

# This indicates which is the oldest kernel we support
# Update this if you are adding support for older kernels.
OLDEST_KERNEL_SUPPORTED="2.6.21"
COMPAT_RELEASE="compat-release"
KERNEL_RELEASE="git-describe"
MULT_DEP_FILE=".compat_pivot_dep"

if [ $# -ne 1 ]; then
	echo "Usage $0 config-file"
	exit
fi

COMPAT_CONFIG="$1"

if [ ! -f $COMPAT_CONFIG ]; then
	echo "File $1 is not a file"
	exit
fi

if [ ! -f $COMPAT_RELEASE ]; then
	which git 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		echo "You need git to generate $COMPAT_RELEASE file"
		exit
	fi
	git describe  > $COMPAT_RELEASE
fi

CREL=$(cat $COMPAT_RELEASE)
DATE=$(date)

# Defines a CONFIG_ option if not defined yet, this helps respect
# linux/autoconf.h 
function define_config {
	VAR=$1	
	VALUE=$2
	echo "#ifndef $VAR"
	case $VALUE in
	n) # Do nothing
		;;
	y)
		echo "#define $VAR 1"
		;;
	m)
		echo "#define $VAR 1"
		;;
	*) # Assume string
		# XXX: add better checks to make sure what was on
		# the right was indeed a string
		echo "#define $VAR \"$VALUE\""
		;;
	esac
	echo "#endif /* $VAR */ "
}

# This deals with core compat-wireless kernel requirements.
function define_config_req {
	VAR=$1
	echo "#ifndef $VAR"
	echo -n "#error Compat-wireless requirement: $VAR must be enabled "
	echo "in your kernel"
	echo "#endif /* $VAR */"
}

# This handles modules which have dependencies from the kernel
# which compat-wireless isn't providing yet either because
# the dependency is not available as kernel module or
# the module simply isn't provided by compat-wireless.
function define_config_dep {
	VAR=$1
	VALUE=$2
	DEP=$3
	WARN_VAR="COMPAT_WARN_$VAR"
	echo "#ifdef $DEP"
	define_config $VAR $VALUE
	echo "#else"
	# XXX: figure out a way to warn only once
	# define only once in case user tried to enable config option
	# twice in config.mk
	echo "#ifndef $WARN_VAR"
	# Lets skip these for now.. they might be too annoying
	#echo "#warning Skipping $VAR as $DEP was needed... "
	#echo "#warning This just means $VAR won't be built and is not fatal."
	echo "#define $WARN_VAR"
	echo "#endif /* $VAR */"
	echo "#endif /* $WARN_VAR */"
}

# This handles options which have *multiple* dependencies from the kernel
function define_config_multiple_deps {
	VAR=$1
	VALUE=$2
	DEP_ARRAY=$3

	# First, put all ifdefs
	for i in $(cat $MULT_DEP_FILE); do
		echo "#ifdef $i"
	done

	# Now put our option in the middle
	define_config $VAR $VALUE

	# Now close all ifdefs
	# First, put all ifdefs
	for i in $(cat $MULT_DEP_FILE); do
		echo "#endif"
	done

}

function kernel_version_req {
	VERSION=$(echo $1 | sed -e 's/\./,/g')
	echo "#if (LINUX_VERSION_CODE < KERNEL_VERSION($VERSION))"
	echo "#error Compat-wireless requirement: Linux >= $VERSION"
	echo "#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION($VERSION) */ "
}

cat <<EOF
#ifndef COMPAT_AUTOCONF_INCLUDED
#define COMPAT_AUTOCONF_INCLUDED
/*
 * Automatically generated C config: don't edit
 * $DATE 
 * compat-wireless-2.6: $CREL
 */
#define COMPAT_RELEASE "$CREL"
EOF

# Checks user is compiling against a kernel we support
kernel_version_req $OLDEST_KERNEL_SUPPORTED

# Handle core kernel wireless depenencies here
define_config_req CONFIG_WIRELESS_EXT

# For each CONFIG_FOO=x option
for i in $(grep '^CONFIG_' $COMPAT_CONFIG); do
	# Get the element on the left of the "="
	VAR=$(echo $i | cut -d"=" -f 1)
	# Get the element on the right of the "="
	VALUE=$(echo $i | cut -d"=" -f 2)

	# skip vars that weren't actually set due to dependencies
	#if [ "${!VAR}" = "" ] ; then
	#	continue
	#fi

	# Handle core kernel module depenencies here.
	case $VAR in
	CONFIG_USB_NET_RNDIS_WLAN)
		define_config_dep $VAR $VALUE CONFIG_USB_NET_CDCETHER
		continue
		;;
	CONFIG_USB_NET_RNDIS_HOST)
		define_config_dep $VAR $VALUE CONFIG_USB_NET_CDCETHER
		continue
		;;
	# ignore this, we have a special hanlder for this at the botttom
	# instead. We still need to keep this in config.mk to let Makefiles
	# know its enabled so just ignore it here.
	CONFIG_MAC80211_QOS)
		continue
		;;
	esac
	# Any other module which can *definitely* be built as a module goes here
	define_config $VAR $VALUE
done

# Deal with special cases. CONFIG_MAC80211_QOS is such a case.
# We handle this specially for different kernels we support.
if [ -f $KLIB_BUILD/Makefile ]; then
	SUBLEVEL=$(make -C $KLIB_BUILD kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
	if [ $SUBLEVEL -le 22 ]; then
		define_config CONFIG_MAC80211_QOS y
	else # kernel >= 2.6.23
		# CONFIG_MAC80211_QOS on these kernels requires
		# CONFIG_NET_SCHED and CONFIG_NETDEVICES_MULTIQUEUE
		rm -f $MULT_DEP_FILE
		echo CONFIG_NET_SCHED >> $MULT_DEP_FILE
		echo CONFIG_NETDEVICES_MULTIQUEUE >> $MULT_DEP_FILE
		define_config_multiple_deps CONFIG_MAC80211_QOS y $ALL_DEPS
		rm -f $MULT_DEP_FILE
	fi
fi
echo "#endif /* COMPAT_AUTOCONF_INCLUDED */"
