#pragma once

#include "../common.h"
#include "memory.h"
#include "../tic80.h"

typedef struct {
	union {
		uint16_t af;
		struct {
			uint8_t f;
			uint8_t a;
		};
	};
	union {
		uint16_t bc;
		struct {
			uint8_t c;
			uint8_t b;
		};
	};
	union {
		uint16_t de;
		struct {
			uint8_t e;
			uint8_t d;
		};
	};
	union {
		uint16_t hl;
		struct {
			uint8_t l;
			uint8_t h;
		};
	};
	uint16_t pc;
	uint16_t sp;
} gb_cpu_registers_t;

extern gb_cpu_registers_t gb_cpu_registers;

typedef struct {
	uint8_t master;
	uint8_t master_scheduled;
	uint8_t enable;
	uint8_t flags;
} gb_cpu_interrupt_t;

extern gb_cpu_interrupt_t gb_cpu_interrupt;

extern int ticks;

#define FLAG_ZERO (1<<7)
#define FLAG_NEGATIVE (1<<6)
#define FLAG_HALFCARRY (1<<5)
#define FLAG_CARRY (1<<4)

#define FLAG_SET(x) (gb_cpu_registers.f |= x)
#define FLAG_CLEAR(x) (gb_cpu_registers.f &= ~(x))
#define FLAG_ISSET(x) (gb_cpu_registers.f & (x))
#define FLAG_SETIF(cond,x) if (cond) FLAG_SET(x); \
else FLAG_CLEAR(x);
#define FLAG_CLEARIF(cond,x) if (cond) FLAG_CLEAR(x); \
else FLAG_SET(x);

#define MSB(x) ((x)>>8)&0xff
#define LSB(x) (x)&0xff

#define INTERRUPT_VBLANK (1<<0)
#define INTERRUPT_LCDSTAT (1<<1)
#define INTERRUPT_TIMER (1<<2)
#define INTERRUPT_SERIAL (1<<3)
#define INTERRUPT_JOYPAD (1<<4)

#define INTERRUPT_REQUEST(x) (gb_cpu_interrupt.flags |= (x))
#define INTERRUPT_CLEAR(x) (gb_cpu_interrupt.flags &= ~(x))
#define INTERRUPT_ISSET(x) (gb_cpu_interrupt.flags & (x))

void gb_cpu_init();
void gb_cpu_tick();
void gb_cpu_trace_pc();

extern int gb_exit;
