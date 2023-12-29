#pragma once

#include "memory.h"
#include "cpu.h"
#include "../tic80.h"

typedef enum {
	GB_GPU_MODE_HBLANK,
	GB_GPU_MODE_VBLANK,
	GB_GPU_MODE_OAM,
	GB_GPU_MODE_VRAM
} gb_gpu_mode_e;

extern gb_gpu_mode_e gb_gpu_mode;
extern uint8_t gpu_on;
extern uint16_t gpu_ticks;
extern uint8_t gpu_scanline;
extern uint8_t lcdc;
extern uint8_t scx;
extern uint8_t scy;
extern uint8_t bgp;
extern uint8_t obp0;
extern uint8_t obp1;

void gb_gpu_init();
void gb_gpu_tick();
void gb_gpu_on_tilewrite(uint16_t, uint8_t);
void gb_gpu_on_tilemapwrite(uint16_t, uint8_t);

void gb_gpu_drawscanline();

#define OAM_PRIORITY (1<<7)
#define OAM_YFLIP (1<<6)
#define OAM_XFLIP (1<<5)
#define OAM_PALETTE (1<<4)

#define OAM_FLAG(flags,x) ((flags)&(x))
