// Memory emulation.
#pragma once

#include "../common.h"
#include "cpu.h"
#include "gpu.h"

#define GB_MEMORY_SIZE 0x10000
#define GB_BOOTROM_SIZE 0x100

void gb_memory_init();
uint8_t gb_memory_read(uint16_t);
void gb_memory_write(uint16_t, uint8_t);
uint8_t gb_memory_raw_read(uint16_t);
void gb_memory_raw_write(uint16_t, uint8_t);
