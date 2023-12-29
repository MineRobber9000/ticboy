// Memory emulation.

#include "../tic80.h"
#include "../rand.h"
#include "memory.h"
#include <string.h>

uint8_t gb_memory[GB_MEMORY_SIZE];
uint8_t gb_bootrom[GB_BOOTROM_SIZE];

uint8_t bootrom_mapped;

uint8_t gb_memory_read(uint16_t addr) {
	if (bootrom_mapped && addr<0x100) return gb_bootrom[addr];
	if (addr==0xff0f) { // rIF - interrupt flags
		return gb_cpu_interrupt.flags;
	}
	if (addr==0xff40) { // rLCDC - LCD control
		return lcdc;
	}
	if (addr==0xff42) { // rSCY - bg scroll Y
		return scy;
	}
	if (addr==0xff43) { // rSCX - bg scroll X
		return scx;
	}
	if (addr==0xff44) { // rLY - scanline
		return gpu_scanline;
	}
	if (addr==0xffff) { // rIE - interrupt enable
		return gb_cpu_interrupt.enable;
	}
	return gb_memory[addr];
}

void gb_memory_write(uint16_t addr, uint8_t val) {
	// VRAM tiles
	if (addr>=0x8000 && addr<=0x97ff) gb_gpu_on_tilewrite(addr-0x8000,val);
	// VRAM tilemaps
	if (addr>=0x9800 && addr<=0x9fff) gb_gpu_on_tilemapwrite(addr-0x9800,val);
	// rLCDC - LCD control
	if (addr==0xff40) lcdc=val;
	// rSCY, rSCX - bg scroll Y/X
	if (addr==0xff42) scy=val;
	if (addr==0xff43) scx=val;
	// rBGP, rOBP0, rOBP1 - palettes
	if (addr==0xff47) bgp=val;
	if (addr==0xff48) obp0=val;
	if (addr==0xff49) obp1=val;
	// unmaps bootrom
	if (addr==0xff50) bootrom_mapped=0;
	// rIF - interrupt flags
	if (addr==0xff0f) gb_cpu_interrupt.flags = val;
	// rIE - interrupt enable
	if (addr==0xffff) gb_cpu_interrupt.enable = val;
	gb_memory[addr]=val;
}

uint8_t gb_memory_raw_read(uint16_t addr) {
	if (bootrom_mapped && addr<0x100) return gb_bootrom[addr];
	return gb_memory[addr];
}

void gb_memory_raw_write(uint16_t addr, uint8_t val) {
	gb_memory[addr]=val;
}

void gb_memory_init() {
	// fill memory with random garbage
	// except certain areas
	srand(tstamp());
	for (int i=0;i<GB_MEMORY_SIZE;++i) {
		if (i<0x8000) { // ROM
			gb_memory[i]=0xff;
		} else {
			gb_memory[i]=rand()&0xff;
		}
	}
	// ensure our data is loaded
	sync(0,0,0);
	// load bootrom
	for (int i=0;i<GB_BOOTROM_SIZE;++i) {
		gb_bootrom[i]=TILES[i];
		TILES[i]=0; // clean up after ourselves
	}
	// for larger ROMs, more complicated methods will be necessary
	// but for Geometrix (our packaged game), the ROM is small enough that no important data gets left out of the map
	for (int i=0;i<32640;++i) {
		gb_memory[i]=MAP[i];
		MAP[i]=0; // clean up after ourselves
	}
	// ensure the memory functions treat the bootrom as mapped
	bootrom_mapped = 1;
}
