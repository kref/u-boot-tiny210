/*
 * Ma Haijun <mahaijuns@gmail.com>
 *
 * Samsung S5PV210 BL1 NAND driver
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <common.h>
#include <asm/io.h>

#define NFCONF	((volatile void *)0xB0E00000)
#define NFCONT	((volatile void *)0xB0E00004)
#define NFCMMD	((volatile void *)0xB0E00008)
#define NFADDR	((volatile void *)0xB0E0000C)
#define NFDATA	((volatile void *)0xB0E00010)
#define NFSTAT	((volatile void *)0xB0E00028)

#define NFECCCONF	((volatile void *)0xB0E20000)
#define NFECCCONT	((volatile void *)0xB0E20020)
#define NFECCSTAT	((volatile void *)0xB0E20030)
#define NFECCSECSTAT	((volatile void *)0xB0E20040)

#define NFECCERL0	((volatile void *)(0xB0E200C0 + 0 * 4))
#define NFECCERL1	((volatile void *)(0xB0E200C0 + 1 * 4))
#define NFECCERL2	((volatile void *)(0xB0E200C0 + 2 * 4))
#define NFECCERL3	((volatile void *)(0xB0E200C0 + 3 * 4))
#define NFECCERL4	((volatile void *)(0xB0E200C0 + 4 * 4))
#define NFECCERL5	((volatile void *)(0xB0E200C0 + 5 * 4))
#define NFECCERL6	((volatile void *)(0xB0E200C0 + 6 * 4))
#define NFECCERL7	((volatile void *)(0xB0E200C0 + 7 * 4))

#define NFECCERP0	((volatile void *)(0xB0E200F0 + 0 * 4))
#define NFECCERP1	((volatile void *)(0xB0E200F0 + 1 * 4))
#define NFECCERP2	((volatile void *)(0xB0E200F0 + 2 * 4))
#define NFECCERP3	((volatile void *)(0xB0E200F0 + 3 * 4))

#define NFCONF_TACLS(x)		((x)<<12)
#define NFCONF_TACLS_MASK	(0xF<<12)
#define NFCONF_TWRPH0(x)		((x)<<8)
#define NFCONF_TWRPH0_MASK	(0xF<<8)
#define NFCONF_TWRPH1(x)		((x)<<4)
#define NFCONF_TWRPH1_MASK	(0xF<<4)
#define NFCONF_ECC_1BIT		(0<<23)
#define NFCONF_ECC_4BIT		(2<<23)
#define NFCONF_ECC_DISABLE	(3<<23)
#define NFCONF_ECC_MASK		(3<<23)

#define NFCONT_ECC_ENC			(1<<18)
#define NFCONT_ENABLE_ILEGL_ACC_INT	(1<<10)
#define NFCONT_ENABLE_RNB_INT		(1<<9)
#define NFCONT_MECCLOCK			(1<<7)
#define NFCONT_SECCLOCK			(1<<6)
#define NFCONT_INITMECC			(1<<5)
#define NFCONT_INITSECC			(1<<4)
#define NFCONT_nFCE1			(1<<2)
#define NFCONT_nFCE0			(1<<1)
#define NFCONT_ENABLE			(1<<0)

#define NFSTAT_ECCENCDONE	(1<<7)
#define NFSTAT_ECCDECDONE	(1<<6)
#define NFSTAT_ILEGL_ACC		(1<<5)
#define NFSTAT_RnB_CHANGE	(1<<4)
#define NFSTAT_nFCE1		(1<<3)
#define NFSTAT_nFCE0		(1<<2)
#define NFSTAT_Res1			(1<<1)
#define NFSTAT_READY		(1<<0)
#define NFSTAT_BUSY		(1<<0)

#define NFECCERR0_ECCBUSY	(1<<31)

#define NFECCCONF_ECCTYPE		(0xf<<0)
#define NFECCCONF_ECC_DISABLE		(0<<0)
#define NFECCCONF_ECC_8BIT		(3<<0)
#define NFECCCONF_ECC_12BIT		(4<<0)
#define NFECCCONF_ECC_16BIT		(5<<0)
#define NFECCCONF_MSGLEN_MASK		(0x3FF<<16)
#define NFECCCONF_MSGLEN(len)		((len-1)<<16)

#define NFECCCONT_ECC_ENC		(1<<16)
#define NFECCCONT_ENALBE_MLC_ENC_INT	(1<<25)
#define NFECCCONT_ENALBE_MLC_DEC_INT	(1<<24)
#define NFECCCONT_INITMECC		(1<<2)

#define NFECCSTAT_FREE_PAGE		(1 << 8)
#define NFECCSTAT_DECDONE		(1 << 24)
#define NFECCSTAT_ENCDONE		(1 << 25)
#define NFECCSTAT_BUSY			(1 << 31)

#define NFECCSECSTAT_VALD_ERROR_STAT_SHIFT	8
#define NFECCSECSTAT_ERROR_NO_MASK		0x1F

#define NAND_CMD_READ0          0
#define NAND_CMD_RNDOUT         5
/* Extended commands for large page devices */
#define NAND_CMD_READSTART      0x30
#define NAND_CMD_RNDOUTSTART    0xE0

#define OM_STAT	((volatile void *)0xE010E100)

/* Nand 2KB, 5cycle (Nand 8bit ECC) */
#define OM_NAND_2K_5C_8E		0b0001

/* Nand 4KB, 5cycle (Nand 8bit ECC) */
#define OM_NAND_4K_5C_8E		0b0010

/* Nand 4KB, 5cycle (Nand 16bit ECC) */
#define OM_NAND_4K_5C_16E	0b0011

/* Nand 2KB, 5cycle (16-bit bus, 4-bit ECC) */
#define OM_NAND_2K_5C_4E		0b1000

/* Nand 2KB, 4cycle (Nand 8bit ECC) */
#define OM_NAND_2K_4C_8E		0b1001


#define NAND_BL1_PAGE_SIZE  (4 * 1024)

#ifdef CONFIG_SPL_NAND_8K_PAGE
	#define NAND_BL2_PAGE_SIZE  (8 * 1024)
	#define BOOT_PARTITION_SIZE (1024 * 1024)
	/* the kernel oob layout's eccpos start at 36 */
	#define OOB_ECC_OFFSET	36
#else
	/* irom support up to 4K page size */
	#define NAND_BL2_PAGE_SIZE  (4 * 1024)
	/* 1MiB but only half is used */
	#define BOOT_PARTITION_SIZE (1024 * 1024 / 2)
	/* irom compatible */
	#define OOB_ECC_OFFSET	12
#endif

#define MESSAGE_LENGTH	512

#define BOOT_BLOCK	1


inline void nand_command(u8 cmd)
{
	writeb(cmd, NFCMMD);
}

inline void nand_address(u8 addr)
{
	writeb(addr, NFADDR);
}

inline void nand_select_chip(void)
{
	u32 reg;

	reg = readl(NFCONT);
	/* select chip */
	reg &= ~NFCONT_nFCE0;
	writel(reg, NFCONT);

	/* clear these bits. note they are write 1 clear bits*/
	reg = NFSTAT_RnB_CHANGE;
	writel(reg, NFSTAT);
}

inline void nand_unselect_chip(void)
{
	u32 reg;

	reg = readl(NFCONT);
	reg |= NFCONT_nFCE0;
	writel(reg, NFCONT);
}

void nand_seek(u32 column_addr)
{
	nand_command(NAND_CMD_RNDOUT);
	/* 2 bytes column address */
	nand_address((u8)column_addr);
	nand_address((u8)(column_addr >> 8));
	nand_command(NAND_CMD_RNDOUTSTART);
}

void nand_start_main_ecc(void)
{
	u32 reg;

	reg = readl(NFCONT);
	reg &= ~NFCONT_MECCLOCK;
	reg |= NFCONT_INITMECC;
	reg |= NFCONT_INITSECC;
	writel(reg, NFCONT);

	reg = readl(NFECCCONT);
	reg |= NFECCCONT_INITMECC;
	writel(reg, NFECCCONT);
}

void nand_lock_main_ecc(void)
{
	u32 reg;

	reg = readl(NFCONT);
	reg |= NFCONT_MECCLOCK;
	writel(reg, NFCONT);
}

void wait_and_clear(volatile void *reg_addr, u32 flags)
{
	while (!(readl(reg_addr) & flags));	/* spin untill bit set */
	/* write 1 to clear */
	writel(flags, reg_addr);
}

void nand_init_ecc(int message_length, int ecc_strength)
{
	u32 reg;

	reg = readl(NFCONF);
	/* disable 1/4 bit ecc */
	reg &= ~NFCONF_ECC_MASK;
	reg |= NFCONF_ECC_DISABLE;
	/* timing */
	reg &= ~NFCONF_TACLS_MASK;
	reg |= NFCONF_TACLS(7);
	reg &= ~NFCONF_TWRPH0_MASK;
	reg |= NFCONF_TWRPH0(7);
	reg &= ~NFCONF_TWRPH1_MASK;
	reg |= NFCONF_TWRPH1(7);
	writel(reg, NFCONF);

	reg = readl(NFCONT);
	/* set ecc direction to decode */
	reg &= ~NFCONT_ECC_ENC;
	reg |= NFCONT_ENABLE;
	reg |= NFCONT_SECCLOCK;
	reg &= ~NFCONT_ENABLE_ILEGL_ACC_INT;
	reg &= ~NFCONT_ENABLE_RNB_INT;
	writel(reg, NFCONT);

	reg = readl(NFSTAT);
	/* clear these bits. note they are write 1 clear bits*/
	reg |= NFSTAT_ECCENCDONE;
	reg |= NFSTAT_ECCDECDONE;
	reg |= NFSTAT_ILEGL_ACC;
	reg |= NFSTAT_RnB_CHANGE;
	writel(reg, NFSTAT);

	reg = readl(NFECCCONF);
	/* set message length */
	reg &= ~NFECCCONF_MSGLEN_MASK;
	reg |= NFECCCONF_MSGLEN(message_length);
	reg &= ~NFECCCONF_ECCTYPE;
	if (ecc_strength ==16)
		reg |= NFECCCONF_ECC_16BIT;
	else
		reg |= NFECCCONF_ECC_8BIT;
	writel(reg, NFECCCONF);

	reg = readl(NFECCCONT);
	/* set ecc engine direction to decode */
	reg &= ~NFECCCONT_ECC_ENC;
	reg &= ~NFECCCONT_ENALBE_MLC_ENC_INT;
	reg &= ~NFECCCONT_ENALBE_MLC_DEC_INT;
	writel(reg, NFECCCONT);

	reg = readl(NFECCSTAT);
	/* clear these bits */
	reg |= NFECCSTAT_ENCDONE;
	reg |= NFECCSTAT_DECDONE;
	writel(reg, NFECCSTAT);

	nand_lock_main_ecc();
}

/* returns:
 * 0	no error or all errors are corrected
 * 1	uncorrectable errors
 */
int nand_ecc_correct(u8 *buffer, int message_length, int ecc_strength)
{
	int error_count = 0;
	int i;
	u32 valid_mask;
	u32 reg;
	u32 nfeccerr[8];
	u32 nfeccerp[4];
	u16 location;
	u16 *loc = (u16 *) nfeccerr;
	u8 *pat = (u8 *) nfeccerp;

	reg = readl(NFECCSECSTAT);
	valid_mask = reg >> NFECCSECSTAT_VALD_ERROR_STAT_SHIFT;
	error_count = reg & NFECCSECSTAT_ERROR_NO_MASK;
	if (error_count == 0)
		return 0;
	if (error_count > ecc_strength)
		return 1;

	nfeccerr[0] = readl(NFECCERL0);
	nfeccerr[1] = readl(NFECCERL1);
	nfeccerr[2] = readl(NFECCERL2);
	nfeccerr[3] = readl(NFECCERL3);
	nfeccerr[4] = readl(NFECCERL4);
	nfeccerr[5] = readl(NFECCERL5);
	nfeccerr[6] = readl(NFECCERL6);
	nfeccerr[7] = readl(NFECCERL7);

	nfeccerp[0] = readl(NFECCERP0);
	nfeccerp[1] = readl(NFECCERP1);
	nfeccerp[2] = readl(NFECCERP2);
	nfeccerp[3] = readl(NFECCERP3);

	for (i = 0; i < 16; i++) {
		if (valid_mask & 1) {
			location = loc[i];
			if (location < message_length)
				buffer[location] ^= pat[i];
		}
		valid_mask >>= 1;
	}

	return 0;
}

void nand_init_param(
		int *page_size,
		int *addr_cycle,
		int *pages_per_block,
		int *message_length,
		int *ecc_strength,
		int *ecc_bytes)
{
	u32 boot_mode;	/* OM [4:1] */

	*message_length = MESSAGE_LENGTH;
	boot_mode = readl(OM_STAT);
	/*OM[0] determine clock type, not relevant here */
	boot_mode >>= 1;
	/*OM[5] means boot from uart/usb first, not relevant here */
	boot_mode &= 0xF;
	/*
	 * note: do not use switch case here, because compiler will generate
	 * jump table which is not position-independent by default.
	 * solution	1.turn on some compiler flag to make it position-independent,
	 *			fno-jump-tables or fpic
	 * 		2.link with proper load address (0xD002_0010),
	 *		3.avoid using switch, which is my choice now.
	 */
	if (boot_mode == OM_NAND_2K_5C_8E) {
		*page_size = 2048;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
	} else if (boot_mode == OM_NAND_2K_4C_8E) {
		*page_size = 2048;
		*addr_cycle = 4;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
	} else if (boot_mode == OM_NAND_4K_5C_8E) {
		*page_size = 4096;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  128;
	} else if (boot_mode == OM_NAND_4K_5C_16E) {
#ifdef CONFIG_SPL_NAND_8K_PAGE
		*page_size = 8192;
#else
		*page_size = 4096;
#endif
		*addr_cycle = 5;
		*ecc_strength = 16;
#ifdef CONFIG_SPL_NAND_8K_PAGE
		/* the kernel's oob layout aligns eccbytes up to 28/512 */
		*ecc_bytes = 28;
#else
		*ecc_bytes = 26;
#endif
		*pages_per_block =  128;
	} else { /* OM_NAND_2K_5C_4E ??? */
		*page_size = 2048;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
	}
}

void  nand_init_param_switch_case(
		int *page_size,
		int *addr_cycle,
		int *pages_per_block,
		int *message_length,
		int *ecc_strength,
		int *ecc_bytes)
{
	u32 boot_mode;	/* OM [4:1] */

	*message_length = MESSAGE_LENGTH;
	boot_mode = readl(OM_STAT);
	/*OM[0] determine clock type, not relevant here */
	boot_mode >>= 1;
	/*OM[5] means boot from uart/usb first, not relevant here */
	boot_mode &= 0xF;

	switch (boot_mode) {
	case OM_NAND_2K_5C_8E:
		*page_size = 2048;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
		break;
	case OM_NAND_2K_4C_8E:
		*page_size = 2048;
		*addr_cycle = 4;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
		break;
	case OM_NAND_4K_5C_8E:
		*page_size = 4096;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  128;
		break;
	case OM_NAND_4K_5C_16E:
		*page_size = 4096;
		*addr_cycle = 5;
		*ecc_strength = 16;
		*ecc_bytes = 26;
		*pages_per_block =  128;
		break;
	default: /* OM_NAND_2K_5C_4E ??? */
		*page_size = 2048;
		*addr_cycle = 5;
		*ecc_strength = 8;
		*ecc_bytes = 13;
		*pages_per_block =  64;
		break;
	}
}

int nand_read_page(u32 block, u32 page, u8 *buffer)
{
	/* nand parameters */
	int page_size;
	int addr_cycle;
	int pages_per_block;
	int message_length;
	int ecc_strength;	/* N bit ecc */
	int ecc_bytes;

	u32 row_addr;		/* page index */
	u32 column_addr;	/* offset in page */
	u32 ecc_column_addr;
	int i;
	int error_count = 0;

	nand_init_param(&page_size, &addr_cycle, &pages_per_block,
				&message_length, &ecc_strength, &ecc_bytes);
	row_addr = block * pages_per_block + page;

	nand_init_ecc(message_length, ecc_strength);
	nand_select_chip();
	for (column_addr = 0, ecc_column_addr = page_size + OOB_ECC_OFFSET;
		column_addr < page_size;
		column_addr += message_length, ecc_column_addr += ecc_bytes) {
		if (column_addr == 0){
			nand_command(NAND_CMD_READ0);
			/* 2 bytes column address */
			nand_address(0);
			nand_address(0);
			/* 2-3 bytes row address */
			nand_address((u8)row_addr);
			nand_address((u8)(row_addr >> 8));
			if (addr_cycle == 5)
				nand_address((u8)(row_addr >> 16));

			nand_command(NAND_CMD_READSTART);
			wait_and_clear(NFSTAT, NFSTAT_RnB_CHANGE);

		} else {
			nand_seek(column_addr);
		}
		nand_start_main_ecc();
		/* read data */
		for (i = 0; i < message_length; i++) {
			*buffer++ = readb(NFDATA);
		}
		/* read ecc */
		nand_seek(ecc_column_addr);
		for (i = 0; i < ecc_bytes; i++) {
			readb(NFDATA);
		}
		nand_lock_main_ecc();
		wait_and_clear(NFECCSTAT, NFECCSTAT_DECDONE);
		while (readl(NFECCSTAT) & NFECCSTAT_BUSY);	/* spin */

		error_count += nand_ecc_correct(buffer - message_length,
						message_length, ecc_strength);
	}
	nand_unselect_chip();

	return error_count;
}

/**
* This Function copies a block of page to destination memory.
* 8-Bit or 16-Bit ECC depend on boot mode
* @param uint32 block : Source block address number to copy.
* @param uint32 page : Source page address number to copy.
* @param uint8 *buffer : Target Buffer pointer.
* @return int32 - Success or failure.
*/
#define NF8_ReadPage_Adv(a,b,c) (((int(*)(u32, u32, u8*))(*((u32 *) 0xD0037F90)))(a,b,c))

void copy_uboot_to_ram(void)
{
	int page;
	u8 *buffer = (u8 *)CONFIG_SYS_TEXT_BASE;

	for (page = 0; /* BL2 is at erase block 1 while BL1 at block 0 */
		page < BOOT_PARTITION_SIZE / NAND_BL2_PAGE_SIZE;
		page++, buffer+= NAND_BL2_PAGE_SIZE)
	{
		/* TODO: check for error, and retry if applicable */
#ifdef CONFIG_SPL_NAND_IROM_HELPER
		NF8_ReadPage_Adv(BOOT_BLOCK, page, buffer);
#else
		nand_read_page(BOOT_BLOCK, page, buffer);
#endif
	}
}

void board_init_f(unsigned long bootflag)
{
        __attribute__((noreturn)) void (*uboot)(void);
        copy_uboot_to_ram();

        /* Jump to U-Boot image */
        uboot = (void *)CONFIG_SYS_TEXT_BASE;
        (*uboot)();
        /* Never returns Here */
}

/* Place Holders */
void board_init_r(gd_t *id, ulong dest_addr)
{
        /* Function attribute is no-return */
        /* This Function never executes */
        while (1)
                ;
}

void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3) {}

