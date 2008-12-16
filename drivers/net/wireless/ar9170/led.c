/*
 * Atheros 11n USB driver
 *
 * LED handling
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
#include "ar9170.h"

void ar9170_update_leds(struct work_struct *work)
{
        struct ar9170 *ar = container_of(work, struct ar9170,
        				 led_work);
	u32 ledval = 0;
	int i;

	mutex_lock(&ar->mutex);

	for (i = 0; i < AR9170_NUM_LEDS; i++)
		if (ar->leds[i].l.brightness)
			ledval |= 1<<i;

	ar9170_write_reg(ar, AR9170_GPIO_REG_DATA, ledval);

	mutex_unlock(&ar->mutex);
}

static void ar9170_led_brightness_set(struct led_classdev *led,
				      enum led_brightness brightness)
{
	struct ar9170_led *arl = container_of(led, struct ar9170_led, l);
	struct ar9170 *ar = arl->ar;

	queue_work(ar->hw->workqueue, &ar->led_work);
}

int ar9170_init_leds(struct ar9170 *ar)
{
	int err, i;

	for (i = 0; i < AR9170_NUM_LEDS; i++) {
		snprintf(ar->leds[i].name, sizeof(ar->leds[i].name),
			 "%s-led%d", wiphy_name(ar->hw->wiphy), i + 1);

		ar->leds[i].ar = ar;
		ar->leds[i].l.name = ar->leds[i].name;
		ar->leds[i].l.brightness_set = ar9170_led_brightness_set;
		ar->leds[i].l.brightness = 0;

		err = led_classdev_register(wiphy_dev(ar->hw->wiphy),
					    &ar->leds[i].l);
		if (err)
			goto fail;
	}

	return 0;

 fail:
	i--;
	for (; i >= 0; i--)
		led_classdev_unregister(&ar->leds[i].l);

	return err;
}

void ar9170_exit_leds(struct ar9170 *ar)
{
	int i;

	for (i = 0; i < AR9170_NUM_LEDS; i++)
		led_classdev_unregister(&ar->leds[i].l);
}
