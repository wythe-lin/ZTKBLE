/*
 ******************************************************************************
 * gplink.c - GP link interface
 *
 * Copyright (c) 2014-2016 by ZealTek Electronic Co., Ltd.
 *
 * This software is copyrighted by and is the property of ZealTek
 * Electronic Co., Ltd. All rights are reserved by ZealTek Electronic
 * Co., Ltd. This software may only be used in accordance with the
 * corresponding license agreement. Any unauthorized use, duplication,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice MUST not be removed or modified without prior
 * written consent of ZealTek Electronic Co., Ltd.
 *
 * ZealTek Electronic Co., Ltd. reserves the right to modify this
 * software without notice.
 *
 * History:
 *	2015.03.17	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */

/*
 ******************************************************************************
 *
 * includes
 *
 ******************************************************************************
 */
#include <ioCC2541.h>

#include "hal_mcu.h"
#include "OSAL.h"
#include "gplink.h"

#include "wcube.h"

/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define DBG_MSG			1
#if (DBG_MSG == 1)
    #include <stdio.h>
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif

/*
 *****************************************************************************
 *
 * constants
 *
 *****************************************************************************
 */
/*
 * Fsys     = 32MHz
 * Baudrate = 115200
 * M = 216, E = 11
 *
 * Baudrate = 9600
 * M = 59, E = 8
 */
#define UART_BAUD_M		216
#define UART_BAUD_E		11


/*
 ******************************************************************************
 *
 * global variables
 *
 ******************************************************************************
 */
unsigned char		gplink_handle;
static unsigned char	gplink_parse;


/*
 ******************************************************************************
 *
 * local variables
 *
 ******************************************************************************
 */
#define TXBUF_SIZE		128
#define RXBUF_SIZE		128

struct kfifo {
	unsigned char	*buffer;	/* the buffer holding the data */
	unsigned char	size;		/* the size of the allocated buffer */
	unsigned char	in;		/* data is added at offset (in % size) */
	unsigned char	out;		/* data is extracted from off. (out % size) */
};

static struct kfifo	txqueue; 
static struct kfifo	rxqueue;
static unsigned char	txbuf[TXBUF_SIZE];
static unsigned char	rxbuf[RXBUF_SIZE];


/*
 ******************************************************************************
 *
 * local functions
 *
 ******************************************************************************
 */
#define min			MIN

/**
 * kfifo_reset - removes the entire FIFO contents
 * @fifo: the fifo to be emptied.
 */
static inline void kfifo_reset(struct kfifo *fifo)
{
	fifo->in = fifo->out = 0;
}

static void kfifo_init(struct kfifo *fifo, void *buffer, unsigned char size)
{
	fifo->buffer = buffer;
	fifo->size   = size;

	kfifo_reset(fifo);
}

/**
 * kfifo_size - returns the size of the fifo in bytes
 * @fifo: the fifo to be used.
 */
static inline unsigned char kfifo_size(struct kfifo *fifo)
{
	return fifo->size;
}

/**
 * kfifo_len - returns the number of used bytes in the FIFO
 * @fifo: the fifo to be used.
 */
static inline unsigned char kfifo_len(struct kfifo *fifo)
{
	register unsigned char	out;

	out = fifo->out;
	return fifo->in - out;
}

/**
 * kfifo_is_empty - returns true if the fifo is empty
 * @fifo: the fifo to be used.
 */
static inline char kfifo_is_empty(struct kfifo *fifo)
{
	return fifo->in == fifo->out;
}

/**
 * kfifo_is_full - returns true if the fifo is full
 * @fifo: the fifo to be used.
 */
static inline char kfifo_is_full(struct kfifo *fifo)
{
	return kfifo_len(fifo) == kfifo_size(fifo);
}

/*
 * kfifo_add_out internal helper function for updating the out offset
 */
static inline void __kfifo_add_out(struct kfifo *fifo, unsigned char off)
{
	fifo->out += off;
}

/*
 * kfifo_add_in internal helper function for updating the in offset
 */
static inline void __kfifo_add_in(struct kfifo *fifo, unsigned char off)
{
	fifo->in += off;
}

/*
 * __kfifo_off internal helper function for calculating the index of a
 * given offeset
 */
static inline unsigned char __kfifo_off(struct kfifo *fifo, unsigned char off)
{
	return off & (fifo->size - 1);
}

/**
 * kfifo_avail - returns the number of bytes available in the FIFO
 * @fifo: the fifo to be used.
 */
static inline unsigned char kfifo_avail(struct kfifo *fifo)
{
	return kfifo_size(fifo) - kfifo_len(fifo);
}


static inline void __kfifo_in_data(struct kfifo *fifo, const void *from, unsigned char len)
{
	unsigned char	l;                                                        
	unsigned char	off;

	/*
	 * Ensure that we sample the fifo->out index -before- we
	 * start putting bytes into the kfifo.
	 */
	off = __kfifo_off(fifo, fifo->in);

	/* first put the data starting from fifo->in to buffer end */
	l = min(len, fifo->size - off);
	osal_memcpy(fifo->buffer + off, from, l);

	/* then put the rest (if any) at the beginning of the buffer */
	osal_memcpy(fifo->buffer, (char *) from + l, len - l);
}

static inline void __kfifo_out_data(struct kfifo *fifo, void *to, unsigned char len)
{
	unsigned char	l;
	unsigned char	off;

	/*
	 * Ensure that we sample the fifo->in index -before- we
	 * start removing bytes from the kfifo.
	 */
	off = __kfifo_off(fifo, fifo->out);

	/* first get the data from fifo->out until the end of the buffer */
	l = min(len, fifo->size - off);
	osal_memcpy(to, fifo->buffer + off, l);

	/* then get the rest (if any) from the beginning of the buffer */
	osal_memcpy((char *) to + l, fifo->buffer, len - l);

}


/**
 * kfifo_in - puts some data into the FIFO
 * @fifo: the fifo to be used.
 * @from: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @from buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 */
unsigned char kfifo_in(struct kfifo *fifo, const void *from, unsigned char len)
{
	len = min(kfifo_avail(fifo), len);

	__kfifo_in_data(fifo, from, len);
	__kfifo_add_in(fifo, len);
	return len;
}

/**
 * kfifo_out - gets some data from the FIFO
 * @fifo: the fifo to be used.
 * @to: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @to buffer and returns the number of copied bytes.
 */
unsigned char kfifo_out(struct kfifo *fifo, void *to, unsigned char len)
{
	len = min(kfifo_len(fifo), len);

	__kfifo_out_data(fifo, to, len);
	__kfifo_add_out(fifo, len);
	return len;
}


/*
 ******************************************************************************
 *
 * global functions
 *
 ******************************************************************************
 */
void gplink_init(void)
{
	// init ring buffer
	kfifo_init(&txqueue, txbuf, TXBUF_SIZE);
	kfifo_init(&rxqueue, rxbuf, RXBUF_SIZE);

	gplink_handle = 0;
	gplink_parse  = 0;

	// init UART port
	PERCFG     &= ~(1 << 1);
	P0SEL      |=  (0x0F << 2);
	P2DIR      &= ~(0xC0);
	P2DIR      |=  (0x40);

	U1CSR      |=  (1 << 7);
	U1GCR      &= ~(0x1F);
	U1GCR      |=  UART_BAUD_E;
	U1BAUD      =  UART_BAUD_M;
	U1CSR      |=  (1 << 6);

	// interrupt
	IEN2	   |=  (1 << 3);	// tx
	URX1IE	    =  1;		// rx
}


HAL_ISR_FUNCTION(gplink_tx_isr, UTX1_VECTOR)
{
	unsigned char	val;

	HAL_ENTER_ISR();
        
	if (kfifo_out(&txqueue, &val, 1)) {
		U1DBUF = val;
	} else {
		UTX1IF = 0;
	}


	HAL_EXIT_ISR();
}

extern void	wCube_SetRxEvent(void);
#define set_rx_event	wCube_SetRxEvent

HAL_ISR_FUNCTION(gplink_rx_isr, URX1_VECTOR)
{
	unsigned char	val;

	HAL_ENTER_ISR();

	val = U1DBUF;
	kfifo_in(&rxqueue, &val, 1);

	set_rx_event();

	HAL_EXIT_ISR();
}




/**
 * @fn      gplink_send_pkt
 *
 * @brief   gen packet
 *
 * @param   src - 
 * @param   len - 
 *
 * @return  none
 */
void gplink_send_pkt(const void *src, unsigned char len)
{
	register unsigned char	n;

	n = kfifo_in(&txqueue, src, len);
	if (len != n) {
		dmsg(("txqueue: the free space is not enough.\n"));
		return;
	}

	// startup UART
	UTX1IF = 1;

}


/**
 * @fn      gplink_recv_pkt
 *
 * @brief   gen packet
 *
 * @param   src - 
 * @param   len - 
 *
 * @return  none
 */
unsigned char gplink_recv_pkt(void *buf)
{
	unsigned char	tmp;
	unsigned char	*pkt = (unsigned char *) buf;

	switch (gplink_parse) {
	case 0:	// get leading code
		if (kfifo_len(&rxqueue) == 0) {
			return 0;	// length not enough
		}
		kfifo_out(&rxqueue, &tmp, 1);
		if (tmp == 0xfa) {
			pkt[0] = tmp;
			gplink_parse = 1;
		}
		break;

	case 1:	// get length
		if (kfifo_len(&rxqueue)) {
			kfifo_out(&rxqueue, &tmp, 1);
			if (tmp < RXBUF_SIZE*2/3) {
				pkt[1] = tmp;
				gplink_parse = 2;
			} else {
				gplink_parse = 0;	// length fail, re-get leading code
			}
		}
		break;

	case 2:	// get packet content
		if (kfifo_len(&rxqueue) >= (pkt[1]-2)) {
			kfifo_out(&rxqueue, &pkt[2], pkt[1]-2);
			gplink_parse = 3;
			return 2;	// finish
		}
		break;

	// waiting for <ble->app>
	default:
		break;
	}

	return 1;	// continue
}

/**
 * @fn      gplink_genpkt
 *
 * @brief   gen packet
 *
 * @param   src - 
 * @param   len - 
 *
 * @return  none
 */
void gplink_rst_parse(void)
{
	gplink_parse = 0;
}

