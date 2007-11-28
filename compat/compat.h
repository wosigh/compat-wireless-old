#ifndef LINUX_26_COMPAT_H
#define LINUX_26_COMPAT_H

#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/genetlink.h>
#include <net/neighbour.h>
#include <linux/version.h>
#include <linux/scatterlist.h>

/* So all *.[ch] can pick up the options as if defined 
 * by the kernel's .config. */

/* XXX: See if we can do something better about this and config.mk */
#define CONFIG_MAC80211			1
#define CONFIG_MAC80211_RCSIMPLE	1
#define CONFIG_CFG80211			1
#undef CONFIG_NL80211			
#define CONFIG_ATH5K			1
#define CONFIG_IWL3945			1
#define CONFIG_IWL4965			1
#define CONFIG_ZD1211RW_MAC80211	1
#if 1
#define CONFIG_B43
//#define CONFIG_B43_RFKILL		1
//#define CONFIG_B43_LEDS			1
#define CONFIG_B43_PCMCIA		1
//#define CONFIG_B43_DEBUG		1
#define CONFIG_B43_DMA			1
#define CONFIG_B43_PIO			1
#define CONFIG_B43LEGACY_PCI_AUTOSELECT	1
#define CONFIG_B43LEGACY_PCICORE_AUTOSELECT	1
#define CONFIG_B43LEGACY_DMA		1
#define CONFIG_B43LEGACY_PIO		1
#define CONFIG_B43LEGACY_DMA_AND_PIO_MODE	1

#define CONFIG_SSB			1
#define CONFIG_SSB_PCIHOST		1
#define CONFIG_SSB_PCMCIAHOST		1
#undef CONFIG_SSB_DRIVER_MIPS
//#define CONFIG_SSB_DRIVER_EXTIF		1
#define CONFIG_SSB_DRIVER_PCICORE	1
#define CONFIG_SSB_PCIHOST		1
/* For mips */
#undef CONFIG_SSB_PCICORE_HOSTMODE
#endif

#define CONFIG_RT2X00_LIB_FIRMWARE	1

#define CONFIG_IEEE80211		1
#define CONFIG_IEEE80211_CRYPT_CCMP	1
#define CONFIG_IEEE80211_CRYPT_TKIP	1
#define CONFIG_IEEE80211_CRYPT_WEP	1
/* Die old softmac, die ! Bleed! */
#undef CONFIG_IEEE80211_SOFTMAC

/* Compat work for 2.6.22 and 2.6.23 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

/* From include/linux/mod_devicetable.h */

/* SSB core, see drivers/ssb/ */
struct ssb_device_id {
	__u16   vendor;
	__u16   coreid;
	__u8    revision;
};
#define SSB_DEVICE(_vendor, _coreid, _revision)  \
	{ .vendor = _vendor, .coreid = _coreid, .revision = _revision, }
#define SSB_DEVTABLE_END  \
	{ 0, },

#define SSB_ANY_VENDOR          0xFFFF
#define SSB_ANY_ID              0xFFFF
#define SSB_ANY_REV             0xFF


/* Namespace stuff, introduced on 2.6.24 */
#define dev_get_by_index(a, b)		dev_get_by_index(b)
#define __dev_get_by_index(a, b)	__dev_get_by_index(b)

/*
 * Display a 6 byte device address (MAC) in a readable format.
 */
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
extern char *print_mac(char *buf, const u8 *addr);
#define DECLARE_MAC_BUF(var) char var[18] __maybe_unused

extern int		eth_header(struct sk_buff *skb, struct net_device *dev,
				unsigned short type, void *daddr,
				void *saddr, unsigned len);
extern int		eth_rebuild_header(struct sk_buff *skb);
extern void		eth_header_cache_update(struct hh_cache *hh, struct net_device *dev,
				unsigned char * haddr);
extern int		eth_header_cache(struct neighbour *neigh,
			struct hh_cache *hh);

/* This structure is simply not present on 2.6.22 and 2.6.23 */
struct header_ops {
	int     (*create) (struct sk_buff *skb, struct net_device *dev,
		unsigned short type, void *daddr,
		void *saddr, unsigned len);
	int     (*parse)(const struct sk_buff *skb, unsigned char *haddr);
	int     (*rebuild)(struct sk_buff *skb);
	#define HAVE_HEADER_CACHE
	int     (*cache)(struct neighbour *neigh, struct hh_cache *hh);
	void    (*cache_update)(struct hh_cache *hh,
		struct net_device *dev,
		unsigned char *haddr);
};

/* net/ieee80211/ieee80211_crypt_tkip uses sg_init_table. This was added on 
 * 2.6.24. CONFIG_DEBUG_SG was added in 2.6.24 as well, so lets just ignore
 * the debug stuff. Note that adding this required changes to the struct
 * scatterlist on include/asm/scatterlist*, so the right way to port this
 * is to simply ignore the new structure changes and zero the scatterlist
 * array. We lave the kdoc intact for reference.
 */

/**
 * sg_mark_end - Mark the end of the scatterlist
 * @sg:          SG entryScatterlist
 *
 * Description:
 *   Marks the passed in sg entry as the termination point for the sg
 *   table. A call to sg_next() on this entry will return NULL.
 *
 **/
static inline void sg_mark_end(struct scatterlist *sg)
{
}

/**
 * sg_init_table - Initialize SG table
 * @sgl:           The SG table
 * @nents:         Number of entries in table
 *
 * Notes:
 *   If this is part of a chained sg table, sg_mark_end() should be
 *   used only on the last table part.
 *
 **/
static inline void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
	memset(sgl, 0, sizeof(*sgl) * nents);
}

#endif

/* Compat work for 2.6.22 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))

/* dev_mc_list was replaced with dev_addr_list as of 2.6.23 */
#define dev_addr_list	dev_mc_list
#define da_addr		dmi_addr
#define da_addrlen	dmi_addrlen
#define da_users	dmi_users
#define da_gusers	dmi_gusers

/* dev_set_promiscuity() was moved to __dev_set_promiscuity() on 2.6.23 and 
 * dev_set_promiscuity() became a wrapper. */
#define __dev_set_promiscuity dev_set_promiscuity

/* Our own 2.6.22 port on compat.c */
extern void	dev_mc_unsync(struct net_device *to, struct net_device *from);
extern int	dev_mc_sync(struct net_device *to, struct net_device *from);

/* Our own 2.6.22 port on compat.c */
extern void	__dev_set_rx_mode(struct net_device *dev);

/* Simple to add this */
extern int cancel_delayed_work_sync(struct delayed_work *work);

#define cancel_delayed_work_sync cancel_rearming_delayed_work

#define debugfs_rename(a, b, c, d) 1

/* Eh we need a lot more than this to complete this support. Not
 * a priority for now. This is for nl80211 support */

/**
 * struct genl_multicast_group - generic netlink multicast group
 * @name: name of the multicast group, names are per-family
 * @id: multicast group ID, assigned by the core, to use with
 * 	genlmsg_multicast().
 * @list: list entry for linking
 * @family: pointer to family, need not be set before registering
 */
struct genl_multicast_group
{
	struct genl_family      *family;        /* private */
	struct list_head        list;           /* private */
	char                    name[GENL_NAMSIZ];
	u32                     id;
};

/* Added as of 2.6.23 */
int pci_try_set_mwi(struct pci_dev *dev);

/* Added as of 2.6.23 */
#ifdef CONFIG_PM_SLEEP
/*
 * Tell the freezer that the current task should be frozen by it
 */
static inline void set_freezable(void)
{
	current->flags &= ~PF_NOFREEZE;
}

#else
static inline void set_freezable(void) {}
#endif

#endif

#endif
