#include "gpu.h"

gb_gpu_mode_e gb_gpu_mode;
uint16_t gpu_ticks;
uint8_t gpu_scanline;
int last_ticks;
uint8_t lcdc;
uint8_t scx;
uint8_t scy;
uint8_t bgp;
uint8_t obp0;
uint8_t obp1;

void gb_gpu_init() {
	lcdc = 0;
	gpu_ticks = 0;
	gpu_scanline = 0;
	last_ticks = 0;
}

void gb_gpu_tick() {
	if (!(lcdc&0x80)) return;
	gpu_ticks += (ticks - last_ticks);
	last_ticks = ticks;

	switch (gb_gpu_mode) {
		case GB_GPU_MODE_HBLANK: {
			if (gpu_ticks>=408) {
				gpu_scanline++;
				if (gpu_scanline==143) {
					INTERRUPT_REQUEST(INTERRUPT_VBLANK);
					gb_gpu_mode = GB_GPU_MODE_VBLANK;
				} else {
					gb_gpu_mode = GB_GPU_MODE_OAM;
				}
				gpu_ticks-=408;
			}
			break;
		}
		case GB_GPU_MODE_VBLANK: {
			if (gpu_ticks>=912) {
				gpu_scanline++;
				if (gpu_scanline>153) {
					gpu_scanline = 0;
					gb_gpu_mode = GB_GPU_MODE_OAM;
				}
				gpu_ticks-=912;
			}
			break;
		}
		case GB_GPU_MODE_OAM: {
			if (gpu_ticks>=160) {
				gb_gpu_mode = GB_GPU_MODE_VRAM;
				gpu_ticks-=160;
			}
			break;
		}
		case GB_GPU_MODE_VRAM: {
			if (gpu_ticks>=344) {
				gb_gpu_mode = GB_GPU_MODE_HBLANK;
				gb_gpu_drawscanline();
				gpu_ticks-=344;
			}
			break;
		}
	}
}

void _settile(uint32_t bitaddr, uint8_t val) {
	for (int i=7;i>=0;--i) {
		uint8_t b = 0;
		if (val&(1<<i)) b = 1;
		poke1(bitaddr,b);
		bitaddr+=2;
	}
}

void gb_gpu_on_tilewrite(uint16_t addr, uint8_t val) {
	uint16_t s = addr>>4;
	uint8_t b = addr&0xf;
	if (s>=0 && s<=0x7f) {
		uint32_t byteaddr = 0x4000 + (s>>1)*32 + (s&1)*2 + (s>>16)*32*8 + (b>>1)*4;
		if (b&1) _settile(byteaddr*8+1,val);
		else _settile(byteaddr*8,val);
	} else if (s>=0x80 && s<=0xff) {
		uint32_t byteaddr = 0x4000 + (s>>1)*32 + (s&1)*2 + (s>>16)*32*8 + (b>>1)*4;
		if (b&1) _settile(byteaddr*8+1,val);
		else _settile(byteaddr*8,val);
		byteaddr+=0x4000;
		if (b&1) _settile(byteaddr*8+1,val);
		else _settile(byteaddr*8,val);
	} else {
		uint32_t byteaddr = 0x8000 + (s>>1)*32 + (s&1)*2 + (s>>16)*32*8 + (b>>1)*4;
		if (b&1) _settile(byteaddr*8+1,val);
		else _settile(byteaddr*8,val);
	}
}

void gb_gpu_on_tilemapwrite(uint16_t addr, uint8_t val) {
	uint16_t idx = addr&0x3ff;
	uint16_t x = idx&0x1f;
	uint16_t y = idx>>5;
	if (addr&0x400) x+=64;
	mset(x,y,val);
	mset(x+32,y,val);
	mset(x,y+32,val);
	mset(x+32,y+32,val);
}

void _apply_palette(uint8_t pal) {
	poke4((intptr_t)(&FRAMEBUFFER->PALETTE_MAP)*2+0,pal&0b11);
	poke4((intptr_t)(&FRAMEBUFFER->PALETTE_MAP)*2+1,(pal>>2)&0b11);
	poke4((intptr_t)(&FRAMEBUFFER->PALETTE_MAP)*2+2,(pal>>4)&0b11);
	poke4((intptr_t)(&FRAMEBUFFER->PALETTE_MAP)*2+3,(pal>>6)&0b11);
}

void _apply_tileset(uint8_t set) {
	poke4((intptr_t)(&FRAMEBUFFER->BLIT_SEGMENT)*2,(set ? 6 : 4));
}

uint8_t *sprite_transcolors = {0};
void gb_gpu_drawscanline() {
	// ignore scanlines outside of TIC-80 screen
	if (gpu_scanline<4 || gpu_scanline>140) return;
	clip(0,gpu_scanline-4,160,1);
	_apply_palette(bgp);
	_apply_tileset(0);
	map((scx>>3),(scy>>3),32,32,-(scx&7),-(scy&7),0,0,1,0);
	for (int i=0;i<40;++i) {
		int16_t sprite_y = gb_memory_raw_read(0xfe00+(i*4))-16;
		int16_t sprite_x = gb_memory_raw_read(0xfe00+(i*4)+1)-8;
		uint8_t sprite_id = gb_memory_raw_read(0xfe00+(4*i)+2);
		uint8_t sprite_flags = gb_memory_raw_read(0xfe00+(4*i)+3);
		if (sprite_y>=4 && sprite_y < 140 && sprite_x>=-7 && sprite_x<160) {
			_apply_palette(OAM_FLAG(sprite_flags,OAM_PALETTE) ? obp1 : obp0);
			uint8_t flip = 0;
			if (OAM_FLAG(sprite_flags,OAM_YFLIP)) flip |= 2;
			if (OAM_FLAG(sprite_flags,OAM_XFLIP)) flip |= 1;
			if (lcdc & 0b100) {
				spr(sprite_id&0xfe,sprite_x,sprite_y-4,sprite_transcolors,1,1,flip,0,1,1);
				spr(sprite_id|1,sprite_x,sprite_y-4+8,sprite_transcolors,1,1,flip,0,1,1);
			} else {
				spr(sprite_id,sprite_x,sprite_y-4,sprite_transcolors,1,1,flip,0,1,1);
			}
		}
	}
}
