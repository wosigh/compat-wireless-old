/*
 * Atheros 11n USB driver
 *
 * Hardware and mac80211 interaction code
 *
 * Copyright 2008, Johannes Berg <johannes@sipsolutions.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, see
 * http://www.gnu.org/licenses/.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *    Copyright (c) 2007-2008 Atheros Communications, Inc.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <net/mac80211.h>
#include "ar9170.h"
#include "hw.h"

MODULE_AUTHOR("Johannes Berg <johannes@sipsolutions.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Atheros 802.11n USB wireless");
MODULE_FIRMWARE("ar9170-1.fw");
MODULE_FIRMWARE("ar9170-2.fw");

static struct usb_device_id ar9170_ids[] = {
	/* Atheros 9170 */
	{ USB_DEVICE(0x0cf3, 0x9170) },
	/* Atheros TG121N */
	{ USB_DEVICE(0x0cf3, 0x1001) },
	/* D-Link DWA 160A */
	{ USB_DEVICE(0x07d1, 0x3c10) },
	/* Netgear WNDA3100 */
	{ USB_DEVICE(0x0846, 0x9010) },
	/* Netgear WN111 v2 */
	{ USB_DEVICE(0x0846, 0x9001) },
	/* Zydas ZD1221 */
	{ USB_DEVICE(0x0ace, 0x1221) },
	/* Z-Com UB81 BG */
	{ USB_DEVICE(0x0cde, 0x0023) },
	/* Z-Com UB82 ABG */
	{ USB_DEVICE(0x0cde, 0x0026) },
	/* Arcadyan WN7512 */
	{ USB_DEVICE(0x083a, 0xf522) },
	/* Planex GWUS300 */
	{ USB_DEVICE(0x2019, 0x5304) },
	/* IO-Data WNGDNUS2 */
	{ USB_DEVICE(0x04bb, 0x093f) },

	/* terminate */
	{}
};
MODULE_DEVICE_TABLE(usb, ar9170_ids);

#define RATE(_bitrate, _hw_rate, _txpidx, _flags) {	\
	.bitrate	= (_bitrate),			\
	.flags		= (_flags),			\
	.hw_value	= (_hw_rate) | (_txpidx) << 4,	\
}

static struct ieee80211_rate __ar9170_ratetable[] = {
	RATE(10, 0, 0, 0),
	RATE(20, 1, 1, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(55, 2, 2, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(110, 3, 3, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(60, 0xb, 0, 0),
	RATE(90, 0xf, 0, 0),
	RATE(120, 0xa, 0, 0),
	RATE(180, 0xe, 0, 0),
	RATE(240, 0x9, 0, 0),
	RATE(360, 0xd, 1, 0),
	RATE(480, 0x8, 2, 0),
	RATE(540, 0xc, 3, 0),
};
#undef RATE

#define ar9170_g_ratetable	(__ar9170_ratetable + 0)
#define ar9170_g_ratetable_size	12
#define ar9170_a_ratetable	(__ar9170_ratetable + 4)
#define ar9170_a_ratetable_size	8

/*
 * NB: The hw_value is used as an index into the ar9170_phy_freq_params
 *     array in phy.c so that we don't have to do frequency lookups!
 */
#define CHAN(_freq, _idx) {		\
	.center_freq	= (_freq),	\
	.hw_value	= (_idx),	\
	.max_power	= 30, /* XXX */	\
}

static struct ieee80211_channel ar9170_2ghz_chantable[] = {
	CHAN(2412,  0),
	CHAN(2417,  1),
	CHAN(2422,  2),
	CHAN(2427,  3),
	CHAN(2432,  4),
	CHAN(2437,  5),
	CHAN(2442,  6),
	CHAN(2447,  7),
	CHAN(2452,  8),
	CHAN(2457,  9),
	CHAN(2462, 10),
	CHAN(2467, 11),
	CHAN(2472, 12),
	CHAN(2484, 13),
};

static struct ieee80211_channel ar9170_5ghz_chantable[] = {
	CHAN(4920, 14),
	CHAN(4940, 15),
	CHAN(4960, 16),
	CHAN(4980, 17),
	CHAN(5040, 18),
	CHAN(5060, 19),
	CHAN(5080, 20),
	CHAN(5180, 21),
	CHAN(5200, 22),
	CHAN(5220, 23),
	CHAN(5240, 24),
	CHAN(5260, 25),
	CHAN(5280, 26),
	CHAN(5300, 27),
	CHAN(5320, 28),
	CHAN(5500, 29),
	CHAN(5520, 30),
	CHAN(5540, 31),
	CHAN(5560, 32),
	CHAN(5580, 33),
	CHAN(5600, 34),
	CHAN(5620, 35),
	CHAN(5640, 36),
	CHAN(5660, 37),
	CHAN(5680, 38),
	CHAN(5700, 39),
	CHAN(5745, 40),
	CHAN(5765, 41),
	CHAN(5785, 42),
	CHAN(5805, 43),
	CHAN(5825, 44),
	CHAN(5170, 45),
	CHAN(5190, 46),
	CHAN(5210, 47),
	CHAN(5230, 48),
};
#undef CHAN

static struct ieee80211_supported_band ar9170_band_2GHz = {
	.channels	= ar9170_2ghz_chantable,
	.n_channels	= ARRAY_SIZE(ar9170_2ghz_chantable),
	.bitrates	= ar9170_g_ratetable,
	.n_bitrates	= ar9170_g_ratetable_size,
};

static struct ieee80211_supported_band ar9170_band_5GHz = {
	.channels	= ar9170_5ghz_chantable,
	.n_channels	= ARRAY_SIZE(ar9170_5ghz_chantable),
	.bitrates	= ar9170_a_ratetable,
	.n_bitrates	= ar9170_a_ratetable_size,
};


static int ar9170_op_start(struct ieee80211_hw *hw)
{
	struct ar9170 *ar = hw->priv;
	int err;

	mutex_lock(&ar->mutex);

	err = ar9170_init_mac(ar);
	if (err)
		goto out;

	err = ar9170_init_phy(ar, IEEE80211_BAND_2GHZ);
	if (err)
		goto out;

	err = ar9170_init_rf(ar);
	if (err)
		goto out;

	/* start DMA */
	err = ar9170_write_reg(ar, 0x1c3d30, 0x100);
	if (err)
		goto out;

	ar->started = true;
 out:
	mutex_unlock(&ar->mutex);

	return err;
}

static void ar9170_op_stop(struct ieee80211_hw *hw)
{
	struct ar9170 *ar = hw->priv;

	mutex_lock(&ar->mutex);

	/* stop DMA */
	ar9170_write_reg(ar, 0x1c3d30, 0);

	ar->started = false;

	mutex_unlock(&ar->mutex);
}

#if 0
static inline void ar9170_add_tx_status_skb(struct ar9170 *ar,
					    struct sk_buff *skb)
{
	skb->next = NULL;

	if (ar->tx_status_queue_tail) {
		ar->tx_status_queue_tail->next = skb;
		ar->tx_status_queue_tail = skb;
	} else {
		ar->tx_status_queue_tail = skb;
		ar->tx_status_queue_head = skb;
	}
}

static inline struct sk_buff *ar9170_get_tx_status_skb(struct ar9170 *ar)
{
	struct sk_buff *skb;

	skb = ar->tx_status_queue_head;

	if (skb) {
		ar->tx_status_queue_head = skb->next;
		if (ar->tx_status_queue_tail == skb)
			ar->tx_status_queue_tail = NULL;
	}

	return skb;
}
#endif

static void tx_urb_complete(struct urb *urb)
{
	struct sk_buff *skb = (void *)urb->context;
/*
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ar9170 *ar = 
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
                            info->driver_data[0];
#else
                            info->rate_driver_data[0];
#endif
 */

	/*
	 * For proper rate control we really need to provide
	 * TX status callbacks. For now, don't.
	 */
	usb_free_urb(urb);
	dev_kfree_skb_irq(skb);
}

int ar9170_op_tx(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	struct ar9170 *ar = hw->priv;
	struct urb *urb = NULL;
	struct ieee80211_hdr *hdr;
	struct ar9170_tx_control *txc;
	struct ieee80211_tx_info *info;
	struct ieee80211_rate *rate = NULL;
	unsigned long flags;
	u32 power, chains;
	u16 keytype = 0;
	u16 len;

	hdr = (void *)skb->data;
	info = IEEE80211_SKB_CB(skb);
	len = skb->len;
	txc = (void *)skb_push(skb, sizeof(*txc));

	/* Length */
	txc->length = cpu_to_le16(len + 4);
	txc->phy_control = 0;
	txc->mac_control = cpu_to_le16(AR9170_TX_MAC_HW_DURATION) |
			   cpu_to_le16(AR9170_TX_MAC_BACKOFF);

	if (info->control.hw_key) {
		switch (info->control.hw_key->alg) {
		case ALG_WEP:
			keytype = AR9170_TX_MAC_ENCR_WEP;
		case ALG_TKIP:
			keytype = AR9170_TX_MAC_ENCR_TKIP;
			break;
		case ALG_CCMP:
			keytype = AR9170_TX_MAC_ENCR_AES;
			break;
		default:
			WARN_ON(1);
			goto free;
		}
		printk(KERN_DEBUG "ar9170: tx hw crypto %#.4x\n", keytype);
	}

	txc->mac_control |= cpu_to_le16(keytype);

	if (info->flags & IEEE80211_TX_CTL_NO_ACK)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_NO_ACK);

	if (info->flags & IEEE80211_TX_CTL_AMPDU)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_AGGR);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	if (info->flags & IEEE80211_TX_CTL_USE_CTS_PROTECT)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_PROT_CTS);
	else if (info->flags & IEEE80211_TX_CTL_USE_RTS_CTS)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_PROT_RTS);

	if (info->flags & IEEE80211_TX_CTL_GREEN_FIELD)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_GREENFIELD);

	if (info->flags & IEEE80211_TX_CTL_SHORT_PREAMBLE)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_SHORT_PREAMBLE);

	if (info->flags & IEEE80211_TX_CTL_40_MHZ_WIDTH)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_BW_40MHZ);
	/* this works because 40 MHz is 2 and dup is 3 */
	if (info->flags & IEEE80211_TX_CTL_DUP_DATA)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_BW_40MHZ_DUP);

	if (info->flags & IEEE80211_TX_CTL_SHORT_GI)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_SHORT_GI);

	if (info->flags & IEEE80211_TX_CTL_OFDM_HT) {
		u32 r = info->tx_rate_idx; /* Is this correct? */
		u8 *txpower;
		r <<= AR9170_TX_PHY_MCS_SHIFT;
		if (WARN_ON(r & ~AR9170_TX_PHY_MCS_MASK))
			goto free;
		txc->phy_control |= cpu_to_le32(r & AR9170_TX_PHY_MCS_MASK);
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_MOD_HT);

		if (info->flags & IEEE80211_TX_CTL_40_MHZ_WIDTH) {
			if (info->band == IEEE80211_BAND_5GHZ)
				txpower = ar->power_5G_ht40;
			else
				txpower = ar->power_2G_ht40;
		} else {
			if (info->band == IEEE80211_BAND_5GHZ)
				txpower = ar->power_5G_ht20;
			else
				txpower = ar->power_2G_ht20;
		}

		power = txpower[info->tx_rate_idx & 7];
	} else {
		u8 *txpower;
		u32 mod;
		u32 phyrate;
		u8 idx = info->tx_rate_idx;

		if (info->band != IEEE80211_BAND_2GHZ) {
			idx += 4;
			txpower = ar->power_5G_leg;
			mod = AR9170_TX_PHY_MOD_OFDM;
		} else {
			if (idx < 4) {
				txpower = ar->power_2G_cck;
				mod = AR9170_TX_PHY_MOD_CCK;
			} else {
				mod = AR9170_TX_PHY_MOD_OFDM;
				txpower = ar->power_2G_ofdm;
			}
		}

		rate = &__ar9170_ratetable[idx];

		phyrate = rate->hw_value & 0xF;
		power = txpower[(rate->hw_value & 0x30) >> 4];
		phyrate <<= AR9170_TX_PHY_MCS_SHIFT;

		txc->phy_control |= cpu_to_le32(mod);
		txc->phy_control |= cpu_to_le32(phyrate);
	}

#else
	if (info->control.rates[0].flags & IEEE80211_TX_RC_USE_CTS_PROTECT)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_PROT_CTS);
	else if (info->control.rates[0].flags & IEEE80211_TX_RC_USE_RTS_CTS)
		txc->mac_control |= cpu_to_le16(AR9170_TX_MAC_PROT_RTS);

	if (info->control.rates[0].flags & IEEE80211_TX_RC_GREEN_FIELD)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_GREENFIELD);

	if (info->control.rates[0].flags & IEEE80211_TX_RC_USE_SHORT_PREAMBLE)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_SHORT_PREAMBLE);

	if (info->control.rates[0].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_BW_40MHZ);
	/* this works because 40 MHz is 2 and dup is 3 */
	if (info->control.rates[0].flags & IEEE80211_TX_RC_DUP_DATA)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_BW_40MHZ_DUP);

	if (info->control.rates[0].flags & IEEE80211_TX_RC_SHORT_GI)
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_SHORT_GI);

	if (info->control.rates[0].flags & IEEE80211_TX_RC_MCS) {
		u32 r = info->control.rates[0].idx;
		u8 *txpower;
		r <<= AR9170_TX_PHY_MCS_SHIFT;
		if (WARN_ON(r & ~AR9170_TX_PHY_MCS_MASK))
			goto free;
		txc->phy_control |= cpu_to_le32(r & AR9170_TX_PHY_MCS_MASK);
		txc->phy_control |= cpu_to_le32(AR9170_TX_PHY_MOD_HT);

		if (info->control.rates[0].flags & IEEE80211_TX_RC_40_MHZ_WIDTH) {
			if (info->band == IEEE80211_BAND_5GHZ)
				txpower = ar->power_5G_ht40;
			else
				txpower = ar->power_2G_ht40;
		} else {
			if (info->band == IEEE80211_BAND_5GHZ)
				txpower = ar->power_5G_ht20;
			else
				txpower = ar->power_2G_ht20;
		}

		power = txpower[info->control.rates[0].idx & 7];
	} else {
		u8 *txpower;
		u32 mod;
		u32 phyrate;
		u8 idx = info->control.rates[0].idx;

		if (info->band != IEEE80211_BAND_2GHZ) {
			idx += 4;
			txpower = ar->power_5G_leg;
			mod = AR9170_TX_PHY_MOD_OFDM;
		} else {
			if (idx < 4) {
				txpower = ar->power_2G_cck;
				mod = AR9170_TX_PHY_MOD_CCK;
			} else {
				mod = AR9170_TX_PHY_MOD_OFDM;
				txpower = ar->power_2G_ofdm;
			}
		}

		rate = &__ar9170_ratetable[idx];

		phyrate = rate->hw_value & 0xF;
		power = txpower[(rate->hw_value & 0x30) >> 4];
		phyrate <<= AR9170_TX_PHY_MCS_SHIFT;

		txc->phy_control |= cpu_to_le32(mod);
		txc->phy_control |= cpu_to_le32(phyrate);
	}
#endif
	power <<= AR9170_TX_PHY_TX_PWR_SHIFT;
	power &= AR9170_TX_PHY_TX_PWR_MASK;
	txc->phy_control |= cpu_to_le32(power);

	/* set TX chains */
	if (ar->eeprom.tx_mask == 1) {
		chains = AR9170_TX_PHY_TXCHAIN_1;
	} else {
		chains = AR9170_TX_PHY_TXCHAIN_2;

		/* >= 36M legacy OFDM - use only one chain */
		if (rate && rate->bitrate >= 360)
			chains = AR9170_TX_PHY_TXCHAIN_1;
	}
	txc->phy_control |= cpu_to_le32(chains << AR9170_TX_PHY_TXCHAIN_SHIFT);

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb)
		goto free;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	info->driver_data[0] = ar;
#else
	info->rate_driver_data[0] = ar;
#endif
	usb_fill_bulk_urb(urb, ar->udev,
			  usb_sndbulkpipe(ar->udev, AR9170_EP_TX),
			  skb->data, skb->len, tx_urb_complete, skb);

	spin_lock_irqsave(&ar->tx_status_lock, flags);

	if (usb_submit_urb(urb, GFP_ATOMIC))
		goto unlock;

#if 0
	if (!is_multicast_ether_addr(hdr->addr1)) {
		printk(KERN_DEBUG "ucast mac/phy ctl (seq): %#.4x/%#.8x (%d)\n",
			le16_to_cpu(txc->mac_control),
			le32_to_cpu(txc->phy_control),
			le16_to_cpu(hdr->seq_ctrl)>>4);
/*
 * need to see how to get status information first!
		ar9170_add_tx_status_skb(ar, skb);
 */
	} else {
		printk(KERN_DEBUG "mcast mac/phy ctl (seq): %#.4x/%#.8x (%d)\n",
			le16_to_cpu(txc->mac_control),
			le32_to_cpu(txc->phy_control),
			le16_to_cpu(hdr->seq_ctrl)>>4);
	}
#endif

	spin_unlock_irqrestore(&ar->tx_status_lock, flags);

	return NETDEV_TX_OK;
 unlock:
	spin_unlock_irqrestore(&ar->tx_status_lock, flags);
 free:
	usb_free_urb(urb);
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static int ar9170_set_mac_reg(struct ar9170 *ar, const u32 reg, const u8 *mac)
{
	static const u8 zero[ETH_ALEN] = { 0 };

	if (!mac)
		mac = zero;

	ar9170_regwrite_begin(ar);

	ar9170_regwrite(reg,
			(mac[3] << 24) | (mac[2] << 16) |
			(mac[1] << 8) | mac[0]);

	ar9170_regwrite(reg + 4, (mac[5] << 8) | mac[4]);

	ar9170_regwrite_finish();

	return ar9170_regwrite_result();
}

static int ar9170_op_add_interface(struct ieee80211_hw *hw,
				   struct ieee80211_if_init_conf *conf)
{
	struct ar9170 *ar = hw->priv;
	int err = 0;

	mutex_lock(&ar->mutex);

	if (ar->vif) {
		err = -EBUSY;
		goto unlock;
	}

	ar->vif = conf->vif;

	err = ar9170_set_mac_reg(ar, AR9170_MAC_REG_MAC_ADDR_L,
				 conf->mac_addr);

 unlock:
	mutex_unlock(&ar->mutex);
	return err;
}

static void ar9170_op_remove_interface(struct ieee80211_hw *hw,
				       struct ieee80211_if_init_conf *conf)
{
	struct ar9170 *ar = hw->priv;

	mutex_lock(&ar->mutex);
	ar->vif = NULL;
	dev_kfree_skb(ar->beacon);
	ar->beacon = NULL;
	ar9170_set_operating_mode(ar);
	ar9170_set_mac_reg(ar, AR9170_MAC_REG_MAC_ADDR_L, NULL);
	mutex_unlock(&ar->mutex);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static int ar9170_op_config(struct ieee80211_hw *hw, struct ieee80211_conf *conf)
{
	struct ar9170 *ar = hw->priv;
	int err = 0;

	mutex_lock(&ar->mutex);

	err = ar9170_set_operating_mode(ar);
	if (err)
		goto out;

	err = ar9170_set_channel(ar, hw->conf.channel,
				 AR9170_RFI_NONE, AR9170_BW_20);
	if (err)
		goto out;
	/* adjust slot time for 5 GHz */
	if (hw->conf.channel->band == IEEE80211_BAND_5GHZ)
		err = ar9170_write_reg(ar, AR9170_MAC_REG_SLOT_TIME,
					       9 << 10);

 out:
	mutex_unlock(&ar->mutex);
	return err;
}
#else
static int ar9170_op_config(struct ieee80211_hw *hw, u32 changed)
{
	struct ar9170 *ar = hw->priv;
	int err = 0;

	mutex_lock(&ar->mutex);

	if (changed & IEEE80211_CONF_CHANGE_RADIO_ENABLED) {
		/* TODO */
	}

	if (changed & IEEE80211_CONF_CHANGE_BEACON_INTERVAL) {
		err = ar9170_set_operating_mode(ar);
		if (err)
			goto out;
	}

	if (changed & IEEE80211_CONF_CHANGE_LISTEN_INTERVAL) {
		/* TODO */
	}

	if (changed & IEEE80211_CONF_CHANGE_PS) {
		/* TODO */
	}

	if (changed & IEEE80211_CONF_CHANGE_POWER) {
		/* TODO */
	}

	if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
		err = ar9170_set_channel(ar, hw->conf.channel,
					 AR9170_RFI_NONE, AR9170_BW_20);
		if (err)
			goto out;
		/* adjust slot time for 5 GHz */
		if (hw->conf.channel->band == IEEE80211_BAND_5GHZ)
			err = ar9170_write_reg(ar, AR9170_MAC_REG_SLOT_TIME,
					       9 << 10);
	}

	if (changed & IEEE80211_CONF_CHANGE_RETRY_LIMITS) {
		/* TODO */
	}

	if (changed & IEEE80211_CONF_CHANGE_HT) {
		/* TODO */
	}

 out:
	mutex_unlock(&ar->mutex);

	return err;
}
#endif /* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)) */

static int ar9170_op_config_interface(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif,
				      struct ieee80211_if_conf *conf)
{
	struct ar9170 *ar = hw->priv;
	int err = 0;

	mutex_lock(&ar->mutex);

	if (conf->changed & IEEE80211_IFCC_BSSID)
		err = ar9170_set_mac_reg(ar, AR9170_MAC_REG_BSSID_L,
					 conf->bssid);

	if (conf->changed & IEEE80211_IFCC_BEACON) {
		err = ar9170_update_beacon(ar);

		if (err)
			goto out;
		err = ar9170_set_operating_mode(ar);
	}

 out:
	mutex_unlock(&ar->mutex);

	return err;
}

static void ar9170_set_filters(struct work_struct *work)
{
	struct ar9170 *ar = container_of(work, struct ar9170,
					 filter_config_work);
	int err;

	mutex_lock(&ar->mutex);

	ar9170_regwrite_begin(ar);

	if (ar->cur_mc_hash != ar->want_mc_hash) {
		ar9170_regwrite(AR9170_MAC_REG_GROUP_HASH_TBL_H, ar->want_mc_hash >> 32);
		ar9170_regwrite(AR9170_MAC_REG_GROUP_HASH_TBL_L, ar->want_mc_hash);
	}

	if (ar->cur_filter != ar->want_filter)
		ar9170_regwrite(AR9170_MAC_REG_FRAMETYPE_FILTER,
				ar->want_filter);

	ar9170_regwrite_finish();
	err = ar9170_regwrite_result();
	if (err)
		goto out;

	ar->cur_mc_hash = ar->want_mc_hash;
	ar->cur_filter = ar->want_filter;
 out:
	mutex_unlock(&ar->mutex);
}

static void ar9170_op_configure_filter(struct ieee80211_hw *hw,
				       unsigned int changed_flags,
				       unsigned int *new_flags,
				       int mc_count, struct dev_mc_list *mclist)
{
	struct ar9170 *ar = hw->priv;
	u64 mchash;
	u32 filter;
	int i;

	/* mask supported flags */
	*new_flags &= FIF_ALLMULTI | FIF_CONTROL | FIF_BCN_PRBRESP_PROMISC;

	/*
	 * We can support more by setting the sniffer bit and
	 * then checking the error flags, later.
	 */

	if (*new_flags & FIF_ALLMULTI) {
		mchash = ~0ULL;
	} else {
		/* always get broadcast frames */
		mchash = 1ULL << (0xff>>2);

		for (i = 0; i < mc_count; i++) {
			if (WARN_ON(!mclist))
				break;
			mchash |= 1ULL << (mclist->dmi_addr[5] >> 2);
			mclist = mclist->next;
		}
	}

	filter = AR9170_MAC_REG_FTF_DEFAULTS;
	if (*new_flags & FIF_CONTROL)
		filter |= AR9170_MAC_REG_FTF_PSPOLL |
			  AR9170_MAC_REG_FTF_RTS |
			  AR9170_MAC_REG_FTF_CTS |
			  AR9170_MAC_REG_FTF_ACK |
			  AR9170_MAC_REG_FTF_CFE |
			  AR9170_MAC_REG_FTF_CFE_ACK;
	ar->want_filter = filter;
	ar->want_mc_hash = mchash;
	queue_work(ar->hw->workqueue, &ar->filter_config_work);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static void ar9170_op_bss_info_changed(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	struct ar9170 *ar = hw->priv;
	int err = 0;

	mutex_lock(&ar->mutex);

	ar9170_regwrite_begin(ar);

	if (changed & BSS_CHANGED_ASSOC) {
		/* TODO */
	}

	if (changed & BSS_CHANGED_ERP_SLOT) {
		u32 slottime = 20;
		if (bss_conf->use_short_slot)
			slottime = 9;
		ar9170_regwrite(AR9170_MAC_REG_SLOT_TIME, slottime << 10);
	}

	if (changed & BSS_CHANGED_HT) {
		/* TODO */
	}

	if (changed & BSS_CHANGED_BASIC_RATES) {
		u32 cck, ofdm;

		if (hw->conf.channel->band == IEEE80211_BAND_5GHZ) {
			ofdm = bss_conf->basic_rates;
			cck = 0;
		} else {
			/* four cck rates */
			cck = bss_conf->basic_rates & 0xf;
			ofdm = bss_conf->basic_rates >> 4;
		}
		ar9170_regwrite(AR9170_MAC_REG_BASIC_RATE,
				ofdm << 8 | cck);
	}

	ar9170_regwrite_finish();
	err = ar9170_regwrite_result();

	mutex_unlock(&ar->mutex);

	WARN_ON(err);
}
#endif

static int ar9170_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
			  const u8 *local_address, const u8 *address,
			  struct ieee80211_key_conf *key)
{
	struct ar9170 *ar = hw->priv;
	int err = 0, i;
	u8 ktype;

#ifndef HWCRYPT
	return -EOPNOTSUPP;
#endif

	switch (key->alg) {
	case ALG_WEP:
		if (key->keylen == LEN_WEP40)
			ktype = AR9170_ENC_ALG_WEP64;
		else
			ktype = AR9170_ENC_ALG_WEP128;
		break;
	case ALG_TKIP:
		ktype = AR9170_ENC_ALG_TKIP;
		break;
	case ALG_CCMP:
		ktype = AR9170_ENC_ALG_AESCCMP;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&ar->mutex);

	if (cmd == SET_KEY) {
		/* group keys need all-zeroes address */
		if (!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE))
			address = NULL;

		if (key->flags & IEEE80211_KEY_FLAG_PAIRWISE) {
			for (i = 0; i < 64; i++)
				if (!(ar->usedkeys & BIT(i)))
					break;
			if (i == 64) {
				err = -ENOSPC;
				goto out;
			}
		} else {
			i = 64 + key->keyidx;
		}

		key->hw_key_idx = i;

		err = ar9170_upload_key(ar, i, address, ktype, 0,
					key->key, min_t(u8, 16, key->keylen));
		if (err)
			goto out;

		if (key->alg == ALG_TKIP) {
			err = ar9170_upload_key(ar, i, address, ktype, 1,
						key->key + 16, 16);
			if (err)
				goto out;
		}

		if (i < 64)
			ar->usedkeys |= BIT(i);

		key->flags |= IEEE80211_KEY_FLAG_GENERATE_IV;
		key->flags |= IEEE80211_KEY_FLAG_GENERATE_MMIC;

		printk(KERN_DEBUG "ar9170: installed key to %d\n", i);
	} else {
		err = ar9170_disable_key(ar, key->hw_key_idx);
		if (err)
			goto out;

		if (key->hw_key_idx < 64) {
			ar->usedkeys &= ~BIT(key->hw_key_idx);
		} else {
			err = ar9170_upload_key(ar, key->hw_key_idx, NULL,
						AR9170_ENC_ALG_NONE, 0,
						NULL, 0);
			if (err)
				goto out;

			if (key->alg == ALG_TKIP) {
				err = ar9170_upload_key(ar, key->hw_key_idx,
							NULL,
							AR9170_ENC_ALG_NONE, 1,
							NULL, 0);
				if (err)
					goto out;
			}

		}

		printk(KERN_DEBUG "ar9170: removed key %d\n", key->hw_key_idx);
	}

	ar9170_regwrite_begin(ar);

	ar9170_regwrite(AR9170_MAC_REG_ROLL_CALL_TBL_L, ar->usedkeys);
	ar9170_regwrite(AR9170_MAC_REG_ROLL_CALL_TBL_H, ar->usedkeys >> 32);

	ar9170_regwrite_finish();
	err = ar9170_regwrite_result();

 out:
	mutex_unlock(&ar->mutex);

	return err;
}

static const struct ieee80211_ops ar9170_ops = {
	.start			= ar9170_op_start,
	.stop			= ar9170_op_stop,
	.tx			= ar9170_op_tx,
	.add_interface		= ar9170_op_add_interface,
	.remove_interface	= ar9170_op_remove_interface,
	.config			= ar9170_op_config,
	.config_interface	= ar9170_op_config_interface,
	.configure_filter	= ar9170_op_configure_filter,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	.bss_info_changed	= ar9170_op_bss_info_changed,
#endif
	.set_key		= ar9170_set_key,
};

static struct ar9170 *ar9170_alloc(struct usb_interface *intf)
{
	struct ieee80211_hw *hw;
	struct ar9170 *ar;

	hw = ieee80211_alloc_hw(sizeof(struct ar9170), &ar9170_ops);
	if (!hw)
		return ERR_PTR(-ENOMEM);

	ar = hw->priv;
	ar->hw = hw;
	ar->udev = interface_to_usbdev(intf);
	mutex_init(&ar->mutex);
	spin_lock_init(&ar->cmdlock);
	spin_lock_init(&ar->tx_status_lock);
	INIT_WORK(&ar->filter_config_work, ar9170_set_filters);
	INIT_WORK(&ar->led_work, ar9170_update_leds);
	INIT_WORK(&ar->beacon_work, ar9170_new_beacon);

	/* all hw supports 2.4 GHz, so set channel to 1 by default */
	ar->channel = &ar9170_2ghz_chantable[0];

	return ar;
}

static int ar9170_read_eeprom(struct usb_interface *intf, struct ar9170 *ar)
{
#define RW	8	/* number of words to read at once */
#define RB	(sizeof(u32)  *RW)
	DECLARE_MAC_BUF(mbuf);
	u8 *eeprom = (void *)&ar->eeprom;
	u8 *addr = ar->eeprom.mac_address;
	__le32 offsets[RW];
	int i, j, err, bands = 0;

	BUILD_BUG_ON(sizeof(ar->eeprom) & 3);

	BUILD_BUG_ON(RB > AR9170_MAX_CMD_LEN - 4);
#ifndef __CHECKER__
	/* don't want to handle trailing remains */
	BUILD_BUG_ON(sizeof(ar->eeprom) % RB);
#endif

	for (i = 0; i < sizeof(ar->eeprom)/RB; i++) {
		for (j = 0; j < RW; j++)
			offsets[j] = cpu_to_le32(AR9170_EEPROM_START +
						 RB * i + 4 * j);

		err = ar9170_exec_cmd(ar, AR9170_CMD_RREG,
				      RB, (u8 *) &offsets,
				      RB, eeprom + RB * i);
		if (err)
			return err;
	}

#undef RW
#undef RB

	if (ar->eeprom.length == cpu_to_le16(0xFFFF))
		return -ENODATA;

	if (ar->eeprom.operating_flags & AR9170_OPFLAG_2GHZ) {
		ar->hw->wiphy->bands[IEEE80211_BAND_2GHZ] = &ar9170_band_2GHz;
		bands++;
	}
	if (ar->eeprom.operating_flags & AR9170_OPFLAG_5GHZ) {
		ar->hw->wiphy->bands[IEEE80211_BAND_5GHZ] = &ar9170_band_5GHz;
		bands++;
	}
	/*
	 * I measured this, a bandswitch takes roughly
	 * 135 ms and a frequency switch about 80.
	 *
	 * FIXME: measure these values again once EEPROM settings
	 *	  are used, that will influence them!
	 */
	if (bands == 2)
		ar->hw->channel_change_time = 135 * 1000;
	else
		ar->hw->channel_change_time = 80 * 1000;

	/* second part of wiphy init */
	SET_IEEE80211_PERM_ADDR(ar->hw, addr);

	return 0;
}

static void ar9170_irq_completed(struct urb *urb)
{
	struct ar9170 *ar = urb->context;

	print_hex_dump(KERN_DEBUG, "ar9170 irq: ", DUMP_PREFIX_OFFSET,
		       16, 1, ar->ibuf, urb->actual_length, true);
	usb_submit_urb(urb, GFP_KERNEL);
}

static int ar9170_irq_urb(struct usb_interface *intf,
			  struct ar9170 *ar)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct urb *u;
	int err;

	if (WARN_ON(ar->iurb))
		return -EBUSY;

	u = usb_alloc_urb(0, GFP_KERNEL);

	if (!u)
		return -ENOMEM;

	usb_fill_int_urb(u, udev, usb_rcvintpipe(udev, AR9170_EP_IRQ),
			 ar->ibuf, sizeof(ar->ibuf), ar9170_irq_completed,
			 ar, 4);

	err = usb_submit_urb(u, GFP_KERNEL);
	if (err) {
		usb_free_urb(u);
		return err;
	}

	ar->iurb = u;

	return 0;
}

static void ar9170_cancel_irq_urb(struct ar9170 *ar)
{
	if (!ar->iurb)
		return;

	usb_kill_urb(ar->iurb);
	usb_free_urb(ar->iurb);
	ar->iurb = NULL;
}

static void ar9170_rx_completed(struct urb *urb);

static int ar9170_prep_rx_urb(struct ar9170 *ar, struct urb *u, gfp_t gfp)
{
	struct sk_buff *skb;

	skb = __dev_alloc_skb(AR9170_MAX_RX_BUFFER_SIZE + 32, gfp);
	if (!skb)
		return -ENOMEM;

	/* reserve some space for mac80211's radiotap */
	skb_reserve(skb, 32);

	((void **)skb->cb)[0] = ar;

	usb_fill_bulk_urb(u, ar->udev, usb_rcvbulkpipe(ar->udev, AR9170_EP_RX),
			  skb->data, min(skb_tailroom(skb), 8192),
			  ar9170_rx_completed, skb);

	return 0;
}

static void ar9170_handle_command_response(struct ar9170 *ar, u8 *buf, int len)
{
	u8 type;
	u32 in, out;

	type = buf[1];

	if ((type & 0xc0) != 0xc0) {
		in = le32_to_cpup((__le32 *)buf);
		out = le32_to_cpu(ar->cmdbuf[0]);

		/* mask off length byte */
		out &= ~0xFF;

		if (ar->readlen >= 0) {
			/* add expected length */
			out |= ar->readlen;
		} else {
			/* add obtained length */
			out |= in & 0xFF;
		}

		if (unlikely(out != in)) {
			print_hex_dump(KERN_DEBUG, "ar9170 invalid resp: ",
				       DUMP_PREFIX_OFFSET,
				       16, 1, buf, len, true);
			/*
			 * Do not complete, then the command times out,
			 * and we get a stack trace from there.
			 */
			return;
		}

		spin_lock(&ar->cmdlock);

		if (ar->readbuf && ar->readlen > 0) {
			memcpy(ar->readbuf, buf + 4, ar->readlen);
			ar->readbuf = NULL;
		}

		complete(&ar->cmd_wait);
		spin_unlock(&ar->cmdlock);
	} else switch (type) {
	case 0xc1:
		/*
		 * TX status notification:
		 * bytes: 0c c1 .. .. M1 M2 M3 M4 M5 M6 R4 R3 R2 R1 S2 S1
		 *
		 * M1-M6 is the MAC address
		 * R1-R4 is the transmit rate
		 * S1-S2 is the transmit status
		 *
		 * ignored for now.
		 */
		break;
	case 0xc0:
		/*
		 * pre-TBTT event
		 */
		if (ar->vif && ar->vif->type == NL80211_IFTYPE_AP)
			queue_work(ar->hw->workqueue, &ar->beacon_work);
	default:
		break;
	}
}

/*
 * If the frame alignment is right (or the kernel has
 * CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS), and there
 * is only a single MPDU in the USB frame, then we can
 * submit to mac80211 the SKB directly. However, since
 * there may be multiple packets in one SKB in stream
 * mode, and we need to observe the proper ordering,
 * this is non-trivial.
 */
static void ar9170_handle_mpdu(struct ar9170 *ar, u8 *buf, int len)
{
	struct sk_buff *skb;
	struct ar9170_rx_head *head = (void *)buf;
	struct ar9170_rx_tail *tail;
	struct ieee80211_rx_status status;
	int mpdu_len, i;
	u8 error, antennas = 0, decrypt;

	/* Received MPDU */
	mpdu_len = len;
	mpdu_len -= sizeof(struct ar9170_rx_head);
	mpdu_len -= sizeof(struct ar9170_rx_tail);
	BUILD_BUG_ON(sizeof(struct ar9170_rx_head) != 12);
	BUILD_BUG_ON(sizeof(struct ar9170_rx_tail) != 24);

	if (mpdu_len <= FCS_LEN)
		return;

	tail = (void *)(buf + sizeof(struct ar9170_rx_head) + mpdu_len);

	for (i = 0; i < 3; i++)
		if (tail->rssi[i] != 0x80)
			antennas |= BIT(i);

	/* post-process RSSI */
	for (i = 0; i < 7; i++)
		if (tail->rssi[i] & 0x80)
			tail->rssi[i] = ((tail->rssi[i] & 0x7f) + 1) & 0x7f;

	memset(&status, 0, sizeof(status));

	status.band = ar->channel->band;
	status.freq = ar->channel->center_freq;
	status.signal = tail->rssi_combined;
	status.antenna = antennas;

	switch (tail->status & AR9170_RX_STATUS_MODULATION_MASK) {
	case AR9170_RX_STATUS_MODULATION_CCK:
		if (tail->status & AR9170_RX_STATUS_SHORT_PREAMBLE)
			status.flag |= RX_FLAG_SHORTPRE;
		switch (head->plcp[0]) {
		case 0x0a:
			status.rate_idx = 0;
			break;
		case 0x14:
			status.rate_idx = 1;
			break;
		case 0x37:
			status.rate_idx = 2;
			break;
		case 0x6e:
			status.rate_idx = 3;
			break;
		default:
			printk(KERN_DEBUG "ar9170: invalid plcp cck rate\n");
			return;
		}
		break;
	case AR9170_RX_STATUS_MODULATION_OFDM:
		switch (head->plcp[0] & 0xF) {
		case 0xB:
			status.rate_idx = 0;
			break;
		case 0xF:
			status.rate_idx = 1;
			break;
		case 0xA:
			status.rate_idx = 2;
			break;
		case 0xE:
			status.rate_idx = 3;
			break;
		case 0x9:
			status.rate_idx = 4;
			break;
		case 0xD:
			status.rate_idx = 5;
			break;
		case 0x8:
			status.rate_idx = 6;
			break;
		case 0xC:
			status.rate_idx = 7;
			break;
		default:
			printk(KERN_DEBUG "ar9170: invalid plcp ofdm rate\n");
			return;
		}
		if (status.band == IEEE80211_BAND_2GHZ)
			status.rate_idx += 4;
		break;
	case AR9170_RX_STATUS_MODULATION_HT:
	case AR9170_RX_STATUS_MODULATION_DUPOFDM:
		/* XXX */
		printk(KERN_DEBUG "ar9170: invalid modulation\n");
		return;
	}

	error = tail->error;

	if (error & AR9170_RX_ERROR_MMIC) {
		status.flag |= RX_FLAG_MMIC_ERROR;
		error &= ~AR9170_RX_ERROR_MMIC;
	}

	if (error & AR9170_RX_ERROR_PLCP) {
		status.flag |= RX_FLAG_FAILED_PLCP_CRC;
		error &= ~AR9170_RX_ERROR_PLCP;
	}

	if (error & AR9170_RX_ERROR_FCS) {
		status.flag |= RX_FLAG_FAILED_FCS_CRC;
		error &= ~AR9170_RX_ERROR_FCS;
	}

	decrypt = ar9170_get_decrypt_type(tail);
	if (!(decrypt & AR9170_RX_ENC_SOFTWARE) &&
	    decrypt != AR9170_ENC_ALG_NONE) {
		printk(KERN_DEBUG "hw decrypt!\n");
		status.flag |= RX_FLAG_DECRYPTED;
	}

	/* ignore wrong RA errors */
	error &= ~AR9170_RX_ERROR_WRONG_RA;

	if (error & AR9170_RX_ERROR_DECRYPT) {
		error &= ~AR9170_RX_ERROR_DECRYPT;
		printk(KERN_DEBUG "ar9170: decrypt error\n");
	}

	/* drop any other error frames */
	if (error) {
		printk(KERN_DEBUG "ar9170: errors: %#x\n", error);
		return;
	}

	skb = dev_alloc_skb(mpdu_len + 32);
	if (!skb)
		return;

	skb_reserve(skb, 32);
	memcpy(skb_put(skb, mpdu_len),
	       buf + sizeof(struct ar9170_rx_head),
	       mpdu_len);

	ieee80211_rx_irqsafe(ar->hw, skb, &status);
}

static void ar9170_rx_completed(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct ar9170 *ar = ((void **)skb->cb)[0];
	int i, tlen = urb->actual_length, resplen;
	u8 *tbuf = skb->data, *respbuf;

	/* device died */
	if (urb->status == -EOVERFLOW) {
		printk(KERN_DEBUG "complete rx %p (%db, %d)\n", urb, tlen, urb->status);
		return;
	}

	if (urb->status)
		goto resubmit;

	while (tlen >= 4) {
		int clen = tbuf[1] << 8 | tbuf[0];
		int wlen = (clen + 3) & ~3;

		/*
		 * parse stream (if any)
		 */
		if (tbuf[2] != 0 || tbuf[3] != 0x4e) {
			printk(KERN_ERR "ar9170: missing tag!\n");
			goto resubmit;
		}
		if (wlen > tlen - 4) {
			printk(KERN_ERR "ar9170: invalid RX (%d, %d, %d)\n",
			       clen, wlen, tlen);
			print_hex_dump(KERN_DEBUG, "data: ",
				       DUMP_PREFIX_OFFSET,
				       16, 1, tbuf, tlen, true);
			goto resubmit;
		}
		resplen = clen;
		respbuf = tbuf + 4;
		tbuf += wlen + 4;
		tlen -= wlen + 4;

		i = 0;

		/* weird thing, but this is the same in the original driver */
		while (resplen > 2 && i < 12 &&
		       respbuf[0] == 0xff && respbuf[1] == 0xff) {
			i += 2;
			resplen -= 2;
			respbuf += 2;
		}

		if (resplen < 4)
			continue;

		/* found the 6 * 0xffff marker? */
		if (i == 12)
			ar9170_handle_command_response(ar, respbuf, resplen);
		else
			ar9170_handle_mpdu(ar, respbuf, resplen);
	}

	if (tlen)
		printk(KERN_ERR "ar9170: buffer remains!\n");

 resubmit:
	usb_submit_urb(urb, GFP_KERNEL);
}

static int ar9170_rx_urbs(struct usb_interface *intf, struct ar9170 *ar)
{
	struct urb *u;
	int err, i;

	if (WARN_ON(ar->rxurbs[0]))
		return -EBUSY;

	for (i = 0; i < AR9170_NUM_RX_URBS; i++) {
		u = usb_alloc_urb(0, GFP_KERNEL);
		if (!u)
			goto nomem;

		err = ar9170_prep_rx_urb(ar, u, GFP_KERNEL);
		if (err)
			goto nomem;

		err = usb_submit_urb(u, GFP_KERNEL);
		if (err)
			printk(KERN_ERR "ar9170: submit RX urb failed\n");

		ar->rxurbs[i] = u;
	}

	return 0;
 nomem:
	for (i = 0; i < AR9170_NUM_RX_URBS; i++) {
		u = ar->rxurbs[i];
		if (!u)
			continue;

		dev_kfree_skb(u->context);
		usb_free_urb(u);
		ar->rxurbs[i] = NULL;
	}

	return -ENOMEM;
}

static void ar9170_cancel_rx_urbs(struct ar9170 *ar)
{
	struct urb *u;
	int i;

	for (i = 0; i < AR9170_NUM_RX_URBS; i++) {
		u = ar->rxurbs[i];
		if (!u)
			continue;

		usb_kill_urb(u);
		dev_kfree_skb(u->context);
		usb_free_urb(u);
		ar->rxurbs[i] = NULL;
	}
}

static int ar9170_load_fw(struct ar9170 *ar, const struct firmware *fw,
			  u32 addr, bool complete)
{
	size_t sz = fw->size;
	int transfer, err;
	char *buf = kmalloc(4096, GFP_KERNEL);
	const char *data = fw->data;

	if (!buf)
		return -ENOMEM;

	while (sz) {
		transfer = min_t(int, sz, 4096);
		memcpy(buf, data, transfer);
		err = usb_control_msg(ar->udev,
				      usb_sndctrlpipe(ar->udev, 0),
				      0x30 /* FW DL */, 0x40 | USB_DIR_OUT,
				      addr >> 8, 0, buf, transfer, 1000);
		if (err >= 0 && err != transfer)
			err = -EMSGSIZE;
		if (err < 0)
			return err;
		sz -= transfer;
		data += transfer;
		addr += transfer;
	}

	if (complete) {
		err = usb_control_msg(ar->udev,
				      usb_sndctrlpipe(ar->udev, 0),
				      0x31 /* FW DL COMPLETE */,
				      0x40 | USB_DIR_OUT,
				      0, 0, NULL, 0, 1000);
		if (err)
			return err;
	}

	kfree(buf);

	return 0;
}

static int ar9170_upload_firmware(struct ar9170 *ar)
{
	const struct firmware *fw;
	int err;

	/* First, upload initial values to device RAM */
	err = request_firmware(&fw, "ar9170-1.fw", &ar->udev->dev);
	if (err) {
		printk(KERN_DEBUG "ar9170: firmware part 1 not found\n");
		return err;
	}

	err = ar9170_load_fw(ar, fw, 0x102800, false);
	if (err) {
		printk(KERN_DEBUG "ar9170: firmware part 1 upload failed\n");
		return err;
	}

	release_firmware(fw);

	/* let it settle */
	msleep(1000);

	/* Then, upload the firmware itself and start it */
	err = request_firmware(&fw, "ar9170-2.fw", &ar->udev->dev);
	if (err) {
		printk(KERN_DEBUG "ar9170: firmware part 2 not found\n");
		return err;
	}

	err = ar9170_load_fw(ar, fw, 0x200000, true);
	if (err) {
		printk(KERN_DEBUG "ar9170: firmware part 2 upload failed\n");
		return err;
	}

	release_firmware(fw);

	return 0;
}

static int ar9170_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct ar9170 *ar;
	int err;

	ar = ar9170_alloc(intf);
	if (IS_ERR(ar)) {
		err = PTR_ERR(ar);
		goto out;
	}

	/* first part of wiphy init */
	SET_IEEE80211_DEV(ar->hw, &udev->dev);
	ar->hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
					 BIT(NL80211_IFTYPE_WDS) |
					 BIT(NL80211_IFTYPE_ADHOC);
	ar->hw->flags |= IEEE80211_HW_RX_INCLUDES_FCS |
			 IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING;
	ar->hw->extra_tx_headroom = 8;

	usb_set_intfdata(intf, ar);

	mutex_lock(&ar->mutex);

	err = ar9170_rx_urbs(intf, ar);
	if (err)
		goto err_unlock;

	err = ar9170_irq_urb(intf, ar);
	if (err)
		goto err_unrx;

	err = ar9170_upload_firmware(ar);
	if (err)
		goto err_unirq;

	/* disable LEDs */
	/* GPIO 0/1 mode: output, 2/3: input */
	err = ar9170_write_reg(ar, AR9170_GPIO_REG_PORT_TYPE, 3);
	if (err)
		goto err_unirq;

	/* GPIO 0/1 value: off */
	err = ar9170_write_reg(ar, AR9170_GPIO_REG_DATA, 0);
	if (err)
		goto err_unirq;

	err = ar9170_init_leds(ar);
	if (err)
		goto err_unirq;

	/* try to read EEPROM, init MAC addr */
	err = ar9170_read_eeprom(intf, ar);
	if (err)
		goto err_unirq;

	mutex_unlock(&ar->mutex);

	ieee80211_register_hw(ar->hw);

	return 0;
 err_unirq:
	ar9170_cancel_irq_urb(ar);
 err_unrx:
	ar9170_cancel_rx_urbs(ar);
 err_unlock:
	mutex_unlock(&ar->mutex);
	mutex_destroy(&ar->mutex);
	ieee80211_free_hw(ar->hw);
 out:
	return err;
}

static void ar9170_disconnect(struct usb_interface *intf)
{
	struct ar9170 *ar = usb_get_intfdata(intf);

	ar9170_exit_leds(ar);

	ieee80211_unregister_hw(ar->hw);

	mutex_lock(&ar->mutex);

	ar9170_cancel_irq_urb(ar);
	ar9170_cancel_rx_urbs(ar);

	mutex_unlock(&ar->mutex);

	mutex_destroy(&ar->mutex);
	ieee80211_free_hw(ar->hw);
}

static struct usb_driver ar9170_driver = {
	.name = KBUILD_MODNAME,
	.probe = ar9170_probe,
	.disconnect = ar9170_disconnect,
	.id_table = ar9170_ids,
};

static int __init ar9170_init(void)
{
	return usb_register(&ar9170_driver);
}

static void __exit ar9170_exit(void)
{
	usb_deregister(&ar9170_driver);
}

module_init(ar9170_init);
module_exit(ar9170_exit);
