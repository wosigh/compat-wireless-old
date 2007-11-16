/*
 * Copyright 2007	Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Compatibility file for Linux wireless for kernels 2.6.22 - tip
 * The headers don't need to be modified as we're simply adding them.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/netpoll.h>
#include <linux/rtnetlink.h>
#include <linux/audit.h>
#include <linux/workqueue.h>
#include <linux/pci.h>
#include <net/arp.h>
#include <net/compat.h>

/* All things not in 2.6.22 and 2.6.23 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

/* Part of net/ethernet/eth.c as of 2.6.24 */
char *print_mac(char *buf, const u8 *addr)
{
	sprintf(buf, MAC_FMT,
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
	return buf;
}
EXPORT_SYMBOL(print_mac);

/* On net/core/dev.c as of 2.6.24 */
int __dev_addr_delete(struct dev_addr_list **list, int *count,
                      void *addr, int alen, int glbl)
{
	struct dev_addr_list *da;

	for (; (da = *list) != NULL; list = &da->next) {
		if (memcmp(da->da_addr, addr, da->da_addrlen) == 0 &&
			alen == da->da_addrlen) {
			if (glbl) {
				int old_glbl = da->da_gusers;
				da->da_gusers = 0;
				if (old_glbl == 0)
					break;
			}
			if (--da->da_users)
				return 0;

			*list = da->next;
			kfree(da);
			(*count)--;
			return 0;
		}
	}
	return -ENOENT;
}

/* On net/core/dev.c as of 2.6.24. This is not yet used by mac80211 but
 * might as well add it */
int __dev_addr_add(struct dev_addr_list **list, int *count,
                   void *addr, int alen, int glbl)
{
	struct dev_addr_list *da;

	for (da = *list; da != NULL; da = da->next) {
		if (memcmp(da->da_addr, addr, da->da_addrlen) == 0 &&
			da->da_addrlen == alen) {
			if (glbl) {
				int old_glbl = da->da_gusers;
				da->da_gusers = 1;
				if (old_glbl)
					return 0;
			}
			da->da_users++;
			return 0;
		}
	}

	da = kmalloc(sizeof(*da), GFP_ATOMIC);
	if (da == NULL)
		return -ENOMEM;
	memcpy(da->da_addr, addr, alen);
	da->da_addrlen = alen;
	da->da_users = 1;
	da->da_gusers = glbl ? 1 : 0;
	da->next = *list;
	*list = da;
	(*count)++;
	return 0;
}

/* 2.6.22 and 2.6.23 have eth_header_cache_update defined as extern in include/linux/etherdevice.h
 * and actually defined in net/ethernet/eth.c but 2.6.24 exports it. Lets export it here */

/**
 * eth_header_cache_update - update cache entry
 * @hh: destination cache entry
 * @dev: network device
 * @haddr: new hardware address
 *
 * Called by Address Resolution module to notify changes in address.
 */
void eth_header_cache_update(struct hh_cache *hh,
                             struct net_device *dev,
                             unsigned char *haddr)
{
	memcpy(((u8 *) hh->hh_data) + HH_DATA_OFF(sizeof(struct ethhdr)),
		haddr, ETH_ALEN);
}
EXPORT_SYMBOL(eth_header_cache_update);

/* 2.6.22 and 2.6.23 have eth_header_cache defined as extern in include/linux/etherdevice.h
 * and actually defined in net/ethernet/eth.c but 2.6.24 exports it. Lets export it here */

/**
 * eth_header_cache - fill cache entry from neighbour
 * @neigh: source neighbour
 * @hh: destination cache entry
 * Create an Ethernet header template from the neighbour.
 */
int eth_header_cache(struct neighbour *neigh, struct hh_cache *hh)
{
	__be16 type = hh->hh_type;
	struct ethhdr *eth;
	const struct net_device *dev = neigh->dev;

	eth = (struct ethhdr *)
	    (((u8 *) hh->hh_data) + (HH_DATA_OFF(sizeof(*eth))));

	if (type == htons(ETH_P_802_3))
		return -1;

	eth->h_proto = type;
	memcpy(eth->h_source, dev->dev_addr, ETH_ALEN);
	memcpy(eth->h_dest, neigh->ha, ETH_ALEN);
	hh->hh_len = ETH_HLEN;
	return 0;
}
EXPORT_SYMBOL(eth_header_cache);

/* 2.6.22 and 2.6.23 have eth_header() defined as extern in include/linux/etherdevice.h
 * and actually defined in net/ethernet/eth.c but 2.6.24 exports it. Lets export it here */

/**
 * eth_header - create the Ethernet header
 * @skb:	buffer to alter
 * @dev:	source device
 * @type:	Ethernet type field
 * @daddr: destination address (NULL leave destination address)
 * @saddr: source address (NULL use device source address)
 * @len:   packet length (<= skb->len)
 *
 *
 * Set the protocol type. For a packet of type ETH_P_802_3 we put the length
 * in here instead. It is up to the 802.2 layer to carry protocol information.
 */
int eth_header(struct sk_buff *skb, struct net_device *dev, unsigned short type,
	       void *daddr, void *saddr, unsigned len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	if (type != ETH_P_802_3)
		eth->h_proto = htons(type);
	else
		eth->h_proto = htons(len);

	/*
	 *      Set the source hardware address.
	 */

	if (!saddr)
		saddr = dev->dev_addr;
	memcpy(eth->h_source, saddr, dev->addr_len);

	if (daddr) {
		memcpy(eth->h_dest, daddr, dev->addr_len);
		return ETH_HLEN;
	}

	/*
	 *      Anyway, the loopback-device should never use this function...
	 */

	if (dev->flags & (IFF_LOOPBACK | IFF_NOARP)) {
		memset(eth->h_dest, 0, dev->addr_len);
		return ETH_HLEN;
	}

	return -ETH_HLEN;
}

EXPORT_SYMBOL(eth_header);

/* 2.6.22 and 2.6.23 have eth_rebuild_header defined as extern in include/linux/etherdevice.h
 * and actually defined in net/ethernet/eth.c but 2.6.24 exports it. Lets export it here */

/**
 * eth_rebuild_header- rebuild the Ethernet MAC header.
 * @skb: socket buffer to update
 *
 * This is called after an ARP or IPV6 ndisc it's resolution on this
 * sk_buff. We now let protocol (ARP) fill in the other fields.
 *
 * This routine CANNOT use cached dst->neigh!
 * Really, it is used only when dst->neigh is wrong.
 */
int eth_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *)skb->data;
	struct net_device *dev = skb->dev;

	switch (eth->h_proto) {
#ifdef CONFIG_INET
	case __constant_htons(ETH_P_IP):
		return arp_find(eth->h_dest, skb);
#endif
	default:
		printk(KERN_DEBUG
		       "%s: unable to resolve type %X addresses.\n",
		       dev->name, (int)eth->h_proto);

		memcpy(eth->h_source, dev->dev_addr, ETH_ALEN);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(eth_rebuild_header);

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24) */

/* All things not in 2.6.22 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))

/* Part of net/core/dev_mcast.c as of 2.6.23. This is a slightly different version.
 * Since da->da_synced is not part of 2.6.22 we need to take longer route when 
 * syncing */

/**
 *	dev_mc_sync	- Synchronize device's multicast list to another device
 *	@to: destination device
 *	@from: source device
 *
 * 	Add newly added addresses to the destination device and release
 * 	addresses that have no users left. The source device must be
 * 	locked by netif_tx_lock_bh.
 *
 *	This function is intended to be called from the dev->set_multicast_list
 *	function of layered software devices.
 */
int dev_mc_sync(struct net_device *to, struct net_device *from)
{
	struct dev_addr_list *da, *next, *da_to;
	int err = 0;

	netif_tx_lock_bh(to);
	da = from->mc_list;
	while (da != NULL) {
		int synced = 0;
		next = da->next;
		da_to = to->mc_list;
		/* 2.6.22 does not have da->da_synced so lets take the long route */
		while (da_to != NULL) {
			if (memcmp(da_to->da_addr, da->da_addr, da_to->da_addrlen) == 0 &&
				da->da_addrlen == da_to->da_addrlen)
				synced = 1;
				break;
		}
		if (!synced) {
			err = __dev_addr_add(&to->mc_list, &to->mc_count,
					     da->da_addr, da->da_addrlen, 0);
			if (err < 0)
				break;
			da->da_users++;
		} else if (da->da_users == 1) {
			__dev_addr_delete(&to->mc_list, &to->mc_count,
					  da->da_addr, da->da_addrlen, 0);
			__dev_addr_delete(&from->mc_list, &from->mc_count,
					  da->da_addr, da->da_addrlen, 0);
		}
		da = next;
	}
	if (!err)
		__dev_set_rx_mode(to);
	netif_tx_unlock_bh(to);

	return err;
}
EXPORT_SYMBOL(dev_mc_sync);


/* Part of net/core/dev_mcast.c as of 2.6.23. This is a slighty different version. 
 * Since da->da_synced is not part of 2.6.22 we need to take longer route when 
 * unsyncing */

/**
 *      dev_mc_unsync   - Remove synchronized addresses from the destination
 *			  device
 *	@to: destination device
 *	@from: source device
 *
 *	Remove all addresses that were added to the destination device by
 *	dev_mc_sync(). This function is intended to be called from the
 *	dev->stop function of layered software devices.
 */
void dev_mc_unsync(struct net_device *to, struct net_device *from)
{
	struct dev_addr_list *da, *next, *da_to;

	netif_tx_lock_bh(from);
	netif_tx_lock_bh(to);

	da = from->mc_list;
	while (da != NULL) {
		bool synced = false;
		next = da->next;
		da_to = to->mc_list;
		/* 2.6.22 does not have da->da_synced so lets take the long route */
		while (da_to != NULL) {
			if (memcmp(da_to->da_addr, da->da_addr, da_to->da_addrlen) == 0 &&
				da->da_addrlen == da_to->da_addrlen)
				synced = true;
				break;
		}
		if (!synced) {
			da = next;
			continue;
		}
		__dev_addr_delete(&to->mc_list, &to->mc_count,
			da->da_addr, da->da_addrlen, 0);
		__dev_addr_delete(&from->mc_list, &from->mc_count,
			da->da_addr, da->da_addrlen, 0);
		da = next;
	}
	__dev_set_rx_mode(to);

	netif_tx_unlock_bh(to);
	netif_tx_unlock_bh(from);
}
EXPORT_SYMBOL(dev_mc_unsync);

/* Added as of 2.6.23 on net/core/dev.c. Slightly modifed, no dev->set_rx_mode on
 * 2.6.22 so ignore that. */

/*
 *	Upload unicast and multicast address lists to device and
 *	configure RX filtering. When the device doesn't support unicast
 *	filtering it is put in promiscous mode while unicast addresses
 *	are present.
 */
void __dev_set_rx_mode(struct net_device *dev)
{
	/* dev_open will call this function so the list will stay sane. */
	if (!(dev->flags&IFF_UP))
		return;

	if (!netif_device_present(dev))
		return;

/* This needs to be ported to 2.6.22 framework */
#if 0
	/* Unicast addresses changes may only happen under the rtnl,
	 * therefore calling __dev_set_promiscuity here is safe.
	 */
	if (dev->uc_count > 0 && !dev->uc_promisc) {
		__dev_set_promiscuity(dev, 1);
		dev->uc_promisc = 1;
	} else if (dev->uc_count == 0 && dev->uc_promisc) {
		__dev_set_promiscuity(dev, -1);
		dev->uc_promisc = 0;
	}
#endif

	if (dev->set_multicast_list)
		dev->set_multicast_list(dev);
}

#ifdef PCI_DISABLE_MWI
int pci_try_set_mwi(struct pci_dev *dev)
{
	return 0;
}
EXPORT_SYMBOL(pci_try_set_mwi);
#else
/**
 * pci_try_set_mwi - enables memory-write-invalidate PCI transaction
 * @dev: the PCI device for which MWI is enabled
 *
 * Enables the Memory-Write-Invalidate transaction in %PCI_COMMAND.
 * Callers are not required to check the return value.
 *
 * RETURNS: An appropriate -ERRNO error value on error, or zero for success.
 */
int pci_try_set_mwi(struct pci_dev *dev)
{
	int rc = pci_set_mwi(dev);
	return rc;
}
EXPORT_SYMBOL(pci_try_set_mwi);
#endif


#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)) */

