/*
 * Copyright (c) 2004-2008 Reyk Floeter <reyk@openbsd.org>
 * Copyright (c) 2006-2008 Nick Kossifidis <mickflemm@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*************************************\
* DMA and interrupt masking functions *
\*************************************/

/*
 * dma.c - DMA and interrupt masking functions
 *
 * Here we setup descriptor pointers (rxdp/txdp) start/stop dma engine and
 * handle queue setup for 5210 chipset (rest are handled on qcu.c).
 * Also we setup interrupt mask register (IMR) and read the various iterrupt
 * status registers (ISR).
 *
 * TODO: Handle SISR on 5211+ and introduce a function to return the queue
 * number that resulted the interrupt.
 */

#include "ath5k.h"
#include "reg.h"
#include "debug.h"
#include "base.h"

/*********\
* Receive *
\*********/

/**
 * ath5k_hw_start_rx_dma - Start DMA receive
 *
 * @ah:	The &struct ath5k_hw
 */
void ath5k_hw_start_rx_dma(struct ath5k_hw *ah)
{
	ATH5K_TRACE(ah->ah_sc);
	ath5k_hw_reg_write(ah, AR5K_CR_RXE, AR5K_CR);
	ath5k_hw_reg_read(ah, AR5K_CR);
}

/**
 * ath5k_hw_stop_rx_dma - Stop DMA receive
 *
 * @ah:	The &struct ath5k_hw
 */
int ath5k_hw_stop_rx_dma(struct ath5k_hw *ah)
{
	unsigned int i;

	ATH5K_TRACE(ah->ah_sc);
	ath5k_hw_reg_write(ah, AR5K_CR_RXD, AR5K_CR);

	/*
	 * It may take some time to disable the DMA receive unit
	 */
	for (i = 2000; i > 0 &&
			(ath5k_hw_reg_read(ah, AR5K_CR) & AR5K_CR_RXE) != 0;
			i--)
		udelay(10);

	return i ? 0 : -EBUSY;
}

/**
 * ath5k_hw_get_rxdp - Get RX Descriptor's address
 *
 * @ah: The &struct ath5k_hw
 *
 * XXX: Is RXDP read and clear ?
 */
u32 ath5k_hw_get_rxdp(struct ath5k_hw *ah)
{
	return ath5k_hw_reg_read(ah, AR5K_RXDP);
}

/**
 * ath5k_hw_set_rxdp - Set RX Descriptor's address
 *
 * @ah: The &struct ath5k_hw
 * @phys_addr: RX descriptor address
 *
 * XXX: Should we check if rx is enabled before setting rxdp ?
 */
void ath5k_hw_set_rxdp(struct ath5k_hw *ah, u32 phys_addr)
{
	ATH5K_TRACE(ah->ah_sc);

	ath5k_hw_reg_write(ah, phys_addr, AR5K_RXDP);
}


/**********\
* Transmit *
\**********/

/**
 * ath5k_hw_start_tx_dma - Start DMA transmit for a specific queue
 *
 * @ah: The &struct ath5k_hw
 * @queue: The hw queue number
 *
 * Start DMA transmit for a specific queue and since 5210 doesn't have
 * QCU/DCU, set up queue parameters for 5210 here based on queue type (one
 * queue for normal data and one queue for beacons). For queue setup
 * on newer chips check out qcu.c. Returns -EINVAL if queue number is out
 * of range or if queue is already disabled.
 *
 * NOTE: Must be called after setting up tx control descriptor for that
 * queue (see below).
 */
int ath5k_hw_start_tx_dma(struct ath5k_hw *ah, unsigned int queue)
{
	u32 tx_queue;

	ATH5K_TRACE(ah->ah_sc);
	AR5K_ASSERT_ENTRY(queue, ah->ah_capabilities.cap_queues.q_tx_num);

	/* Return if queue is declared inactive */
	if (ah->ah_txq[queue].tqi_type == AR5K_TX_QUEUE_INACTIVE)
		return -EIO;

	if (ah->ah_version == AR5K_AR5210) {
		tx_queue = ath5k_hw_reg_read(ah, AR5K_CR);

		/*
		 * Set the queue by type on 5210
		 */
		switch (ah->ah_txq[queue].tqi_type) {
		case AR5K_TX_QUEUE_DATA:
			tx_queue |= AR5K_CR_TXE0 & ~AR5K_CR_TXD0;
			break;
		case AR5K_TX_QUEUE_BEACON:
			tx_queue |= AR5K_CR_TXE1 & ~AR5K_CR_TXD1;
			ath5k_hw_reg_write(ah, AR5K_BCR_TQ1V | AR5K_BCR_BDMAE,
					AR5K_BSR);
			break;
		case AR5K_TX_QUEUE_CAB:
			tx_queue |= AR5K_CR_TXE1 & ~AR5K_CR_TXD1;
			ath5k_hw_reg_write(ah, AR5K_BCR_TQ1FV | AR5K_BCR_TQ1V |
				AR5K_BCR_BDMAE, AR5K_BSR);
			break;
		default:
			return -EINVAL;
		}
		/* Start queue */
		ath5k_hw_reg_write(ah, tx_queue, AR5K_CR);
		ath5k_hw_reg_read(ah, AR5K_CR);
	} else {
		/* Return if queue is disabled */
		if (AR5K_REG_READ_Q(ah, AR5K_QCU_TXD, queue))
			return -EIO;

		/* Start queue */
		AR5K_REG_WRITE_Q(ah, AR5K_QCU_TXE, queue);
	}

	return 0;
}

/**
 * ath5k_hw_stop_tx_dma - Stop DMA transmit on a specific queue
 *
 * @ah: The &struct ath5k_hw
 * @queue: The hw queue number
 *
 * Stop DMA transmit on a specific hw queue and drain queue so we don't
 * have any pending frames. Returns -EBUSY if we still have pending frames,
 * -EINVAL if queue number is out of range.
 *
 * TODO: Test queue drain code
 */
int ath5k_hw_stop_tx_dma(struct ath5k_hw *ah, unsigned int queue)
{
	unsigned int i = 100;
	u32 tx_queue, pending;

	ATH5K_TRACE(ah->ah_sc);
	AR5K_ASSERT_ENTRY(queue, ah->ah_capabilities.cap_queues.q_tx_num);

	/* Return if queue is declared inactive */
	if (ah->ah_txq[queue].tqi_type == AR5K_TX_QUEUE_INACTIVE)
		return -EIO;

	if (ah->ah_version == AR5K_AR5210) {
		tx_queue = ath5k_hw_reg_read(ah, AR5K_CR);

		/*
		 * Set by queue type
		 */
		switch (ah->ah_txq[queue].tqi_type) {
		case AR5K_TX_QUEUE_DATA:
			tx_queue |= AR5K_CR_TXD0 & ~AR5K_CR_TXE0;
			break;
		case AR5K_TX_QUEUE_BEACON:
		case AR5K_TX_QUEUE_CAB:
			/* XXX Fix me... */
			tx_queue |= AR5K_CR_TXD1 & ~AR5K_CR_TXD1;
			ath5k_hw_reg_write(ah, 0, AR5K_BSR);
			break;
		default:
			return -EINVAL;
		}

		/* Stop queue */
		ath5k_hw_reg_write(ah, tx_queue, AR5K_CR);
		ath5k_hw_reg_read(ah, AR5K_CR);
	} else {
		/*
		 * Schedule TX disable and wait until queue is empty
		 */
		AR5K_REG_WRITE_Q(ah, AR5K_QCU_TXD, queue);

		/*Check for pending frames*/
		do {
			pending = ath5k_hw_reg_read(ah,
				AR5K_QUEUE_STATUS(queue)) &
				AR5K_QCU_STS_FRMPENDCNT;
			udelay(100);
		} while (--i && pending);

		/* Clear register */
		ath5k_hw_reg_write(ah, 0, AR5K_QCU_TXD);
		if (pending)
			return -EBUSY;
	}

	/* TODO: Check for success else return error */
	return 0;
}

/**
 * ath5k_hw_get_txdp - Get TX Descriptor's address for a specific queue
 *
 * @ah: The &struct ath5k_hw
 * @queue: The hw queue number
 *
 * Get TX descriptor's address for a specific queue. For 5210 we ignore
 * the queue number and use tx queue type since we only have 2 queues.
 * We use TXDP0 for normal data queue and TXDP1 for beacon queue.
 * For newer chips with QCU/DCU we just read the corresponding TXDP register.
 *
 * XXX: Is TXDP read and clear ?
 */
u32 ath5k_hw_get_txdp(struct ath5k_hw *ah, unsigned int queue)
{
	u16 tx_reg;

	ATH5K_TRACE(ah->ah_sc);
	AR5K_ASSERT_ENTRY(queue, ah->ah_capabilities.cap_queues.q_tx_num);

	/*
	 * Get the transmit queue descriptor pointer from the selected queue
	 */
	/*5210 doesn't have QCU*/
	if (ah->ah_version == AR5K_AR5210) {
		switch (ah->ah_txq[queue].tqi_type) {
		case AR5K_TX_QUEUE_DATA:
			tx_reg = AR5K_NOQCU_TXDP0;
			break;
		case AR5K_TX_QUEUE_BEACON:
		case AR5K_TX_QUEUE_CAB:
			tx_reg = AR5K_NOQCU_TXDP1;
			break;
		default:
			return 0xffffffff;
		}
	} else {
		tx_reg = AR5K_QUEUE_TXDP(queue);
	}

	return ath5k_hw_reg_read(ah, tx_reg);
}

/**
 * ath5k_hw_set_txdp - Set TX Descriptor's address for a specific queue
 *
 * @ah: The &struct ath5k_hw
 * @queue: The hw queue number
 *
 * Set TX descriptor's address for a specific queue. For 5210 we ignore
 * the queue number and we use tx queue type since we only have 2 queues
 * so as above we use TXDP0 for normal data queue and TXDP1 for beacon queue.
 * For newer chips with QCU/DCU we just set the corresponding TXDP register.
 * Returns -EINVAL if queue type is invalid for 5210 and -EIO if queue is still
 * active.
 */
int ath5k_hw_set_txdp(struct ath5k_hw *ah, unsigned int queue, u32 phys_addr)
{
	u16 tx_reg;

	ATH5K_TRACE(ah->ah_sc);
	AR5K_ASSERT_ENTRY(queue, ah->ah_capabilities.cap_queues.q_tx_num);

	/*
	 * Set the transmit queue descriptor pointer register by type
	 * on 5210
	 */
	if (ah->ah_version == AR5K_AR5210) {
		switch (ah->ah_txq[queue].tqi_type) {
		case AR5K_TX_QUEUE_DATA:
			tx_reg = AR5K_NOQCU_TXDP0;
			break;
		case AR5K_TX_QUEUE_BEACON:
		case AR5K_TX_QUEUE_CAB:
			tx_reg = AR5K_NOQCU_TXDP1;
			break;
		default:
			return -EINVAL;
		}
	} else {
		/*
		 * Set the transmit queue descriptor pointer for
		 * the selected queue on QCU for 5211+
		 * (this won't work if the queue is still active)
		 */
		if (AR5K_REG_READ_Q(ah, AR5K_QCU_TXE, queue))
			return -EIO;

		tx_reg = AR5K_QUEUE_TXDP(queue);
	}

	/* Set descriptor pointer */
	ath5k_hw_reg_write(ah, phys_addr, tx_reg);

	return 0;
}

/**
 * ath5k_hw_update_tx_triglevel - Update tx trigger level
 *
 * @ah: The &struct ath5k_hw
 * @increase: Flag to force increase of trigger level
 *
 * This function increases/decreases the tx trigger level for the tx fifo
 * buffer (aka FIFO threshold) that is used to indicate when PCU flushes
 * the buffer and transmits it's data. Lowering this results sending small
 * frames more quickly but can lead to tx underruns, raising it a lot can
 * result other problems (i think bmiss is related). Right now we start with
 * the lowest possible (64Bytes) and if we get tx underrun we increase it using
 * the increase flag. Returns -EIO if we have have reached maximum/minimum.
 *
 * XXX: Link this with tx DMA size ?
 * XXX: Use it to save interrupts ?
 * TODO: Needs testing, i think it's related to bmiss...
 */
int ath5k_hw_update_tx_triglevel(struct ath5k_hw *ah, bool increase)
{
	u32 trigger_level, imr;
	int ret = -EIO;

	ATH5K_TRACE(ah->ah_sc);

	/*
	 * Disable interrupts by setting the mask
	 */
	imr = ath5k_hw_set_imr(ah, ah->ah_imr & ~AR5K_INT_GLOBAL);

	trigger_level = AR5K_REG_MS(ath5k_hw_reg_read(ah, AR5K_TXCFG),
			AR5K_TXCFG_TXFULL);

	if (!increase) {
		if (--trigger_level < AR5K_TUNE_MIN_TX_FIFO_THRES)
			goto done;
	} else
		trigger_level +=
			((AR5K_TUNE_MAX_TX_FIFO_THRES - trigger_level) / 2);

	/*
	 * Update trigger level on success
	 */
	if (ah->ah_version == AR5K_AR5210)
		ath5k_hw_reg_write(ah, trigger_level, AR5K_TRIG_LVL);
	else
		AR5K_REG_WRITE_BITS(ah, AR5K_TXCFG,
				AR5K_TXCFG_TXFULL, trigger_level);

	ret = 0;

done:
	/*
	 * Restore interrupt mask
	 */
	ath5k_hw_set_imr(ah, imr);

	return ret;
}

/*******************\
* Interrupt masking *
\*******************/

/**
 * ath5k_hw_is_intr_pending - Check if we have pending interrupts
 *
 * @ah: The &struct ath5k_hw
 *
 * Check if we have pending interrupts to process. Returns 1 if we
 * have pending interrupts and 0 if we haven't.
 */
bool ath5k_hw_is_intr_pending(struct ath5k_hw *ah)
{
	ATH5K_TRACE(ah->ah_sc);
	return ath5k_hw_reg_read(ah, AR5K_INTPEND);
}

/**
 * ath5k_hw_get_isr - Get interrupt status
 *
 * @ah: The @struct ath5k_hw
 * @interrupt_mask: Driver's interrupt mask used to filter out
 * interrupts in sw.
 *
 * This function is used inside our interrupt handler to determine the reason
 * for the interrupt by reading Primary Interrupt Status Register. Returns an
 * abstract interrupt status mask which is mostly ISR with some uncommon bits
 * being mapped on some standard non hw-specific positions
 * (check out &ath5k_int).
 *
 * NOTE: We use read-and-clear register, so after this function is called ISR
 * is zeroed.
 *
 * XXX: Why filter interrupts in sw with interrupt_mask ? No benefit at all
 * plus it can be misleading (one might thing that we save interrupts this way)
 */
int ath5k_hw_get_isr(struct ath5k_hw *ah, enum ath5k_int *interrupt_mask)
{
	u32 data;

	ATH5K_TRACE(ah->ah_sc);

	/*
	 * Read interrupt status from the Interrupt Status register
	 * on 5210
	 */
	if (ah->ah_version == AR5K_AR5210) {
		data = ath5k_hw_reg_read(ah, AR5K_ISR);
		if (unlikely(data == AR5K_INT_NOCARD)) {
			*interrupt_mask = data;
			return -ENODEV;
		}
	} else {
		/*
		 * Read interrupt status from the Read-And-Clear
		 * shadow register.
		 * Note: PISR/SISR Not available on 5210
		 */
		data = ath5k_hw_reg_read(ah, AR5K_RAC_PISR);
	}

	/*
	 * Get abstract interrupt mask (driver-compatible)
	 */
	*interrupt_mask = (data & AR5K_INT_COMMON) & ah->ah_imr;

	if (unlikely(data == AR5K_INT_NOCARD))
		return -ENODEV;

	if (data & (AR5K_ISR_RXOK | AR5K_ISR_RXERR))
		*interrupt_mask |= AR5K_INT_RX;

	if (data & (AR5K_ISR_TXOK | AR5K_ISR_TXERR
		| AR5K_ISR_TXDESC | AR5K_ISR_TXEOL))
		*interrupt_mask |= AR5K_INT_TX;

	if (ah->ah_version != AR5K_AR5210) {
		/*HIU = Host Interface Unit (PCI etc)*/
		if (unlikely(data & (AR5K_ISR_HIUERR)))
			*interrupt_mask |= AR5K_INT_FATAL;

		/*Beacon Not Ready*/
		if (unlikely(data & (AR5K_ISR_BNR)))
			*interrupt_mask |= AR5K_INT_BNR;
	}

	/*
	 * XXX: BMISS interrupts may occur after association.
	 * I found this on 5210 code but it needs testing. If this is
	 * true we should disable them before assoc and re-enable them
	 * after a successfull assoc + some jiffies.
	 */
#if 0
	interrupt_mask &= ~AR5K_INT_BMISS;
#endif

	/*
	 * In case we didn't handle anything,
	 * print the register value.
	 */
	if (unlikely(*interrupt_mask == 0 && net_ratelimit()))
		ATH5K_PRINTF("0x%08x\n", data);

	return 0;
}

/**
 * ath5k_hw_set_imr - Set interrupt mask
 *
 * @ah: The &struct ath5k_hw
 * @new_mask: The new interrupt mask to be set
 *
 * Set the interrupt mask in hw to save interrupts. We do that by mapping
 * ath5k_int bits to hw-specific bits to remove abstraction and writing
 * Interrupt Mask Register.
 */
enum ath5k_int ath5k_hw_set_imr(struct ath5k_hw *ah, enum ath5k_int new_mask)
{
	enum ath5k_int old_mask, int_mask;

	/*
	 * Disable card interrupts to prevent any race conditions
	 * (they will be re-enabled afterwards).
	 */
	ath5k_hw_reg_write(ah, AR5K_IER_DISABLE, AR5K_IER);
	ath5k_hw_reg_read(ah, AR5K_IER);

	old_mask = ah->ah_imr;

	/*
	 * Add additional, chipset-dependent interrupt mask flags
	 * and write them to the IMR (interrupt mask register).
	 */
	int_mask = new_mask & AR5K_INT_COMMON;

	if (new_mask & AR5K_INT_RX)
		int_mask |= AR5K_IMR_RXOK | AR5K_IMR_RXERR | AR5K_IMR_RXORN |
			AR5K_IMR_RXDESC;

	if (new_mask & AR5K_INT_TX)
		int_mask |= AR5K_IMR_TXOK | AR5K_IMR_TXERR | AR5K_IMR_TXDESC |
			AR5K_IMR_TXURN;

	if (ah->ah_version != AR5K_AR5210) {
		if (new_mask & AR5K_INT_FATAL) {
			int_mask |= AR5K_IMR_HIUERR;
			AR5K_REG_ENABLE_BITS(ah, AR5K_SIMR2, AR5K_SIMR2_MCABT |
					AR5K_SIMR2_SSERR | AR5K_SIMR2_DPERR);
		}
	}

	ath5k_hw_reg_write(ah, int_mask, AR5K_PIMR);

	/* Store new interrupt mask */
	ah->ah_imr = new_mask;

	/* ..re-enable interrupts */
	ath5k_hw_reg_write(ah, AR5K_IER_ENABLE, AR5K_IER);
	ath5k_hw_reg_read(ah, AR5K_IER);

	return old_mask;
}
