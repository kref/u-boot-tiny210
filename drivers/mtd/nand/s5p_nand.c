/* linux/drivers/mtd/nand/s3c2410.c
 *
 * Copyright © 2004-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Samsung S3C2410/S3C2440/S3C2412 NAND driver
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
#define __io

#include <common.h>

#if defined(CONFIG_CMD_NAND)
#include <nand.h>
#include <s5pc110.h>

#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/clk.h>

#define S3C_NFREG(x) (x)

/* for S3C */
#define S3C_NFCONF		S3C_NFREG(0x00)
#define S3C_NFCONT		S3C_NFREG(0x04)
#define S3C_NFCMMD		S3C_NFREG(0x08)
#define S3C_NFADDR		S3C_NFREG(0x0c)
#define S3C_NFDATA8		S3C_NFREG(0x10)
#define S3C_NFDATA		S3C_NFREG(0x10)
#define S3C_NFMECCDATA0		S3C_NFREG(0x14)
#define S3C_NFMECCDATA1		S3C_NFREG(0x18)
#define S3C_NFSECCDATA		S3C_NFREG(0x1c)
#define S3C_NFSBLK		S3C_NFREG(0x20)
#define S3C_NFEBLK		S3C_NFREG(0x24)
#define S3C_NFSTAT		S3C_NFREG(0x28)
#define S3C_NFMECCERR0		S3C_NFREG(0x2c)
#define S3C_NFMECCERR1		S3C_NFREG(0x30)
#define S3C_NFMECC0		S3C_NFREG(0x34)
#define S3C_NFMECC1		S3C_NFREG(0x38)
#define S3C_NFSECC		S3C_NFREG(0x3c)
#define S3C_NFMLCBITPT		S3C_NFREG(0x40)

/* S5PV210 does not have these */
#define S3C_NF8ECCERR0		S3C_NFREG(0x44)
#define S3C_NF8ECCERR1		S3C_NFREG(0x48)
#define S3C_NF8ECCERR2		S3C_NFREG(0x4C)
#define S3C_NFM8ECC0		S3C_NFREG(0x50)
#define S3C_NFM8ECC1		S3C_NFREG(0x54)
#define S3C_NFM8ECC2		S3C_NFREG(0x58)
#define S3C_NFM8ECC3		S3C_NFREG(0x5C)
#define S3C_NFMLC8BITPT0	S3C_NFREG(0x60)
#define S3C_NFMLC8BITPT1	S3C_NFREG(0x64)
/* end S5PV210 does not have these */

#define S3C_NFCONF_NANDBOOT	(1<<31)
#define S3C_NFCONF_ECCCLKCON	(1<<30)
#define S3C_NFCONF_ECC_MLC	(1<<24)
#define S3C_NFCONF_ECC_1BIT	(0<<23)
#define S3C_NFCONF_ECC_4BIT	(2<<23)
#define S3C_NFCONF_ECC_8BIT	(1<<23)
#define S3C_NFCONF_TACLS(x)	((x)<<12)
#define S3C_NFCONF_TWRPH0(x)	((x)<<8)
#define S3C_NFCONF_TWRPH1(x)	((x)<<4)
#define S3C_NFCONF_ADVFLASH	(1<<3)
#define S3C_NFCONF_PAGESIZE	(1<<2)
#define S3C_NFCONF_ADDRCYCLE	(1<<1)
#define S3C_NFCONF_BUSWIDTH	(1<<0)

#define S3C_NFCONT_ECC_ENC	(1<<18)
#define S3C_NFCONT_LOCKTGHT	(1<<17)
#define S3C_NFCONT_LOCKSOFT	(1<<16)
#define S3C_NFCONT_MECCLOCK	(1<<7)
#define S3C_NFCONT_SECCLOCK	(1<<6)
#define S3C_NFCONT_INITMECC	(1<<5)
#define S3C_NFCONT_INITSECC	(1<<4)
#define S3C_NFCONT_nFCE1	(1<<2)
#define S3C_NFCONT_nFCE0	(1<<1)
#define S3C_NFCONT_ENABLE	(1<<0)
#define S3C_NFCONT_INITECC	(S3C_NFCONT_INITSECC | S3C_NFCONT_INITMECC)

#define S3C_NFSTAT_ECCENCDONE	(1<<7)
#define S3C_NFSTAT_ECCDECDONE	(1<<6)
#define S3C_NFSTAT_ILEGL_ACC	(1<<5)
#define S3C_NFSTAT_RnB_CHANGE	(1<<4)
#define S3C_NFSTAT_nFCE1	(1<<3)
#define S3C_NFSTAT_nFCE0	(1<<2)
#define S3C_NFSTAT_Res1		(1<<1)
#define S3C_NFSTAT_READY	(1<<0)
#define S3C_NFSTAT_CLEAR	((1<<7) |(1<<6) |(1<<5) |(1<<4))
#define S3C_NFSTAT_BUSY		(1<<0)

#define S3C_NFECCERR0_ECCBUSY	(1<<31)


/* S5PV210 specific */

#define S5P_NFECCCONF		S3C_NFREG(0x20000)
#define S5P_NFECCCONT		S3C_NFREG(0x20020)
#define S5P_NFECCSTAT		S3C_NFREG(0x20030)
#define S5P_NFECCSECSTAT		S3C_NFREG(0x20040)

#define S5P_NFECCPRGECC0		S3C_NFREG(0x20090 + 0 * 4)
#define S5P_NFECCPRGECC1		S3C_NFREG(0x20090 + 1 * 4)
#define S5P_NFECCPRGECC2		S3C_NFREG(0x20090 + 2 * 4)
#define S5P_NFECCPRGECC3		S3C_NFREG(0x20090 + 3 * 4)
#define S5P_NFECCPRGECC4		S3C_NFREG(0x20090 + 4 * 4)
#define S5P_NFECCPRGECC5		S3C_NFREG(0x20090 + 5 * 4)
#define S5P_NFECCPRGECC6		S3C_NFREG(0x20090 + 6 * 4)

#define S5P_NFECCERL0		S3C_NFREG(0x200C0 + 0 * 4)
#define S5P_NFECCERL1		S3C_NFREG(0x200C0 + 1 * 4)
#define S5P_NFECCERL2		S3C_NFREG(0x200C0 + 2 * 4)
#define S5P_NFECCERL3		S3C_NFREG(0x200C0 + 3 * 4)
#define S5P_NFECCERL4		S3C_NFREG(0x200C0 + 4 * 4)
#define S5P_NFECCERL5		S3C_NFREG(0x200C0 + 5 * 4)
#define S5P_NFECCERL6		S3C_NFREG(0x200C0 + 6 * 4)
#define S5P_NFECCERL7		S3C_NFREG(0x200C0 + 7 * 4)

#define S5P_NFECCERP0		S3C_NFREG(0x200F0 + 0 * 4)
#define S5P_NFECCERP1		S3C_NFREG(0x200F0 + 1 * 4)
#define S5P_NFECCERP2		S3C_NFREG(0x200F0 + 2 * 4)
#define S5P_NFECCERP3		S3C_NFREG(0x200F0 + 3 * 4)

#define S5P_NFECCCONF_ECCTYPE	(0xf<<0)
#define S5P_NFECCCONF_ECC_DISABLE	(0<<0)
#define S5P_NFECCCONF_ECC_8BIT	(3<<0)
#define S5P_NFECCCONF_ECC_12BIT	(4<<0)
#define S5P_NFECCCONF_ECC_16BIT	(5<<0)
#define S5P_NFECCCONF_MSGLEN_MASK	(0x3FF<<16)
#define S5P_NFECCCONF_MSGLEN(len)	((len-1)<<16)

#define S5P_NFECCCONT_DIRECTION	(1<<16)
#define S5P_NFECCCONT_READ		(0<<16)
#define S5P_NFECCCONT_WRITE		(1<<16)

#define S5P_NFECCSTAT_FREE_PAGE	(1 << 8)
#define S5P_NFECCSTAT_DECDONE	(1 << 24)
#define S5P_NFECCSTAT_ENCDONE	(1 << 25)
#define S5P_NFECCSTAT_BUSY		(1 << 31)
#define S5P_NFECCCONT_INITMECC	(1<<2)


#define S3C_NAND_WAIT_TIME_MS	(80)

#define S3C_DEFAULT_MESSAGE_SIZE	0

/* new oob placement block for use with hardware ecc generation
 */

/* Nand flash oob definition for MLC 2k page size */
static struct nand_ecclayout s3c_nand_oob_mlc_64 = {
	.eccbytes = 32,
	.eccpos = {
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63
	},
	.oobfree = {
		{
		.offset = 2,
		.length = 28
		}
	}
};

/* Nand flash oob definition for 4Kb page size with 8_bit ECC */
static struct nand_ecclayout s3c_nand_oob_128 = {
	.eccbytes = 104,
	.eccpos = {
		24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127
	},
	.oobfree = {
		{
		.offset = 2,
		.length = 22
		}
	}
};

static struct nand_ecclayout s3c_nand_oob_512 = {
	.eccbytes = 448,
	.eccpos = {
		36, 37, 38, 39, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51,
		52, 53, 54, 55, 56, 57, 58, 59,
		60, 61, 62, 63, 64, 65, 66, 67,
		68, 69, 70, 71, 72, 73, 74, 75,
		76, 77, 78, 79, 80, 81, 82, 83,
		84, 85, 86, 87, 88, 89, 90, 91,
		92, 93, 94, 95, 96, 97, 98, 99,
		100, 101, 102, 103, 104, 105, 106, 107,
		108, 109, 110, 111, 112, 113, 114, 115,
		116, 117, 118, 119, 120, 121, 122, 123,
		124, 125, 126, 127, 128, 129, 130, 131,
		132, 133, 134, 135, 136, 137, 138, 139,
		140, 141, 142, 143, 144, 145, 146, 147,
		148, 149, 150, 151, 152, 153, 154, 155,
		156, 157, 158, 159, 160, 161, 162, 163,
		164, 165, 166, 167, 168, 169, 170, 171,
		172, 173, 174, 175, 176, 177, 178, 179,
		180, 181, 182, 183, 184, 185, 186, 187,
		188, 189, 190, 191, 192, 193, 194, 195,
		196, 197, 198, 199, 200, 201, 202, 203,
		204, 205, 206, 207, 208, 209, 210, 211,
		212, 213, 214, 215, 216, 217, 218, 219,
		220, 221, 222, 223, 224, 225, 226, 227,
		228, 229, 230, 231, 232, 233, 234, 235,
		236, 237, 238, 239, 240, 241, 242, 243,
		244, 245, 246, 247, 248, 249, 250, 251,
		252, 253, 254, 255, 256, 257, 258, 259,
		260, 261, 262, 263, 264, 265, 266, 267,
		268, 269, 270, 271, 272, 273, 274, 275,
		276, 277, 278, 279, 280, 281, 282, 283,
		284, 285, 286, 287, 288, 289, 290, 291,
		292, 293, 294, 295, 296, 297, 298, 299,
		300, 301, 302, 303, 304, 305, 306, 307,
		308, 309, 310, 311, 312, 313, 314, 315,
		316, 317, 318, 319, 320, 321, 322, 323,
		324, 325, 326, 327, 328, 329, 330, 331,
		332, 333, 334, 335, 336, 337, 338, 339,
		340, 341, 342, 343, 344, 345, 346, 347,
		348, 349, 350, 351, 352, 353, 354, 355,
		356, 357, 358, 359, 360, 361, 362, 363,
		364, 365, 366, 367, 368, 369, 370, 371,
		372, 373, 374, 375, 376, 377, 378, 379,
		380, 381, 382, 383, 384, 385, 386, 387,
		388, 389, 390, 391, 392, 393, 394, 395,
		396, 397, 398, 399, 400, 401, 402, 403,
		404, 405, 406, 407, 408, 409, 410, 411,
		412, 413, 414, 415, 416, 417, 418, 419,
		420, 421, 422, 423, 424, 425, 426, 427,
		428, 429, 430, 431, 432, 433, 434, 435,
		436, 437, 438, 439, 440, 441, 442, 443,
		444, 445, 446, 447, 448, 449, 450, 451,
		452, 453, 454, 455, 456, 457, 458, 459,
		460, 461, 462, 463, 464, 465, 466, 467,
		468, 469, 470, 471, 472, 473, 474, 475,
		476, 477, 478, 479, 480, 481, 482, 483,
	},
	.oobfree = {
		{
		.offset = 4,
		.length = 32
		}
	}
};

struct s3c_nand_info {
	struct nand_chip		chip;
	void __iomem			*regs;

	int 				hwecc_mode;
	int				hwecc_message_size;
} chip_info;

/* conversion functions */

static struct s3c_nand_info *mtd_to_nand_info(struct mtd_info *mtd)
{
	struct nand_chip *nchip = mtd->priv;
	return container_of(nchip, struct s3c_nand_info, chip);
}

static int s3c_chip_mlc(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return (chip->cellinfo & NAND_CI_CELLTYPE_MSK);
}

/* timing calculations */
#define NS_IN_KHZ 1000000

/**
 * s3c_nand_calc_rate - calculate timing data.
 * @wanted: The cycle time in nanoseconds.
 * @clk: The clock rate in kHz.
 * @max: The maximum divider value.
 *
 * Calculate the timing value from the given parameters.
 */
static int s3c_nand_calc_rate(int wanted, unsigned long clk, int max)
{
	int result;

	result = DIV_ROUND_UP((wanted * clk), NS_IN_KHZ);

	if (result > max) {
		return -1;
	}

	if (result < 1)
		result = 1;

	return result;
}

/* controller setup */

/**
 * s3c_nand_setrate - setup controller timing information.
 * @info: The controller instance.
 */
static int s3c_nand_setrate(struct s3c_nand_info *info)
{
	int tacls_max = 15;
	int twrph0_max = 16;
	int twrph1_max =  16;
	int tacls, twrph0, twrph1;
	unsigned long clkrate = get_nand_clk();
	unsigned long set, cfg, mask;

	/* calculate the timing information for the controller */

	clkrate /= 1000;	/* turn clock into kHz for ease of use */

	/* FIXME make timing parameter configuable */
	tacls = s3c_nand_calc_rate(25, clkrate, tacls_max);
	twrph0 = s3c_nand_calc_rate(55, clkrate, twrph0_max);
	twrph1 = s3c_nand_calc_rate(40, clkrate, twrph1_max);

	if (tacls < 0 || twrph0 < 0 || twrph1 < 0) {
		return -EINVAL;
	}

	mask = (S3C_NFCONF_TACLS(15) |
		S3C_NFCONF_TWRPH0(15) |
		S3C_NFCONF_TWRPH1(15));

	set = S3C_NFCONF_TACLS(tacls);
	set |= S3C_NFCONF_TWRPH0(twrph0 - 1);
	set |= S3C_NFCONF_TWRPH1(twrph1 - 1);

	cfg = readl(info->regs + S3C_NFCONF);
	cfg &= ~mask;
	cfg |= set;
	writel(cfg, info->regs + S3C_NFCONF);

	return 0;
}

static void s3c_nand_update_configuration(struct mtd_info *mtd)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	unsigned long nfconf;
	nfconf = readl(info->regs + S3C_NFCONF);
	if (s3c_chip_mlc(mtd)) {
		nfconf |= S3C_NFCONF_ADDRCYCLE | S3C_NFCONF_ADVFLASH;
		if (mtd->writesize == 2048)
			nfconf |= S3C_NFCONF_PAGESIZE;
		else
			nfconf &= ~S3C_NFCONF_PAGESIZE;
	} else {
		if (mtd->writesize == 512)
			nfconf |= S3C_NFCONF_PAGESIZE;
		else
			nfconf &= ~S3C_NFCONF_PAGESIZE;
	}
	writel(nfconf, info->regs + S3C_NFCONF);
}

/**
 * s3c_nand_select_chip - select the given nand chip
 * @mtd: The MTD instance for this chip.
 * @chip: The chip number.
 *
 * This is called by the MTD layer to either select a given chip for the
 * @mtd instance, or to indicate that the access has finished and the
 * chip can be de-selected.
 *
 * The routine ensures that the nFCE line is correctly setup, and any
 * platform specific selection code is called to route nFCE to the specific
 * chip.
 */
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	unsigned long cur;

	cur = readl(info->regs + S3C_NFCONT);

	if (chip == -1) {
		cur |= S3C_NFCONT_nFCE0;
	} else {
		cur &= ~S3C_NFCONT_nFCE0;
	}

	writel(cur, info->regs + S3C_NFCONT);
}

/*
 * Hardware specific access to control-lines function
 */
static void s3c_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		writeb(cmd, info->regs + S3C_NFCMMD);
	else
		writeb(cmd, info->regs + S3C_NFADDR);
}

static int s3c_nand_device_ready(struct mtd_info *mtd)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	return readb(info->regs + S3C_NFSTAT) & S3C_NFSTAT_READY;
}

/* ECC handling functions */

#ifdef CONFIG_MTD_NAND_S3C_HWECC
static void s3c_set_message_size(struct s3c_nand_info *info, int size)
{
	info->hwecc_message_size = size;
}

static int s3c_get_message_size(struct s3c_nand_info *info, struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return info->hwecc_message_size != S3C_DEFAULT_MESSAGE_SIZE ?
		info->hwecc_message_size : chip->ecc.size;
}

static void s3c_wait_bits_set(volatile void* reg,
			      unsigned long mask, unsigned long timeout)
{
	while (timeout--) {
		if (readl(reg) & mask)
			break;
		mdelay(1);
	}
}

static void s3c_wait_bits_clear(volatile void* reg,
			      unsigned long mask, unsigned long timeout)
{
	while (timeout--) {
		if (!(readl(reg) & mask))
			break;
		mdelay(1);
	}
}

static void s3c_nand_wait_enc(struct s3c_nand_info *info)
{
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	s3c_wait_bits_set(info->regs + S3C_NFSTAT,
			  S3C_NFSTAT_ECCENCDONE, S3C_NAND_WAIT_TIME_MS);
}

static void s3c_nand_wait_dec(struct s3c_nand_info *info)
{
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	s3c_wait_bits_set(info->regs + S3C_NFSTAT,
			  S3C_NFSTAT_ECCDECDONE, S3C_NAND_WAIT_TIME_MS);
}

static void s5p_nand_wait_dec(struct s3c_nand_info *info)
{
	s3c_wait_bits_set(info->regs + S5P_NFECCSTAT,
			  S5P_NFECCSTAT_DECDONE, S3C_NAND_WAIT_TIME_MS);
}

static void s5p_nand_wait_enc(struct s3c_nand_info *info)
{
	s3c_wait_bits_set(info->regs + S5P_NFECCSTAT,
			  S5P_NFECCSTAT_ENCDONE, S3C_NAND_WAIT_TIME_MS);
}

/*
 * Function for checking ECC Busy
 */
static void s3c_nand_wait_ecc_busy(struct s3c_nand_info *info)
{
	s3c_wait_bits_clear(info->regs + S3C_NFMECCERR0,
			  S3C_NFECCERR0_ECCBUSY, S3C_NAND_WAIT_TIME_MS);
}


static void s3c_nand_wait_ecc_busy_8bit(struct s3c_nand_info *info)
{
	s3c_wait_bits_clear(info->regs + S3C_NF8ECCERR0,
			  S3C_NFECCERR0_ECCBUSY, S3C_NAND_WAIT_TIME_MS);
}

/* for 8 bit and above */
static void s5p_nand_wait_ecc_busy(struct s3c_nand_info *info)
{
	s3c_wait_bits_clear(info->regs + S5P_NFECCSTAT,
			  S5P_NFECCSTAT_BUSY, S3C_NAND_WAIT_TIME_MS);
}

void s3c_lock_main_ecc(struct s3c_nand_info *info)
{
	u_long nfcont;

	/* Lock */
	nfcont = readl(info->regs + S3C_NFCONT);
	nfcont |= S3C_NFCONT_MECCLOCK;
	writel(nfcont, (info->regs + S3C_NFCONT));
}

/*
 * This function determines whether read data is good or not.
 * If SLC, must write ecc codes to controller before reading status bit.
 * If MLC, status bit is already set, so only reading is needed.
 * If status bit is good, return 0.
 * If correctable errors occured, do that.
 * If uncorrectable errors occured, return -1.
 */
static int s3c_nand_correct_data(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int ret = -1;
	u_long nfestat0, nfestat1, nfmeccdata0, nfmeccdata1, nfmlcbitpt;
	u_char err_type;

	if (!s3c_chip_mlc(mtd)) {
		/* SLC: Write ECC data to compare */
		nfmeccdata0 = (read_ecc[1] << 16) | read_ecc[0];
		nfmeccdata1 = (read_ecc[3] << 16) | read_ecc[2];
		writel(nfmeccdata0, info->regs + S3C_NFMECCDATA0);
		writel(nfmeccdata1, info->regs + S3C_NFMECCDATA1);

		/* Read ECC status */
		nfestat0 = readl(info->regs + S3C_NFMECCERR0);
		err_type = nfestat0 & 0x3;

		switch (err_type) {
		case 0: /* No error */
			ret = 0;
			break;

		case 1: /* 1 bit error (Correctable)
			   (nfestat0 >> 7) & 0x7ff	:error byte number
			   (nfestat0 >> 4) & 0x7	:error bit number */
			dat[(nfestat0 >> 7) & 0x7ff] ^= (1 << ((nfestat0 >> 4) & 0x7));
			printk("to 0x%02x\n", dat[(nfestat0 >> 7) & 0x7ff]);
			ret = 1;
			break;

		case 2: /* Multiple error */
		case 3: /* ECC area error */
			ret = -1;
			break;
		}
	} else {
		/* MLC: */
		s3c_nand_wait_ecc_busy(info);

		nfestat0 = readl(info->regs + S3C_NFMECCERR0);
		nfestat1 = readl(info->regs + S3C_NFMECCERR1);
		nfmlcbitpt = readl(info->regs + S3C_NFMLCBITPT);

		err_type = (nfestat0 >> 26) & 0x7;

		/* No error, If free page (all 0xff) */
		if ((nfestat0 >> 29) & 0x1) {
			err_type = 0;
		} else {
			/* No error, If all 0xff from 17th byte in oob (in case of JFFS2 format) */
			if (dat) {
				if (dat[17] == 0xff && dat[26] == 0xff && dat[35] == 0xff && dat[44] == 0xff && dat[54] == 0xff)
					err_type = 0;
			}
		}

		switch (err_type) {
		case 5: /* Uncorrectable */
			ret = -1;
			break;

		case 4: /* 4 bit error (Correctable) */
			dat[(nfestat1 >> 16) & 0x3ff] ^= ((nfmlcbitpt >> 24) & 0xff);

		case 3: /* 3 bit error (Correctable) */
			dat[nfestat1 & 0x3ff] ^= ((nfmlcbitpt >> 16) & 0xff);

		case 2: /* 2 bit error (Correctable) */
			dat[(nfestat0 >> 16) & 0x3ff] ^= ((nfmlcbitpt >> 8) & 0xff);

		case 1: /* 1 bit error (Correctable) */
			dat[nfestat0 & 0x3ff] ^= (nfmlcbitpt & 0xff);
			ret = err_type;
			break;

		case 0: /* No error */
			ret = 0;
			break;
		}
	}
	return ret;
}

int s3c_nand_correct_data_4bit_nosecc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;

	if (info->hwecc_mode == NAND_ECC_READ) {
		chip->write_buf(mtd, read_ecc, chip->ecc.bytes);
		s3c_lock_main_ecc(info);
		s3c_nand_wait_dec(info);
	}
	return s3c_nand_correct_data(mtd, dat, read_ecc, calc_ecc);
}

int s3c_nand_correct_data_8bit_secc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	int ret = -1;
	u_long nf8eccerr0, nf8eccerr1, nf8eccerr2, nfmlc8bitpt0, nfmlc8bitpt1;
	u_char err_type;
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	s3c_nand_wait_ecc_busy_8bit(info);

	nf8eccerr0 = readl(info->regs + S3C_NF8ECCERR0);
	nf8eccerr1 = readl(info->regs + S3C_NF8ECCERR1);
	nf8eccerr2 = readl(info->regs + S3C_NF8ECCERR2);
	nfmlc8bitpt0 = readl(info->regs + S3C_NFMLC8BITPT0);
	nfmlc8bitpt1 = readl(info->regs + S3C_NFMLC8BITPT1);

	err_type = (nf8eccerr0 >> 25) & 0xf;

	/*
	 * No error, If free page (all 0xff)
	 * While testing, it was found that NFECCERR0[29] bit is set even if
	 * the page contents were not zero. So this code is commented
	 */
	switch (err_type) {
	case 9: /* Uncorrectable */
		ret = -1;
		break;

	case 8: /* 8 bit error (Correctable) */
		dat[(nf8eccerr2 >> 22) & 0x3ff] ^= ((nfmlc8bitpt1 >> 24) & 0xff);

	case 7: /* 7 bit error (Correctable) */
		dat[(nf8eccerr2 >> 11) & 0x3ff] ^= ((nfmlc8bitpt1 >> 16) & 0xff);

	case 6: /* 6 bit error (Correctable) */
		dat[nf8eccerr2 & 0x3ff] ^= ((nfmlc8bitpt1 >> 8) & 0xff);

	case 5: /* 5 bit error (Correctable) */
		dat[(nf8eccerr1 >> 22) & 0x3ff] ^= (nfmlc8bitpt1 & 0xff);

	case 4: /* 4 bit error (Correctable) */
		dat[(nf8eccerr1 >> 11) & 0x3ff] ^= ((nfmlc8bitpt0 >> 24) & 0xff);

	case 3: /* 3 bit error (Correctable) */
		dat[nf8eccerr1 & 0x3ff] ^= ((nfmlc8bitpt0 >> 16) & 0xff);

	case 2: /* 2 bit error (Correctable) */
		dat[(nf8eccerr0 >> 15) & 0x3ff] ^= ((nfmlc8bitpt0 >> 8) & 0xff);

	case 1: /* 1 bit error (Correctable) */
		dat[nf8eccerr0 & 0x3ff] ^= (nfmlc8bitpt0 & 0xff);

		ret = err_type;
		break;

	case 0: /* No error */
		ret = 0;
		break;
	}

	return ret;
}

int s3c_nand_correct_data_8bit_nosecc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;

	if (info->hwecc_mode == NAND_ECC_READ) {
		chip->write_buf(mtd, read_ecc, chip->ecc.bytes);
		s3c_lock_main_ecc(info);
		s3c_nand_wait_dec(info);
	}
	return s3c_nand_correct_data_8bit_secc(mtd, dat, read_ecc, calc_ecc);
}

/* with oob ecc (secondary ecc)*/
int s5p_nand_correct_data_8bit_secc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	u_long nfeccstat, nfeccsecstat;
	u_long nfeccerr[4];
	u_long nfeccerp[2];
	u_char err_type;
	uint16_t *loc = (uint16_t *) nfeccerr;
	uint8_t *pat = (uint8_t *) nfeccerp;
	int i;
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int eccsize = s3c_get_message_size(info, mtd);

	s5p_nand_wait_ecc_busy(info);

	nfeccstat = readl(info->regs + S5P_NFECCSTAT);
	nfeccsecstat = readl(info->regs + S5P_NFECCSECSTAT);

	/* need more test to see if it works or not */
	/*
	if (nfeccstat & S5P_NFECCSTAT_FREE_PAGE)
		return 0;
	*/

	err_type = nfeccsecstat & 0x1f;

	if (likely(!err_type)) {
		return 0;
	} else if (err_type > 8) {
		return -1;
	}

	nfeccerr[0] = readl(info->regs + S5P_NFECCERL0);
	nfeccerr[1] = readl(info->regs + S5P_NFECCERL1);
	nfeccerr[2] = readl(info->regs + S5P_NFECCERL2);
	nfeccerr[3] = readl(info->regs + S5P_NFECCERL3);

	nfeccerp[0] = readl(info->regs + S5P_NFECCERP0);
	nfeccerp[1] = readl(info->regs + S5P_NFECCERP1);

	for (i = 0; i < err_type; i++) {

		int pos = loc[i] & 0x3ff;

		/* it is possible to cause buffer overrun if not checked */
		if (pos < eccsize)
			dat[pos] ^= pat[i];
		else
			;/* an ecc code itself bit flips, the hardware
			  * can handle this case, but seems there is no way to
			  * report back to the caller, so simply ignore it */
	}

	return err_type;
}

/* without oob ecc (secondary ecc)*/
int s5p_nand_correct_data_8bit_nosecc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;

	if (info->hwecc_mode == NAND_ECC_READ) {
		chip->write_buf(mtd, read_ecc, chip->ecc.bytes);
		s3c_lock_main_ecc(info);
		s5p_nand_wait_dec(info);
	}
	return s5p_nand_correct_data_8bit_secc(mtd, dat, read_ecc, calc_ecc);
}

/* with oob ecc (secondary ecc)*/
int s5p_nand_correct_data_16bit_secc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	u_long nfeccstat, nfeccsecstat;
	u_long nfeccerr[8];
	u_long nfeccerp[4];
	u_char err_type;
	uint16_t *loc = (uint16_t *) nfeccerr;
	uint8_t *pat = (uint8_t *) nfeccerp;
	int i;
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int eccsize = s3c_get_message_size(info, mtd);


	s5p_nand_wait_ecc_busy(info);

	nfeccstat = readl(info->regs + S5P_NFECCSTAT);
	nfeccsecstat = readl(info->regs + S5P_NFECCSECSTAT);

	/* need more test to see if it works or not */
	/*
	if (nfeccstat & S5P_NFECCSTAT_FREE_PAGE)
		return 0;
	*/

	err_type = nfeccsecstat & 0x1f;

	if (likely(!err_type)) {
		return 0;
	} else if (err_type > 16) {
		return -1;
	}

	nfeccerr[0] = readl(info->regs + S5P_NFECCERL0);
	nfeccerr[1] = readl(info->regs + S5P_NFECCERL1);
	nfeccerr[2] = readl(info->regs + S5P_NFECCERL2);
	nfeccerr[3] = readl(info->regs + S5P_NFECCERL3);
	nfeccerr[4] = readl(info->regs + S5P_NFECCERL4);
	nfeccerr[5] = readl(info->regs + S5P_NFECCERL5);
	nfeccerr[6] = readl(info->regs + S5P_NFECCERL6);
	nfeccerr[7] = readl(info->regs + S5P_NFECCERL7);

	nfeccerp[0] = readl(info->regs + S5P_NFECCERP0);
	nfeccerp[1] = readl(info->regs + S5P_NFECCERP1);
	nfeccerp[2] = readl(info->regs + S5P_NFECCERP2);
	nfeccerp[3] = readl(info->regs + S5P_NFECCERP3);

	for (i = 0; i < err_type; i++) {

		int pos = loc[i] & 0x3ff;

		/* it is possible to cause buffer overrun if not checked */
		if (pos < eccsize)
			dat[pos] ^= pat[i];
		else
			;/* an ecc code itself bit flips, the hardware
			  * can handle this case, but seems there is no way to
			  * report back to the caller, so simply ignore it */
	}

	return err_type;
}

/* without oob ecc (secondary ecc)*/
int s5p_nand_correct_data_16bit_nosecc(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;

	if (info->hwecc_mode == NAND_ECC_READ) {
		chip->write_buf(mtd, read_ecc, chip->ecc.bytes);
		s3c_lock_main_ecc(info);
		s5p_nand_wait_dec(info);
	}
	return s5p_nand_correct_data_16bit_secc(mtd, dat, read_ecc, calc_ecc);
}

/* ECC functions
 *
 * These allow the s3c2410 and s3c2440 to use the controller's ECC
 * generator block to ECC the data as it passes through]
*/

static void s3c_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	unsigned long nfcont;
	unsigned long nfconf;

	info->hwecc_mode = mode;

	nfconf = readl(info->regs + S3C_NFCONF);

	nfconf &= ~(0x3 << 23);
	if (s3c_chip_mlc(mtd))
		nfconf |= S3C_NFCONF_ECC_4BIT;
	else
		nfconf |= S3C_NFCONF_ECC_1BIT;

	writel(nfconf, info->regs + S3C_NFCONF);

	/* Init main ECC & unlock */

	nfcont = readl(info->regs + S3C_NFCONT);
	nfcont |= S3C_NFCONT_INITMECC;
	nfcont &= ~S3C_NFCONT_MECCLOCK;

	if (s3c_chip_mlc(mtd)) {
		if (info->hwecc_mode == NAND_ECC_WRITE)
			nfcont |= S3C_NFCONT_ECC_ENC;
		else if (mode == NAND_ECC_READ)
			nfcont &= ~S3C_NFCONT_ECC_ENC;
	}

	writel(nfcont, info->regs + S3C_NFCONT);

}

void s3c_nand_enable_hwecc_8bit(struct mtd_info *mtd, int mode)
{
	u_long nfcont, nfconf;
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	info->hwecc_mode = mode;

	/* 8 bit selection */
	nfconf = readl(info->regs + S3C_NFCONF);

	nfconf &= ~(0x3 << 23);
	nfconf |= (0x1 << 23);

	writel(nfconf, (info->regs + S3C_NFCONF));

	/* Initialize & unlock */
	nfcont = readl(info->regs + S3C_NFCONT);
	nfcont |= S3C_NFCONT_INITECC;
	nfcont &= ~S3C_NFCONT_MECCLOCK;

	if (mode == NAND_ECC_WRITE)
		nfcont |= S3C_NFCONT_ECC_ENC;
	else if (mode == NAND_ECC_READ)
		nfcont &= ~S3C_NFCONT_ECC_ENC;

	writel(nfcont, (info->regs + S3C_NFCONT));
}

/* enable s5p 8 12 16 bit ecc */
void s5p_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;
	int eccsize = s3c_get_message_size(info, mtd);
	u_long nfcont, nfeccconf, nfecccont, nfeccstat;

	info->hwecc_mode = mode;

	/* Set ECC type 16-bit ECC */
	nfeccconf = readl(info->regs + S5P_NFECCCONF);
	nfeccconf &= ~ S5P_NFECCCONF_ECCTYPE;
	switch (chip->ecc.strength) {
	case 16:
		nfeccconf |= S5P_NFECCCONF_ECC_16BIT;
		break;
	case 12:
		nfeccconf |= S5P_NFECCCONF_ECC_12BIT;
		break;
	case 8:
		nfeccconf |= S5P_NFECCCONF_ECC_8BIT;
		break;
	default:
		return;
	}

	/* set 8/12/16bit ECC message length to msg */
	nfeccconf &= ~ S5P_NFECCCONF_MSGLEN_MASK;
	nfeccconf |= S5P_NFECCCONF_MSGLEN(eccsize);
	writel(nfeccconf, info->regs + S5P_NFECCCONF);

	/* clear 8/12/16bit ecc encode/decode done */
	nfeccstat = readl(info->regs + S5P_NFECCSTAT);
	nfeccstat |= S5P_NFECCSTAT_DECDONE | S5P_NFECCSTAT_ENCDONE;
	writel(nfeccstat, info->regs + S5P_NFECCSTAT);

	nfecccont = readl(info->regs + S5P_NFECCCONT);
	nfecccont &= ~ S5P_NFECCCONT_DIRECTION;

	/* set Ecc direction */
	if (info->hwecc_mode == NAND_ECC_WRITE)
		nfecccont |= S5P_NFECCCONT_WRITE;
	else if (info->hwecc_mode == NAND_ECC_READ)
		nfecccont |= S5P_NFECCCONT_READ;
	/* Reset ECC value. */
	nfecccont |= S5P_NFECCCONT_INITMECC;
	writel(nfecccont, info->regs + S5P_NFECCCONT);

	/* Initialize & unlock */
	nfcont = readl(info->regs + S3C_NFCONT);
	nfcont &= ~S3C_NFCONT_MECCLOCK;
	writel(nfcont, info->regs + S3C_NFCONT);
}

void s3c_nand_read_ecc_4bit(struct s3c_nand_info *info, u_char *ecc_code)
{
	u_long nfmecc0, nfmecc1;

	nfmecc0 = readl(info->regs + S3C_NFMECC0);
	nfmecc1 = readl(info->regs + S3C_NFMECC1);

	ecc_code[0] = nfmecc0 & 0xff;
	ecc_code[1] = (nfmecc0 >> 8) & 0xff;
	ecc_code[2] = (nfmecc0 >> 16) & 0xff;
	ecc_code[3] = (nfmecc0 >> 24) & 0xff;
	ecc_code[4] = nfmecc1 & 0xff;
	ecc_code[5] = (nfmecc1 >> 8) & 0xff;
	ecc_code[6] = (nfmecc1 >> 16) & 0xff;
	ecc_code[7] = (nfmecc1 >> 24) & 0xff;
}

static int s3c_nand_calculate_ecc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	u_long nfmecc0;

	s3c_lock_main_ecc(info);

	if (!s3c_chip_mlc(mtd)) {
		nfmecc0 = readl(info->regs + S3C_NFMECC0);

		ecc_code[0] = nfmecc0 & 0xff;
		ecc_code[1] = (nfmecc0 >> 8) & 0xff;
		ecc_code[2] = (nfmecc0 >> 16) & 0xff;
		ecc_code[3] = (nfmecc0 >> 24) & 0xff;
	} else {
		if (info->hwecc_mode == NAND_ECC_READ)
			s3c_nand_wait_dec(info);
		else {
			s3c_nand_wait_enc(info);
			s3c_nand_read_ecc_4bit(info, ecc_code);
		}
	}
	return 0;
}

int s3c_nand_calculate_ecc_4bit_nosecc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	if (info->hwecc_mode == NAND_ECC_WRITE) {

		s3c_lock_main_ecc(info);
		s5p_nand_wait_enc(info);
		s3c_nand_read_ecc_4bit(info, ecc_code);
	}

	return 0;
}

void s3c_nand_read_ecc_8bit(struct s3c_nand_info *info, u_char *ecc_code)
{
	u_long nfm8ecc0, nfm8ecc1, nfm8ecc2, nfm8ecc3;

	nfm8ecc0 = readl(info->regs + S3C_NFM8ECC0);
	nfm8ecc1 = readl(info->regs + S3C_NFM8ECC1);
	nfm8ecc2 = readl(info->regs + S3C_NFM8ECC2);
	nfm8ecc3 = readl(info->regs + S3C_NFM8ECC3);

	ecc_code[0] = nfm8ecc0 & 0xff;
	ecc_code[1] = (nfm8ecc0 >> 8) & 0xff;
	ecc_code[2] = (nfm8ecc0 >> 16) & 0xff;
	ecc_code[3] = (nfm8ecc0 >> 24) & 0xff;
	ecc_code[4] = nfm8ecc1 & 0xff;
	ecc_code[5] = (nfm8ecc1 >> 8) & 0xff;
	ecc_code[6] = (nfm8ecc1 >> 16) & 0xff;
	ecc_code[7] = (nfm8ecc1 >> 24) & 0xff;
	ecc_code[8] = nfm8ecc2 & 0xff;
	ecc_code[9] = (nfm8ecc2 >> 8) & 0xff;
	ecc_code[10] = (nfm8ecc2 >> 16) & 0xff;
	ecc_code[11] = (nfm8ecc2 >> 24) & 0xff;
	ecc_code[12] = nfm8ecc3 & 0xff;
}

int s3c_nand_calculate_ecc_8bit_secc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	s3c_lock_main_ecc(info);

	if (info->hwecc_mode == NAND_ECC_READ)
		s3c_nand_wait_dec(info);
	else {
		s3c_nand_wait_enc(info);
		s3c_nand_read_ecc_8bit(info, ecc_code);
	}

	return 0;
}

int s3c_nand_calculate_ecc_8bit_nosecc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	if (info->hwecc_mode == NAND_ECC_WRITE) {

		s3c_lock_main_ecc(info);
		s5p_nand_wait_enc(info);
		s3c_nand_read_ecc_8bit(info, ecc_code);
	}

	return 0;
}

void s5p_nand_read_ecc_8bit(struct s3c_nand_info *info, u_char *ecc_code)
{
	u_long nfecc0, nfecc1, nfecc2, nfecc3;

	nfecc0 = readl(info->regs + S5P_NFECCPRGECC0);
	nfecc1 = readl(info->regs + S5P_NFECCPRGECC1);
	nfecc2 = readl(info->regs + S5P_NFECCPRGECC2);
	nfecc3 = readl(info->regs + S5P_NFECCPRGECC3);

	ecc_code[0] = nfecc0 & 0xff;
	ecc_code[1] = (nfecc0 >> 8) & 0xff;
	ecc_code[2] = (nfecc0 >> 16) & 0xff;
	ecc_code[3] = (nfecc0 >> 24) & 0xff;
	ecc_code[4] = nfecc1 & 0xff;
	ecc_code[5] = (nfecc1 >> 8) & 0xff;
	ecc_code[6] = (nfecc1 >> 16) & 0xff;
	ecc_code[7] = (nfecc1 >> 24) & 0xff;
	ecc_code[8] = nfecc2 & 0xff;
	ecc_code[9] = (nfecc2 >> 8) & 0xff;
	ecc_code[10] = (nfecc2 >> 16) & 0xff;
	ecc_code[11] = (nfecc2 >> 24) & 0xff;
	ecc_code[12] = nfecc3 & 0xff;
}

int s5p_nand_calculate_ecc_8bit_secc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	s3c_lock_main_ecc(info);

	if (info->hwecc_mode == NAND_ECC_READ)
		s5p_nand_wait_dec(info);
	else {
		s5p_nand_wait_enc(info);
		s5p_nand_read_ecc_8bit(info, ecc_code);
	}

	return 0;
}

int s5p_nand_calculate_ecc_8bit_nosecc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	if (info->hwecc_mode == NAND_ECC_WRITE) {

		s3c_lock_main_ecc(info);
		s5p_nand_wait_enc(info);
		s5p_nand_read_ecc_8bit(info, ecc_code);
	}

	return 0;
}

void s5p_nand_read_ecc_16bit(struct s3c_nand_info *info, u_char *ecc_code)
{
	u_long nfecc0, nfecc1, nfecc2, nfecc3;
	u_long nfecc4, nfecc5, nfecc6;

	nfecc0 = readl(info->regs + S5P_NFECCPRGECC0);
	nfecc1 = readl(info->regs + S5P_NFECCPRGECC1);
	nfecc2 = readl(info->regs + S5P_NFECCPRGECC2);
	nfecc3 = readl(info->regs + S5P_NFECCPRGECC3);
	nfecc4 = readl(info->regs + S5P_NFECCPRGECC4);
	nfecc5 = readl(info->regs + S5P_NFECCPRGECC5);
	nfecc6 = readl(info->regs + S5P_NFECCPRGECC6);

	ecc_code[0] = nfecc0 & 0xff;
	ecc_code[1] = (nfecc0 >> 8) & 0xff;
	ecc_code[2] = (nfecc0 >> 16) & 0xff;
	ecc_code[3] = (nfecc0 >> 24) & 0xff;
	ecc_code[4] = nfecc1 & 0xff;
	ecc_code[5] = (nfecc1 >> 8) & 0xff;
	ecc_code[6] = (nfecc1 >> 16) & 0xff;
	ecc_code[7] = (nfecc1 >> 24) & 0xff;
	ecc_code[8] = nfecc2 & 0xff;
	ecc_code[9] = (nfecc2 >> 8) & 0xff;
	ecc_code[10] = (nfecc2 >> 16) & 0xff;
	ecc_code[11] = (nfecc2 >> 24) & 0xff;
	ecc_code[12] = nfecc3 & 0xff;
	ecc_code[13] = (nfecc3 >> 8) & 0xff;
	ecc_code[14] = (nfecc3 >> 16) & 0xff;
	ecc_code[15] = (nfecc3 >> 24) & 0xff;
	ecc_code[16] = nfecc4 & 0xff;
	ecc_code[17] = (nfecc4 >> 8) & 0xff;
	ecc_code[18] = (nfecc4 >> 16) & 0xff;
	ecc_code[19] = (nfecc4 >> 24) & 0xff;
	ecc_code[20] = nfecc5 & 0xff;
	ecc_code[21] = (nfecc5 >> 8) & 0xff;
	ecc_code[22] = (nfecc5 >> 16) & 0xff;
	ecc_code[23] = (nfecc5 >> 24) & 0xff;
	ecc_code[24] = nfecc6 & 0xff;
	ecc_code[25] = (nfecc6 >> 8) & 0xff;
	ecc_code[26] = 0xff;
	ecc_code[27] = 0xff;
}

int s5p_nand_calculate_ecc_16bit_secc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	s3c_lock_main_ecc(info);

	if (info->hwecc_mode == NAND_ECC_READ)
		s5p_nand_wait_dec(info);
	else {
		s5p_nand_wait_enc(info);
		s5p_nand_read_ecc_16bit(info, ecc_code);
	}

	return 0;
}

int s5p_nand_calculate_ecc_16bit_nosecc(struct mtd_info *mtd,
		const u_char *dat, u_char *ecc_code)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);

	if (info->hwecc_mode == NAND_ECC_WRITE) {

		s3c_lock_main_ecc(info);
		s5p_nand_wait_enc(info);
		s5p_nand_read_ecc_16bit(info, ecc_code);
	}

	return 0;
}

static int s5p_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
		int page)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int status = 0;
	int eccbytes = chip->ecc.bytes;
	int secc_start = mtd->oobsize - eccbytes;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);

	/* spare area */
	s3c_set_message_size(info, secc_start);
	chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
	chip->write_buf(mtd, chip->oob_poi, secc_start);
	chip->ecc.calculate(mtd, 0, chip->oob_poi + secc_start);
	chip->write_buf(mtd, chip->oob_poi + secc_start, eccbytes);
	s3c_set_message_size(info, S3C_DEFAULT_MESSAGE_SIZE);
	/* Send command to program the OOB data */

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

static int s5p_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
		int page)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int stat;
	int eccbytes = chip->ecc.bytes;
	int secc_start = mtd->oobsize - eccbytes;

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);

	s3c_set_message_size(info, secc_start);
	chip->ecc.hwctl(mtd, NAND_ECC_READ);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	chip->ecc.calculate(mtd, NULL, NULL);
	stat = chip->ecc.correct(mtd, chip->oob_poi, NULL, NULL);
	if (stat < 0)
		mtd->ecc_stats.failed++;
	else
		mtd->ecc_stats.corrected += stat;
	s3c_set_message_size(info, S3C_DEFAULT_MESSAGE_SIZE);

	return 0;
}

/* with secondary ecc */
static int s5p_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf, int oob_required)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	int secc_start = mtd->oobsize - eccbytes;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;

	uint32_t *eccpos = chip->ecc.layout->eccpos;

	/* main area */
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	/* spare area */
	s3c_set_message_size(info, secc_start);
	chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
	chip->write_buf(mtd, chip->oob_poi, secc_start);
	chip->ecc.calculate(mtd, NULL, &ecc_calc[chip->ecc.total]);
	s3c_set_message_size(info, S3C_DEFAULT_MESSAGE_SIZE);

	if (oob_required)
		memcpy(chip->oob_poi + secc_start,
			ecc_calc + chip->ecc.total, eccbytes);
	chip->write_buf(mtd, ecc_calc + chip->ecc.total, eccbytes);

	return 0;
}

/* with secondary ecc */
static int s5p_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
		uint8_t *buf, int oob_required, int page)
{
	struct s3c_nand_info *info = mtd_to_nand_info(mtd);
	int i, stat, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	int secc_start = mtd->oobsize - eccbytes;
	uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);

	/* spare area */
	s3c_set_message_size(info, secc_start);
	chip->ecc.hwctl(mtd, NAND_ECC_READ);
	/* oob data and oob ecc go through hwecc engine*/
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	chip->ecc.calculate(mtd, chip->oob_poi, &ecc_calc[chip->ecc.total]);
	stat = chip->ecc.correct(mtd, chip->oob_poi,
		chip->oob_poi + secc_start, &ecc_calc[chip->ecc.total]);
	if (stat < 0)
		mtd->ecc_stats.failed++;
	else
		mtd->ecc_stats.corrected += stat;
	s3c_set_message_size(info, S3C_DEFAULT_MESSAGE_SIZE);

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0, -1);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	/* main area */
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->write_buf(mtd, ecc_code + i, eccbytes);
		chip->ecc.calculate(mtd, p,  &ecc_calc[i]);
		stat = chip->ecc.correct(mtd, p, &ecc_code[i], NULL);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}

	return 0;
}
#endif

void s5p_init_slc_hwecc(struct mtd_info* mtd, struct nand_chip *chip)
{
	/* FIXME: these are just copied from random locations, will not work on s5p */
	/* slc defaults */
	chip->ecc.size = 512;

	if (chip->page_shift == 12) { /* 4KiB */

		chip->options |= NAND_NO_SUBPAGE_WRITE;
		chip->ecc.bytes = 13;
		chip->ecc.strength = 8;
		chip->ecc.mode = NAND_ECC_HW_OOB_FIRST;

		/* use default read/write page/oob */

		chip->ecc.hwctl = s3c_nand_enable_hwecc_8bit;
		chip->ecc.calculate = s3c_nand_calculate_ecc_8bit_nosecc;
		chip->ecc.correct = s3c_nand_correct_data_8bit_nosecc;

	} else if ( chip->page_shift >= 10) { /* 1KiB and above */

		chip->ecc.bytes	= 4;
		chip->ecc.strength = 1;

		/* use default read/write page/oob */
	}
}

int compatible_with_secondary_ecc(struct mtd_info* mtd, struct nand_chip *chip)
{
	uint32_t *mecc_pos = chip->ecc.layout->eccpos;
	int total_eccbytes = chip->ecc.layout->eccbytes;
	int eccbytes = chip->ecc.bytes;
	int oobsize = mtd->oobsize;

	/* assume ecc layout leave exact eccbytes for secondary ecc */
	return mecc_pos[total_eccbytes - 1] + 1 + eccbytes == oobsize;
}

int s5p_nand_block_bad_harder(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	struct nand_chip *chip = mtd->priv;
	int (*nand_block_bad)(struct mtd_info *, loff_t, int) = chip->priv;
	int bad;

	/* check the other page */
	chip->bbt_options ^= NAND_BBT_SCANLASTPAGE;
	bad = nand_block_bad(mtd, ofs, getchip);
	chip->bbt_options ^= NAND_BBT_SCANLASTPAGE;
	if (bad)
		return bad;
	/* check the designated page */
	return nand_block_bad(mtd, ofs, getchip);
}

void s5p_init_mlc_hwecc(struct mtd_info* mtd, struct nand_chip *chip)
{
	/* mlc defaults */
	chip->options |= NAND_NO_SUBPAGE_WRITE;
	chip->ecc.size = 512;
	chip->ecc.mode = NAND_ECC_HW_OOB_FIRST;

	if (mtd->writesize == 8192 && mtd->oobsize == 512) {

		if (!chip->ecc.layout)
			chip->ecc.layout = &s3c_nand_oob_512;

		chip->ecc.bytes = 28;
		chip->ecc.strength = 16;
		chip->badblockbits = 8;

		/* this chip need check first and last page for bad, but the
		 * default nand_block_bad is only willing to check one of the
		 * pages, so neeed write a custom one to force it work harder
		 */
		chip->priv = chip->block_bad; /* saved for later use */
		chip->block_bad = s5p_nand_block_bad_harder;

		if (compatible_with_secondary_ecc(mtd, chip)) {

			chip->ecc.read_page = s5p_nand_read_page;
			chip->ecc.write_page = s5p_nand_write_page;
			chip->ecc.read_oob = s5p_nand_read_oob;
			chip->ecc.write_oob = s5p_nand_write_oob;

			chip->ecc.hwctl = s5p_nand_enable_hwecc;
			chip->ecc.calculate = s5p_nand_calculate_ecc_16bit_secc;
			chip->ecc.correct = s5p_nand_correct_data_16bit_secc;
		} else {
			/* use default read/write page/oob */

			chip->ecc.hwctl = s5p_nand_enable_hwecc;
			chip->ecc.calculate = s5p_nand_calculate_ecc_16bit_nosecc;
			chip->ecc.correct = s5p_nand_correct_data_16bit_nosecc;
		}
	} else if (mtd->writesize == 4096 && mtd->oobsize == 128) {

		if (!chip->ecc.layout)
			chip->ecc.layout = &s3c_nand_oob_128;

		chip->ecc.bytes = 13;
		chip->ecc.strength = 8;

		if (compatible_with_secondary_ecc(mtd, chip)) {

			chip->ecc.read_page = s5p_nand_read_page;
			chip->ecc.write_page = s5p_nand_write_page;
			chip->ecc.read_oob = s5p_nand_read_oob;
			chip->ecc.write_oob = s5p_nand_write_oob;

			chip->ecc.hwctl = s5p_nand_enable_hwecc;
			chip->ecc.calculate = s5p_nand_calculate_ecc_8bit_secc;
			chip->ecc.correct = s5p_nand_correct_data_8bit_secc;
		} else {
			/* use default read/write page/oob */

			chip->ecc.hwctl = s5p_nand_enable_hwecc;
			chip->ecc.calculate = s5p_nand_calculate_ecc_8bit_nosecc;
			chip->ecc.correct = s5p_nand_correct_data_8bit_nosecc;
		}
	}
	else if (mtd->writesize == 2048 && mtd->oobsize == 64){

		if (!chip->ecc.layout)
			chip->ecc.layout = &s3c_nand_oob_mlc_64;

		chip->ecc.bytes = 8;    /* really 7 bytes */
		chip->ecc.strength = 4;

		/* use default read/write page/oob */

		chip->ecc.calculate = s3c_nand_calculate_ecc_4bit_nosecc;
		chip->ecc.correct = s3c_nand_correct_data_4bit_nosecc;
	}
}

void board_nand_init(void)
{
	int err;
	struct s3c_nand_info *info = &chip_info;
	struct nand_chip *chip = &info->chip;
	void __iomem *regs = (void __iomem *) ELFIN_NAND_BASE;
	struct mtd_info *mtd = &nand_info[0];

	info->regs = regs;

	err = s3c_nand_setrate(info);
	if (err != 0) {
		printk("s3c_nand_setrate error = %d\n", err);
		return;
	}

	writel(S3C_NFCONT_ENABLE, info->regs + S3C_NFCONT);

	chip->cmd_ctrl		= s3c_nand_hwcontrol;
	chip->dev_ready		= s3c_nand_device_ready;
	chip->select_chip	= s3c_nand_select_chip;
	chip->chip_delay	= 50;
	chip->options		= NAND_SKIP_BBTSCAN;

	chip->IO_ADDR_W = regs + S3C_NFDATA;
	chip->IO_ADDR_R = chip->IO_ADDR_W;

	mtd->priv	   = chip;

#ifdef CONFIG_MTD_NAND_S3C_HWECC
	/* temporary */
	chip->ecc.calculate	= s3c_nand_calculate_ecc;
	chip->ecc.hwctl		= s3c_nand_enable_hwecc;
	chip->ecc.correct	= s3c_nand_correct_data;
	chip->ecc.mode		= NAND_ECC_HW;
	chip->ecc.strength	= 1;

#else
	chip->ecc.mode	    = NAND_ECC_SOFT;
#endif
	err = nand_scan_ident(mtd, CONFIG_SYS_MAX_NAND_DEVICE, NULL);
	if (err != 0) {
		printk("nand_scan_ident error = %d\n", err);
		return;
	}

#ifdef CONFIG_MTD_NAND_S3C_HWECC
	if (s3c_chip_mlc(mtd))
		s5p_init_mlc_hwecc(mtd, chip);
	else
		s5p_init_mlc_hwecc(mtd, chip);
#endif
	s3c_nand_update_configuration(mtd);

	err = nand_scan_tail(mtd);
	if (err != 0) {
		printk("nand_scan_tail error = %d\n", err);
		return;
	}

	nand_register(0);
}

#endif /* (CONFIG_CMD_NAND) */
