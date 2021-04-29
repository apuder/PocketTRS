/*
 * Copyright (C) 1992 Clarendon Hill Software.
 *
 * Permission is granted to any individual or institution to use, copy,
 * or redistribute this software, provided this copyright notice is retained.
 *
 * This software is provided "as is" without any expressed or implied
 * warranty.  If this software brings on any sort of damage -- physical,
 * monetary, emotional, or brain -- too bad.  You've got no one to blame
 * but yourself.
 *
 * The software may be modified for your own purposes, but modified versions
 * must retain this notice.
 */

/*
 * Copyright (c) 1996-2020, Timothy P. Mann
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * trs_memory.c -- memory emulation functions for the TRS-80 emulator
 *
 * Routines in this file perform operations such as mem_read and mem_write,
 * and are the top level emulation points for memory-mapped devices such
 * as the screen and keyboard.
 */

#include "trs.h"
#include "trs-keyboard.h"
#include "trs_screen.h"
#include "esp_attr.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rom/model3-frehd.cpp-inc"

//#define SUPERMEM 1

typedef unsigned char Uchar;

#define MAX_ROM_SIZE	(0x3800)
#define MAX_VIDEO_SIZE	(0x0800)

/* 512K is the largest we support. There were it seems 1MByte
   options at some point which is the full range of the mapping.
   How the mapping register worked for > 1MB is not known */
#define MAX_SUPERMEM_SIZE 	(0x80000)

/* Locations for Model I, Model III, and Model 4 map 0 */
#define ROM_START	(0x0000)
#define IO_START	(0x3000)
#define KEYBOARD_START	(0x3800)
#define MORE_IO_START	(0x3c00)
#define VIDEO_START	(0x3c00)
#define RAM_START	(0x4000)
#define PRINTER_ADDRESS	(0x37E8)

/* Interrupt latch register in EI (Model 1) */
#define TRS_INTLATCH(addr) (((addr)&~3) == 0x37e0)

/* We allow for 2MB of banked memory via port 0x94. That is the extreme limit
   of the port mods rather than anything normal (512K might be more 'normal' */
Uchar memory[2 * 64 * 1024] EXT_RAM_ATTR;
Uchar video[MAX_VIDEO_SIZE + 1] EXT_RAM_ATTR;
const Uchar* rom = model3_frehd_rom;
int trs_rom_size = model3_frehd_rom_len;
int lowercase = 1;
int romin = 0; /* Model 4p */
int huffman_ram = 0;
int hypermem = 0;
#ifdef SUPERMEM
int supermem = 0;
#endif
int selector = 0;

/* private data */
//static Uchar video[MAX_VIDEO_SIZE + 1];
static int trs_video_size;
static int memory_map = 0;
static int bank_offset[2];
#define VIDEO_PAGE_0 0
#define VIDEO_PAGE_1 1024
static int video_offset = (-VIDEO_START + VIDEO_PAGE_0);
static unsigned int bank_base = 0x10000;
static unsigned char mem_command = 0;
#ifdef SUPERMEM
static Uchar *supermem_ram = NULL;
static int supermem_base;
static unsigned int supermem_hi;
#endif
static int selector_reg = 0;

void mem_video_page(int which)
{
    video_offset = -VIDEO_START + (which ? VIDEO_PAGE_1 : VIDEO_PAGE_0);
}

void mem_bank(int command)
{
    switch (command) {
      case 0:
        /* L64 Lower / Upper */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = 0 << 15;
	break;
      case 2:
        /* L64 Lower / H64 Lower */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = bank_base - (1 << 15);
	break;
      case 3:
        /* L64 Lower / H64 upper */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = (0 << 15) + bank_base;
	break;
      case 6:
        /* H64 Lower / L64 upper */
	bank_offset[0] = (0 << 15) + bank_base;
	bank_offset[1] = 0 << 15;
	break;
      case 7:
        /* H64 Upper / L64 Upper */
	bank_offset[0] = (1 << 15) + bank_base;
	bank_offset[1] = 0 << 15;
	break;
      default:
	assert(0);//error("unknown mem_bank command %d", command);
	break;
    }
    mem_command = command;
}

/*
 *	Dave Huffman (and some other) memory expansions. These decode
 *	port 0x94 off U50 as follows
 *
 *	7: only used with Z180 board (not emulated - would need Z180 emulation!)
 *	6: write protect - not emulated
 *	5: sometimes used for > 4MHz turbo mod
 *	4-0: Bits A20-A16 of the alt bank
 *
 *	Set to 1 on a reset so that you get the 'classic' memory map
 *	This port is read-write and the drivers depend upon it
 *	(See RAMDV364.ASM)
 *
 *	The Hypermem is very similar and also changes the upper
 *	64K bank between multiple banks. However the values
 *	are on port 0x90 (sound) bits 4-1, which is a much poorer
 *	design IMHO as sound using apps can randomly change the
 *	upper bank. Fine for a ramdisc but means other software
 *	must take great care.
 *
 *	The MegaMem mappings are not known and not emulated.
 */

void mem_bank_base(int bits)
{
	if (huffman_ram) {
		bits &= 0x1F;
		bank_base = bits << 16;
		mem_bank(mem_command);
	}
	if (hypermem) {
	        /* HyperMem replaces the upper 64K bank with multiple
	           banks according to port 0x90 bits 4-1 */
		bits &= 0x1E;
		/* 0 base is upper bank of 64K */
		bits += 2;
		bank_base = bits << 15;
		mem_bank(mem_command);
	}
#ifdef SUPERMEM
	if (supermem) {
		/* Emulate a 512Kb system. A standard model 1 SuperMEM is 256K
		   or 512K with double stacked chips */
		bits &= 0x0F; /* 15 bits of address + 4bits logical */
		supermem_base = bits << 15;
		/* The supermem can flip the low or high 32K. Set
		   bit 5 to map low */
		if (bits & 0x20)
		    supermem_hi = 0x0000;
		else
		    supermem_hi = 0x8000;
	}
#endif
}

int mem_read_bank_base(void)
{
	if (huffman_ram)
		return (bank_base >> 16) & 0x1F;
#ifdef SUPERMEM
	if (supermem)
		return (supermem_base >> 15) |
			((supermem_hi == 0) ? 0x20 : 0);
#endif
	/* And the HyperMem appears to be write-only */
	return 0xFF;
}

void selector_out(unsigned char value)
{
	/* Not all bits are necessarily really present but hey what
	   you can't read back you can't tell */
	selector_reg = value;
	/* Always Model 1 */
	memory_map = (trs_model << 4) + (selector_reg & 7);
	/* 0x10 is already the default tandy map we add 11-17 in the style
	   of the model 4 approach */
	if (selector_reg & 0x8) {
		/* External RAM enabled */
		/* Effectively the selector bits << 15 */
		/* Low 64K is the base memory */
		bank_base = 32768 + ((selector_reg & 0x30) << 11);
		/* Now are we mapping it high or low */
		if (selector_reg & 1) /* Low */
			bank_base += 32768;
	} else
		bank_base = 0;
}

#if 0
/* Handle reset button if poweron=0;
   handle hard reset or initial poweron if poweron=1 */
void trs_reset(int poweron)
{
    trs_emu_mouse = FALSE;

    /* Close disks opened by Z80 programs */
    do_emt_resetdisk();
    /* Reset devices (Model I SYSRES, Model III/4 RESET) */
    trs_cassette_reset();
    trs_disk_init(poweron); /* also inits trs_hard and trs_stringy */
    /* I'm told that the hard disk controller is enabled on powerup */
    /* XXX should trs_hard_init do this, then? */
    trs_hard_out(TRS_HARD_CONTROL,
		 TRS_HARD_SOFTWARE_RESET|TRS_HARD_DEVICE_ENABLE);

    supermem_base = 0;
    supermem_hi = 0x8000;

    if (trs_model == 5) {
        /* Switch in boot ROM */
	z80_out(0x9C, 1);
    }
    if (trs_model >= 4) {
        /* Turn off various memory map and video mode bits */
	z80_out(0x84, 0);
	if (huffman_ram)
		z80_out(0x94, 0);
        if (hypermem)
                z80_out(0x90, 0);
    }
    if (trs_model >= 3) {
	grafyx_write_mode(0);
	trs_interrupt_mask_write(0);
	trs_nmi_mask_write(0);
    }
    if (trs_model == 3) {
        grafyx_m3_reset();
    }
    if (trs_model == 1) {
	hrg_onoff(0);		/* Switch off HRG1B hi-res graphics. */
	bank_base = 0;
	selector_reg = 0;
    }
    trs_kb_reset();  /* Part of keyboard stretch kludge */
    clear_key_queue(); /* init the key queue */

    trs_cancel_event();
    trs_timer_interrupt(0);
    if (poweron || trs_model >= 4) {
        /* Reset processor */
	z80_reset();
	/* Initialize memory, ROM & timer */
	mem_init();
	trs_rom_init();
	trs_timer_init();
	if (trs_show_led) {
	  trs_disk_led(-1, -1);
	  trs_hard_led(-1, -1);
	  trs_turbo_led();
	}
    } else {
	trs_timer_speed(0);
	/* Signal a nonmaskable interrupt. */
	trs_reset_button_interrupt(1);
	trs_schedule_event(trs_reset_button_interrupt, 0, 2000);
    }
    /* Clear screen */
    screen_init();
}
#endif

void mem_map(int which)
{
    memory_map = which + (trs_model << 4) + (romin << 2);
}

void mem_romin(int state)
{
    romin = (state & 1);
    memory_map = (memory_map & ~4) + (romin << 2);
}

void mem_init()
{
    /* Initialize RAM, ROM & Video memory */
    memset(memory, 0, sizeof(memory));
    memset(video, 0, sizeof(video));

    if (trs_model < 4)
        trs_video_size = 1024;
    else
        trs_video_size = MAX_VIDEO_SIZE;

#ifdef SUPERMEM
    /* We map the SuperMem separately, otherwise it can get really
       confusing when combining with other stuff */
    if (supermem_ram == NULL)
        supermem_ram = (Uchar *) calloc(MAX_SUPERMEM_SIZE + 1, 1);
    else
       memset(supermem_ram, 0, MAX_SUPERMEM_SIZE + 1);
#endif
    mem_map(0);
    mem_bank(0);
    mem_video_page(0);
#if 0
    if (trs_model == 5) {
	z80_out(0x9C, 1);
    }
#endif
}

int mem_read(unsigned int address)
{
    address &= 0xffff; /* allow callers to be sloppy */
 
    /* There are some adapters that sit above the system and
       either intercept before the hardware proper, or adjust
       the address. Deal with these first so that we take their
       output and feed it into the memory map */

#ifdef SUPERMEM
    /* The SuperMem sits between the system and the Z80 */
    if (supermem) {
      if (!((address ^ supermem_hi) & 0x8000))
          return supermem_ram[supermem_base + (address & 0x7FFF)];
      /* Otherwise the request comes from the system */
    }
#endif
    switch (memory_map) {
      case 0x30: /* Model III */
	if (address >= RAM_START) return memory[address];
	//if (address == PRINTER_ADDRESS)	return trs_printer_read();
	if (address < trs_rom_size) return rom[address];
	if (address >= VIDEO_START) {
	  byte character;
          if (trs_screen.getChar(address - VIDEO_START, character)) {
            return character;
	  }
	  return memory[address];
	  //return grafyx_m3_read_byte(address - VIDEO_START);
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x40: /* Model 4 map 0 */
	if (address >= RAM_START) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    return memory[address + bank_offset[address >> 15]];
	}
	//if (address == PRINTER_ADDRESS) return trs_printer_read();
	if (address < trs_rom_size) return rom[address];
	if (address >= VIDEO_START) {
	  return video[address + video_offset];
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x54: /* Model 4P map 0, boot ROM in */
      case 0x55: /* Model 4P map 1, boot ROM in */
	if (address < trs_rom_size) return rom[address];
	/* else fall thru */
      case 0x41: /* Model 4 map 1 */
      case 0x50: /* Model 4P map 0, boot ROM out */
      case 0x51: /* Model 4P map 1, boot ROM out */
	if (address >= RAM_START || address < KEYBOARD_START) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    return memory[address + bank_offset[address >> 15]];
	}
	if (address >= VIDEO_START) {
	  return video[address + video_offset];
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x42: /* Model 4 map 2 */
      case 0x52: /* Model 4P map 2, boot ROM out */
      case 0x56: /* Model 4P map 2, boot ROM in */
	if (address < 0xf400) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    return memory[address + bank_offset[address >> 15]];
	}
	if (address >= 0xf800) {
	  return video[address - 0xf800];
	}
	return trs_kb_mem_read(address);

      case 0x43: /* Model 4 map 3 */
      case 0x53: /* Model 4P map 3, boot ROM out */
      case 0x57: /* Model 4P map 3, boot ROM in */
	assert(address + bank_offset[address >> 15] < sizeof(memory));
	return memory[address + bank_offset[address >> 15]];

    }
    /* not reached */
    assert(0);
    return 0xff;
}


void mem_write(unsigned int address, int value)
{
    address &= 0xffff;

#ifdef SUPERMEM
    /* The SuperMem sits between the system and the Z80 */
    if (supermem) {
      if (!((address ^ supermem_hi) & 0x8000)) {
          supermem_ram[supermem_base + (address & 0x7FFF)] = value;
          return;
      }
      /* Otherwise the request comes from the system */
    }
#endif

    switch (memory_map) {
      case 0x30: /* Model III */
	if (address >= RAM_START) {
	    memory[address] = value;
	} else if (address >= VIDEO_START) {
	  int vaddr = address + video_offset;
          //  if (grafyx_m3_write_byte(vaddr, value)) return;
          if (video[vaddr] != value) {
            video[vaddr] = value;
            trs_screen.drawChar(vaddr, value);
          }
	} else if (address == PRINTER_ADDRESS) {
	    //trs_printer_write(value);
	}
	break;

      case 0x40: /* Model 4 map 0 */
      case 0x50: /* Model 4P map 0, boot ROM out */
      case 0x54: /* Model 4P map 0, boot ROM in */
	if (address >= RAM_START) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= VIDEO_START) {
	  int vaddr = address + video_offset;
          if (video[vaddr] != value) {
            video[vaddr] = value;
            trs_screen.drawChar(vaddr, value);
          }
	} else if (address == PRINTER_ADDRESS) {
	  //trs_printer_write(value);
	}
	break;

      case 0x41: /* Model 4 map 1 */
      case 0x51: /* Model 4P map 1, boot ROM out */
      case 0x55: /* Model 4P map 1, boot ROM in */
	if (address >= RAM_START || address < KEYBOARD_START) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= VIDEO_START) {
	  int vaddr = address + video_offset;
          if (video[vaddr] != value) {
            video[vaddr] = value;
            trs_screen.drawChar(vaddr, value);
           }
	}
	break;

      case 0x42: /* Model 4 map 2 */
      case 0x52: /* Model 4P map 2, boot ROM out */
      case 0x56: /* Model 4P map 2, boot ROM in */
	if (address < 0xf400) {
	    assert(address + bank_offset[address >> 15] < sizeof(memory));
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= 0xf800) {
	  int vaddr = address - 0xf800;
          if (video[vaddr] != value) {
            video[vaddr] = value;
            trs_screen.drawChar(vaddr, value);
          }
	}
	break;

      case 0x43: /* Model 4 map 3 */
      case 0x53: /* Model 4P map 3, boot ROM out */
      case 0x57: /* Model 4P map 3, boot ROM in */
        assert(address + bank_offset[address >> 15] < sizeof(memory));
	memory[address + bank_offset[address >> 15]] = value;
	break;
      default:
        assert(0);
    }
}

/*
 * Words are stored with the low-order byte in the lower address.
 */
int mem_read_word(int address)
{
    int rval;

    rval = mem_read(address++);
    rval |= mem_read(address & 0xffff) << 8;
    return rval;
}

void mem_write_word(int address, int value)
{
    mem_write(address++, value & 0xff);
    mem_write(address, value >> 8);
}
