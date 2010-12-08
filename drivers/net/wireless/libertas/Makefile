libertas-objs := main.o wext.o rx.o tx.o cmd.o cmdresp.o scan.o 11d.o	\
		 debugfs.o persistcfg.o ethtool.o assoc.o

usb8xxx-objs += if_usb.o
libertas_cs-objs += if_cs.o
sd8xxx-objs += if_sdio.o

obj-$(CONFIG_LIBERTAS)     += libertas.o
obj-$(CONFIG_LIBERTAS_USB) += usb8xxx.o
obj-$(CONFIG_LIBERTAS_CS)  += libertas_cs.o
obj-$(CONFIG_LIBERTAS_SDIO) += sd8xxx.o
