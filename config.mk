export

## NOTE
## Make sure to have each variable declaration start
## in the first column, no whitespace allowed.

ifeq ($(wildcard $(KLIB_BUILD)/.config),)
# These will be ignored by compat autoconf
 CONFIG_PCI=y
 CONFIG_USB=n
 CONFIG_PCMCIA=n
else
include $(KLIB_BUILD)/.config
endif

CONFIG_SD8XXX=m
