/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing       */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

/* 
    Based on code and information provided in the Youtube series associated 
    with https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ 
    the Fat Fs SD card system was implemented using the Elm Chan Fat Fs SD card 
    module below by Drew Cochrane. 
 */

#ifdef __XC16 
#ifndef FCY //Must be defined for the __delay_**() functions otherwise project will not build
#define FCY (_XTAL_FREQ/2)
#endif
#include <libpic30.h>
#endif

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "mcc_generated_files/mcc.h" /* Generated files for SPI and GPIO */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

/* Definitions for MMC/SDC command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */

//Note these are static variables - value will remain constant no matter when called
static  DSTATUS Stat = STA_NOINIT; /* Disk status */
static BYTE CardType; /* Card type flags */

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus        
 * Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
static
void deselect (void)
{
	SD_CS_SetHigh();	/* Set CS# high */
	sd_rx();	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready
 * Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
static
BYTE wait_ready (void)	/* 1:Ready, 0:Timeout */
{
	UINT tmr;

	for (tmr = 5000; tmr; tmr--) {	/* Wait for ready in timeout of 500ms */
		if (sd_rx() == 0xFF) break; //Card will send 0xFF when it is ready
		__delay_us(100);
	}

	return tmr ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/* Modified by Drew Cochrane with help from https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ */
/*-----------------------------------------------------------------------*/
static
BYTE select (void)	/* 1:Successful, 0:Timeout */
{
	SD_CS_SetLow();	/* Set CS# low */
	sd_rx();	/* Dummy clock (force DO enabled) */
	if (wait_ready()) return 1;	/* Wait for card ready */

	deselect();
	return 0;	/* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from SD Card                                    */
/* Modified by Drew Cochrane with help from https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
static
BYTE rcvr_datablock (
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count (must be multiple of 4) */
)
{
	BYTE token;
	UINT tmr;

	for (tmr = 2000; tmr; tmr--) {	/* Wait for data packet in timeout of 200ms */
		token = sd_rx();
		if (token != 0xFF) break; //A packet is received
		__delay_us(100);
	}
	if (token != 0xFE) return 0;	/* If not valid data token, return with error */

	do
		*buff++ = sd_rx();		/* Receive the data block into buffer */
	while (--btr);
	sd_rx();					/* Discard CRC */
	sd_rx();

	return 1;					/* Return with success */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to SD Card */
/* Modified by Drew Cochrane with help from https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
static
BYTE xmit_datablock (
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE resp;
	WORD i;


	if (!wait_ready()) return 0;

	sd_tx(token);			/* Xmit data token */
	if (token != 0xFD) {	/* Is data token */
		i = 512;
		do
			sd_tx(*buff++);				/* Xmit the data block to the MMC */
		while (--i);
		sd_rx();						/* CRC (Dummy) */
		sd_rx();
		resp = sd_rx();					/* Receive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return 0;
	}

	return 1;
}

static
BYTE send_cmd (		/* Returns R1 resp (bit7==1:Send failed) */
	BYTE cmd,		/* Command index */
	DWORD arg		/* Argument */
)
{
	BYTE res, n;
    
	if (cmd & 0x80) {	/* ACMD<n> is the command sequence of CMD55-CMD<n> */
		cmd &= 0x7F;

		res = send_cmd(CMD55, 0);

		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		deselect();
		if (!select()) return 0xFF;
	}

	/* Send command packet */
	sd_tx(0x40 | cmd);				/* Start + Command index */
	sd_tx((BYTE)(arg >> 24));		/* Argument[31..24] */
	sd_tx((BYTE)(arg >> 16));		/* Argument[23..16] */
	sd_tx((BYTE)(arg >> 8));		/* Argument[15..8] */
	sd_tx((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* Valid CRC for CMD0(0) + Stop */
	if (cmd == CMD8) n = 0x87;		/* Valid CRC for CMD8(0x1AA) Stop */
	sd_tx(n);

	/* Receive command response */
	if (cmd == CMD12) sd_rx();		/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = sd_rx();
	while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}

/*
 The public functions - use the private helper functions above to accomplish
 their main tasks...
 */

/*-----------------------------------------------------------------------*/
/* Get drive status                                                      */
/* Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
    if (pdrv) /* Physical drive number (0) */
        return STA_NOINIT; /* Supports only single drive */
    
    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/* Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive
                             * In this case should be drive number 0 */
)
{
    BYTE n, cmd, ty, ocr[4];
	UINT tmr;

	if (pdrv) return STA_NOINIT;		/* Supports only single drive */

	if (Stat & STA_NODISK) return Stat;	/* No card in the socket */

	sd_init();
	for (n = 10; n; n--) sd_rx();	/* 80 dummy clocks */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			for (n = 0; n < 4; n++) ocr[n] = sd_rx();	/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_cmd(ACMD41, 1UL << 30) == 0) break;
					__delay_ms(1);
				}
				if (tmr && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = sd_rx();
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state */
				if (send_cmd(cmd, 0) == 0) break;
				__delay_ms(1);
			}
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	deselect();

	if (ty) {			/* Initialization succeeded */
		Stat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
		sd_open();
	}

	return Stat;   
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/* Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	BYTE cmd;

	if (pdrv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	cmd = count > 1 ? CMD18 : CMD17;			/*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
	if (send_cmd(cmd, sector) == 0) {
		do {
			if (!rcvr_datablock(buff, 512)) break;
			buff += 512;
		} while (--count);
		if (cmd == CMD18) send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */
	}
	deselect();

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/* Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0 //In the example code this was _USE_WRITE 

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (pdrv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/* Modified by Drew Cochrane with reference to https://www.studentcompanion.co.za/interfacing-sd-card-with-pic-microcontroller-xc8/ and associated example code */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16], *ptr = buff;
	DWORD csize;


	if (pdrv) return RES_PARERR;

	res = RES_ERROR;

	if (Stat & STA_NOINIT) return RES_NOTRDY;

	switch (cmd) {
	case CTRL_SYNC :		/* Make sure that no pending write process. Do not remove this or written sector might not left updated. */
		if (select()) res = RES_OK;
		break;

	case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
		if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
			if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
				csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
				*(DWORD*)buff = csize << 10;
			} else {					/* SDC ver 1.XX or MMC*/
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(DWORD*)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
		if (CardType & CT_SD2) {	/* SDv2? */
			if (send_cmd(ACMD13, 0) == 0) {	/* Read SD status */
				sd_rx();
				if (rcvr_datablock(csd, 16)) {				/* Read partial block */
					for (n = 64 - 16; n; n--) sd_rx();	/* Purge trailing data */
					*(DWORD*)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		} else {					/* SDv1 or MMCv3 */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {	/* Read CSD */
				if (CardType & CT_SD1) {	/* SDv1 */
					*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				} else {					/* MMCv3 */
					*(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
				}
				res = RES_OK;
			}
		}
		break;

	/* Following commands are never used by FatFs module */

	case MMC_GET_TYPE :		/* Get card type flags (1 byte) */
		*ptr = CardType;
		res = RES_OK;
		break;

	case MMC_GET_CSD :		/* Receive CSD as a data block (16 bytes) */
		if (send_cmd(CMD9, 0) == 0		/* READ_CSD */
			&& rcvr_datablock(ptr, 16))
			res = RES_OK;
		break;

	case MMC_GET_CID :		/* Receive CID as a data block (16 bytes) */
		if (send_cmd(CMD10, 0) == 0		/* READ_CID */
			&& rcvr_datablock(ptr, 16))
			res = RES_OK;
		break;

	case MMC_GET_OCR :		/* Receive OCR as an R3 resp (4 bytes) */
		if (send_cmd(CMD58, 0) == 0) {	/* READ_OCR */
			for (n = 4; n; n--) *ptr++ = sd_rx();
			res = RES_OK;
		}
		break;

	case MMC_GET_SDSTAT :	/* Receive SD status as a data block (64 bytes) */
		if (send_cmd(ACMD13, 0) == 0) {	/* SD_STATUS */
			sd_rx();
			if (rcvr_datablock(ptr, 64))
				res = RES_OK;
		}
		break;

	default:
		res = RES_PARERR;
	}

	deselect();

	return res;
}





