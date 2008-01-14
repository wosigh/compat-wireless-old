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

# For each CONFIG_FOO=x option
for i in $(grep -v ^# $COMPAT_CONFIG | grep ^CONFIG_); do
	# Get the element on the left of the "="
	VAR=$(echo $i | cut -d"=" -f 1)
	# Get the element on the right of the "="
	VALUE=$(echo $i | cut -d"=" -f 2)
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
done

echo "#endif /* COMPAT_AUTOCONF_INCLUDED */"
