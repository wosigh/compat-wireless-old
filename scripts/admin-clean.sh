#!/bin/bash
make clean
rm -rf mac80211 wireless drivers include Module.symvers git-describe
echo "Cleaned wireless-compat-2.6"
