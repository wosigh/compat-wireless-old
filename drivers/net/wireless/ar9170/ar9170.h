/*
 * Atheros 11n USB driver
 *
 * Driver specific definitions
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
#ifndef __AR9170_H
#define __AR9170_H

#include <linux/usb.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/leds.h>
#include <net/wireless.h>
#include <net/mac80211.h>
#include "eeprom.h"
#include "hw.h"


//#define HWCRYPT


#define PAYLOAD_MAX	(AR9170_MAX_CMD_LEN/4 - 1)

enum ar9170_bw {
	AR9170_BW_20,
	AR9170_BW_40_BELOW,
	AR9170_BW_40_ABOVE,

	__AR9170_NUM_BW,
};

enum ar9170_rf_init_mode {
	AR9170_RFI_NONE,
	AR9170_RFI_WARM,
	AR9170_RFI_COLD,
};

#define AR9170_MAX_RX_BUFFER_SIZE		8192
#define AR9170_NUM_RX_URBS			16

struct ar9170;

struct ar9170_led {
	struct ar9170 *ar;
	struct led_classdev l;
	char name[32];
};

struct ar9170 {
	struct ieee80211_hw *hw;
	struct usb_device *udev;
	struct mutex mutex;
	bool started;

	struct ieee80211_vif *vif;

	/* beaconing */
	struct sk_buff *beacon;
	struct work_struct beacon_work;

	u64 usedkeys;

	struct work_struct filter_config_work;
	u64 cur_mc_hash, want_mc_hash;
	u32 cur_filter, want_filter;
	struct ieee80211_channel *channel;

	spinlock_t cmdlock;
	struct completion cmd_wait;
	int readlen;
	u8 *readbuf;

	struct sk_buff *tx_status_queue_head;
	struct sk_buff *tx_status_queue_tail;
	spinlock_t tx_status_lock;

	/* power calibration data */
	u8 power_5G_leg[4];
	u8 power_2G_cck[4];
	u8 power_2G_ofdm[4];
	u8 power_5G_ht20[8];
	u8 power_5G_ht40[8];
	u8 power_2G_ht20[8];
	u8 power_2G_ht40[8];

	struct work_struct led_work;
	struct ar9170_led leds[AR9170_NUM_LEDS];

	struct urb *iurb;
	u8 ibuf[64];

	struct urb *rxurbs[AR9170_NUM_RX_URBS];

	__le32 cmdbuf[PAYLOAD_MAX + 1];

	struct ar9170_eeprom eeprom;
};

/* basic HW access */
int ar9170_exec_cmd(struct ar9170 *ar, enum ar9170_cmd cmd,
		    int plen, u8 *payload, int outlen, u8 *out);
int ar9170_write_reg(struct ar9170 *ar, const u32 reg, const u32 val);
int ar9170_read_reg(struct ar9170 *ar, u32 reg, u32 *val);
int ar9170_echo_test(struct ar9170 *ar, u32 v);

/*
 * Macros to facilitate writing multiple registers in a single
 * write-combining USB command. Note that when the first group
 * fails the whole thing will fail without any others attempted,
 * but you won't know which write in the group failed.
 */
#define ar9170_regwrite_begin(ar)					\
{									\
	int __nreg = 0, __err = 0;					\
	struct ar9170 *__ar = ar;

#define ar9170_regwrite(r, v) do {					\
	__ar->cmdbuf[2 * __nreg + 1] = cpu_to_le32(r);			\
	__ar->cmdbuf[2 * __nreg + 2] = cpu_to_le32(v);			\
	__nreg++;							\
	if (__nreg >= PAYLOAD_MAX/2) {					\
		__err = ar9170_exec_cmd(__ar, AR9170_CMD_WREG,		\
					8 * __nreg, 			\
					(u8 *) &__ar->cmdbuf[1],	\
					0, NULL);			\
		__nreg = 0;						\
		if (__err)						\
			goto __regwrite_out;				\
	}								\
} while (0)

#define ar9170_regwrite_finish()					\
__regwrite_out:								\
	if (__nreg) {							\
		__err = ar9170_exec_cmd(__ar, AR9170_CMD_WREG,		\
					8 * __nreg, 			\
					(u8 *) &__ar->cmdbuf[1],	\
					0, NULL);			\
		__nreg = 0;						\
	}

#define ar9170_regwrite_result()					\
	__err;								\
}

/* MAC */
int ar9170_op_tx(struct ieee80211_hw *hw, struct sk_buff *skb);
int ar9170_init_mac(struct ar9170 *ar);
int ar9170_set_operating_mode(struct ar9170 *ar);
int ar9170_update_beacon(struct ar9170 *ar);
void ar9170_new_beacon(struct work_struct *work);
int ar9170_upload_key(struct ar9170 *ar, u8 id, const u8 *mac, u8 ktype,
		      u8 keyidx, u8 *keydata, int keylen);
int ar9170_disable_key(struct ar9170 *ar, u8 id);

/* LEDs */
int ar9170_init_leds(struct ar9170 *ar);
void ar9170_exit_leds(struct ar9170 *ar);
void ar9170_update_leds(struct work_struct *work);

/* PHY / RF */
int ar9170_init_phy(struct ar9170 *ar, enum ieee80211_band band);
int ar9170_init_rf(struct ar9170 *ar);
int ar9170_set_channel(struct ar9170 *ar, struct ieee80211_channel *channel,
		       enum ar9170_rf_init_mode rfi, enum ar9170_bw bw);

#if 0
#include <linux/mutex.h>
#define mutex_lock(m) do { mutex_lock(m); __acquire(m); } while (0)
#define mutex_unlock(m) do { mutex_unlock(m); __release(m); } while (0)
#endif

#endif /* __AR9170_H */
