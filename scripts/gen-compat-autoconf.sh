#!/bin/bash
# 
# Copyright 2007	Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
#
# Use this to parse a small .config equivalent looking file to generate
# our own autoconf.h. This file has defines for each config option
# just like the kernels include/linux/autoconf.h
#
# XXX: consider using scripts/kconfig/confdata.c instead.
# On the downside this would require the user to have libc though.

# This indicates which is the oldest kernel we support
# Update this if you are adding support for older kernels.
OLDEST_KERNEL_SUPPORTED="2.6.22"
COMPAT_RELEASE="compat-release"
KERNEL_RELEASE="git-describe"

if [ $# -ne 1 ]; then
	echo "Usage $0 config-file"
	exit
fi

COMPAT_CONFIG="$1"

if [ ! -f $COMPAT_CONFIG ]; then
	echo "File $1 is not a file"
	exit
fi

if [ ! -f $COMPAT_RELEASE  -o ! -f $KERNEL_RELEASE ]; then
	echo "Error: $COMPAT_RELEASE or $KERNEL_RELEASE file is missing"
	exit
fi

CREL=$(cat $COMPAT_RELEASE)
KREL=$(cat $KERNEL_RELEASE)
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
 * linux-2.6: $KREL
 */
#define COMPAT_RELEASE "$CREL"
#define COMPAT_KERNEL_RELEASE "$KREL"
EOF

# Checks user is compiling against a kernel we support
kernel_version_req $OLDEST_KERNEL_SUPPORTED

# Handle core kernel wireless depenencies here
define_config_req CONFIG_WIRELESS_EXT

# For each CONFIG_FOO=x option
for i in $(grep -v ^# $COMPAT_CONFIG | grep ^CONFIG_); do
	# Get the element on the left of the "="
	VAR=$(echo $i | cut -d"=" -f 1)
	# Get the element on the right of the "="
	VALUE=$(echo $i | cut -d"=" -f 2)

	# Handle core kernel module depenencies here.
	case $VAR in
	CONFIG_USB_NET_RNDIS_WLAN)
		define_config_dep $VAR $VALUE CONFIG_USB_NET_CDCETHER
		continue
		;;
	CONFIG_USB_NET_RNDIS_HOST)
		define_config_dep $VAR $VALUE CONFIG_USB_NET_CDCETHER
		continue
	esac
	# Any other module which can *definitely* be built as a module goes here
	define_config $VAR $VALUE
done

echo "#endif /* COMPAT_AUTOCONF_INCLUDED */"
