#include "cpu.h"
#include "../tic80.h"
#include <string.h>

WASM_IMPORT("snprintf")
int snprintf ( char * s, size_t n, const char * format, ... );

char debug_buffer[100];
void debug_gb_address(const char * label, uint16_t value) {
	snprintf(debug_buffer,100,"%s: %04X",label,value);
	trace(debug_buffer,-1);
}
void debug_gb_value(const char * label, uint8_t value) {
	snprintf(debug_buffer,100,"%s: %02X",label,value);
	trace(debug_buffer,-1);
}

gb_cpu_registers_t gb_cpu_registers;
gb_cpu_interrupt_t gb_cpu_interrupt;

int ticks;

void gb_cpu_init() {
	memset(&gb_cpu_registers,0,sizeof(gb_cpu_registers_t));
	memset(&gb_cpu_interrupt,0,sizeof(gb_cpu_interrupt_t));
	ticks = 0;
}

uint8_t cpu_read_u8_at_pc() {
	return gb_memory_read(gb_cpu_registers.pc++);
}

uint16_t cpu_read_u16_at_pc() {
	return (uint16_t)(cpu_read_u8_at_pc() | (cpu_read_u8_at_pc()<<8));
}

uint8_t add(uint8_t x, uint8_t y) {
	FLAG_CLEAR(FLAG_NEGATIVE);
	uint16_t result = (uint16_t)x+(uint16_t)y;

	FLAG_SETIF((result&0xff00),FLAG_CARRY);

	uint8_t res8 = result&0xff;
	FLAG_CLEARIF(res8,FLAG_ZERO);

	FLAG_SETIF((x ^ y ^ res8)&0x10,FLAG_HALFCARRY);
	return res8;
}

uint8_t sub(uint8_t x, uint8_t y) {
	FLAG_SET(FLAG_NEGATIVE);

	FLAG_SETIF((y>x),FLAG_CARRY);

	FLAG_SETIF(((y&0xf)>(x&0xf)),FLAG_HALFCARRY);

	uint8_t result = x - y;

	FLAG_CLEARIF(result,FLAG_ZERO);

	return result;
}

void _xor(uint8_t val) {
	gb_cpu_registers.a ^= val;

	FLAG_CLEARIF(gb_cpu_registers.a, FLAG_ZERO);

	FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY | FLAG_CARRY);
}

void _push(uint16_t val) {
	gb_cpu_registers.sp--;
	gb_memory_write(gb_cpu_registers.sp--,MSB(val));
	gb_memory_write(gb_cpu_registers.sp,LSB(val));
}

uint16_t _pop() {
	uint16_t ret = (uint16_t)gb_memory_read(gb_cpu_registers.sp);
	ret |= gb_memory_read(++gb_cpu_registers.sp)<<8;
	++gb_cpu_registers.sp;
	return ret;
}

void _call(uint16_t addr) {
	_push(gb_cpu_registers.pc);
	gb_cpu_registers.pc = addr;
}

uint8_t get_reg8(uint8_t reg) {
	// b, c, d, e, h, l, [hl], a
	// [hl] is the memory address at hl
	switch (reg) {
		case 0: return gb_cpu_registers.b;
		case 1: return gb_cpu_registers.c;
		case 2: return gb_cpu_registers.d;
		case 3: return gb_cpu_registers.e;
		case 4: return gb_cpu_registers.h;
		case 5: return gb_cpu_registers.l;
		case 6: return gb_memory_read(gb_cpu_registers.hl);
		case 7: return gb_cpu_registers.a;
	}
	return 0;
}

void set_reg8(uint8_t reg, uint8_t val) {
	// see above
	switch (reg) {
		case 0: {
			gb_cpu_registers.b = val;
			return;
		}
		case 1: {
			gb_cpu_registers.c = val;
			return;
		}
		case 2: {
			gb_cpu_registers.d = val;
			return;
		}
		case 3: {
			gb_cpu_registers.e = val;
			return;
		}
		case 4: {
			gb_cpu_registers.h = val;
			return;
		}
		case 5: {
			gb_cpu_registers.l = val;
			return;
		}
		case 6: {
			gb_memory_write(gb_cpu_registers.hl,val);
			return;
		}
		case 7: {
			gb_cpu_registers.a = val;
			return;
		}
	}
}

uint16_t get_reg16(uint8_t reg, uint8_t pushpop) {
	// bc, de, hl, sp/af
	// pushpop selects sp or af (af is only in push/pop)
	switch (reg) {
		case 0: return gb_cpu_registers.bc;
		case 1: return gb_cpu_registers.de;
		case 2: return gb_cpu_registers.hl;
		case 3: return (pushpop ? gb_cpu_registers.af : gb_cpu_registers.sp);
	}
	return 0;
}

void set_reg16(uint8_t reg, uint8_t pushpop, uint16_t val) {
	// see above
	switch (reg) {
		case 0: {
			gb_cpu_registers.bc = val;
			return;
		}
		case 1: {
			gb_cpu_registers.de = val;
			return;
		}
		case 2: {
			gb_cpu_registers.hl = val;
			return;
		}
		case 3: {
			if (pushpop) gb_cpu_registers.af = val;
			else gb_cpu_registers.sp = val;
			return;
		}
	}
}

uint8_t get_ind(uint8_t ind) {
	// [bc], [de], [hl+], [hl-]
	// [hl] is reg8 number 6
	switch (ind) {
		case 0: {
			return gb_memory_read(gb_cpu_registers.bc);
		}
		case 1: {
			return gb_memory_read(gb_cpu_registers.de);
		}
		case 2: {
			return gb_memory_read(gb_cpu_registers.hl++);
		}
		case 3: {
			return gb_memory_read(gb_cpu_registers.hl--);
		}
	}
	return 0;
}

void set_ind(uint8_t ind, uint8_t val) {
	// see above
	switch (ind) {
		case 0: {
			gb_memory_write(gb_cpu_registers.bc,val);
			break;
		}
		case 1: {
			gb_memory_write(gb_cpu_registers.de,val);
			break;
		}
		case 2: {
			gb_memory_write(gb_cpu_registers.hl++,val);
			break;
		}
		case 3: {
			gb_memory_write(gb_cpu_registers.hl--,val);
			break;
		}
	}
}

void tick_cb_opcode() {
	uint8_t instr = cpu_read_u8_at_pc();
	uint8_t operation = instr>>3;
	uint8_t reg = instr&3;
	uint8_t val = get_reg8(reg);
	// all operations take 8 t-states
	// except those on [hl], which take 16
	ticks+=8;
	if (reg==6) ticks+=8;
	switch (operation) {
		case 0: { // rlc [reg]
			FLAG_SETIF((val&(1<<7)),FLAG_CARRY);
			val = (uint8_t)((val<<1) | (val>>7));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 1: { // rrc [reg]
			FLAG_SETIF((val&1),FLAG_CARRY);
			val = (uint8_t)((val>>1) | (val<<7));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 2: { // rl [reg]
			uint8_t bit = 0;
			if (FLAG_ISSET(FLAG_CARRY)) bit=1;
			FLAG_SETIF((val&(1<<7)),FLAG_CARRY);
			val = (uint8_t)((val<<1) | bit);
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 3: { // rr [reg]
			uint8_t bit = 0;
			if (FLAG_ISSET(FLAG_CARRY)) bit=1;
			FLAG_SETIF((val&1),FLAG_CARRY);
			val = (uint8_t)((val>>1) | (bit<<7));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 4: { // sla [reg]
			FLAG_SETIF((val&(1<<7)),FLAG_CARRY);
			val = (val<<1)&0xff;
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 5: { // sra [reg]
			FLAG_SETIF((val&1),FLAG_CARRY);
			val = (uint8_t)((val&(1<<7)) | (val>>1));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 6: { // swap [reg]
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY | FLAG_CARRY);
			val = (uint8_t)((val << 4) | (val >> 4));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			return;
		}
		case 7: { // srl [reg]
			FLAG_SETIF((val&1),FLAG_CARRY);
			val = (uint8_t)(0 | (val>>1));
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE | FLAG_HALFCARRY);
			return;
		}
		case 8:    // bit 0, [reg]
		case 9:    // bit 1, [reg]
		case 10:   // bit 2, [reg]
		case 11:   // bit 3, [reg]
		case 12:   // bit 4, [reg]
		case 13:   // bit 5, [reg]
		case 14:   // bit 6, [reg]
		case 15: { // bit 7, [reg]
			uint8_t bitnum = operation&0b111;
			FLAG_SETIF((val&(1<<bitnum)),FLAG_ZERO)
			FLAG_CLEAR(FLAG_NEGATIVE);
			FLAG_SET(FLAG_HALFCARRY); // apparently????
			return;
		}
		case 16:   // res 0, [reg]
		case 17:   // res 1, [reg]
		case 18:   // res 2, [reg]
		case 19:   // res 3, [reg]
		case 20:   // res 4, [reg]
		case 21:   // res 5, [reg]
		case 22:   // res 6, [reg]
		case 23: { // res 7, [reg]
			uint8_t bitnum = operation&0b111;
			val = val & ~(1<<bitnum);
			set_reg8(reg,val);
			return;
		}
		case 24:   // set 0, [reg]
		case 25:   // set 1, [reg]
		case 26:   // set 2, [reg]
		case 27:   // set 3, [reg]
		case 28:   // set 4, [reg]
		case 29:   // set 5, [reg]
		case 30:   // set 6, [reg]
		case 31: { // set 7, [reg]
			uint8_t bitnum = operation&0b111;
			val = (uint8_t)(val | (1<<bitnum));
			set_reg8(reg,val);
			return;
		}
	}
}

int gb_exit=0;
void gb_cpu_tick() {
	if (gb_cpu_interrupt.master && (gb_cpu_interrupt.enable & gb_cpu_interrupt.flags)) {
		uint8_t interrupts_serviceable = (gb_cpu_interrupt.enable & gb_cpu_interrupt.flags);
		if (interrupts_serviceable & INTERRUPT_VBLANK) {
			INTERRUPT_CLEAR(INTERRUPT_VBLANK);
			gb_cpu_interrupt.master = 0;
			gb_cpu_interrupt.master_scheduled = 0;
			_call(0x40); // vblank handler
			ticks+=20;
		} else if (interrupts_serviceable & INTERRUPT_LCDSTAT) {
			INTERRUPT_CLEAR(INTERRUPT_LCDSTAT);
			gb_cpu_interrupt.master = 0;
			gb_cpu_interrupt.master_scheduled = 0;
			_call(0x48); // lcdstat handler
			ticks+=20;
		} else if (interrupts_serviceable & INTERRUPT_TIMER) {
			INTERRUPT_CLEAR(INTERRUPT_TIMER);
			gb_cpu_interrupt.master = 0;
			gb_cpu_interrupt.master_scheduled = 0;
			_call(0x50); // timer handler
			ticks+=20;
		} else if (interrupts_serviceable & INTERRUPT_SERIAL) {
			INTERRUPT_CLEAR(INTERRUPT_SERIAL);
			gb_cpu_interrupt.master = 0;
			gb_cpu_interrupt.master_scheduled = 0;
			_call(0x58); // serial handler
			ticks+=20;
		} else if (interrupts_serviceable & INTERRUPT_JOYPAD) {
			INTERRUPT_CLEAR(INTERRUPT_JOYPAD);
			gb_cpu_interrupt.master = 0;
			gb_cpu_interrupt.master_scheduled = 0;
			_call(0x60); // joypad handler
			ticks+=20;
		}
	}
	if (gb_cpu_interrupt.master_scheduled) { // ei was last instruction?
		gb_cpu_interrupt.master = 1; // enable interrupts
		gb_cpu_interrupt.master_scheduled = 0;
	}
	int old_ticks = ticks*1;
	uint8_t instr = cpu_read_u8_at_pc();
	// instructions are (vaguely) sorted by opcode
	// multiple opcodes that handle the same (so JR/JR <cond>, RST) are sorted
	// by their last opcode
	switch (instr) {
		case 0x00: { // nop
			ticks+=4;
			break;
		}
		case 0x01:   // ld bc, nn
		case 0x11:   // ld de, nn
		case 0x21:   // ld hl, nn
		case 0x31: { // ld sp, nn
			uint8_t reg = (instr>>4)&0b11;
			set_reg16(reg,0,cpu_read_u16_at_pc());
			ticks+=12;
			break;
		}
		case 0x02:   // ld [bc],  a
		case 0x12:   // ld [de],  a
		case 0x22:   // ld [hl+], a
		case 0x32: { // ld [hl-], a
			uint8_t ind = (instr>>4)&0b11;
			set_ind(ind,gb_cpu_registers.a);
			ticks+=8;
			break;
		}
		case 0x03:   // inc bc
		case 0x13:   // inc de
		case 0x23:   // inc hl
		case 0x33: { // inc sp
			uint8_t reg = (instr>>4)&0b11;
			set_reg16(reg,0,get_reg16(reg,0)+1);
			ticks+=8;
			break;
		}
		case 0x18:   // jr     e
		case 0x20:   // jr nz, e
		case 0x28:   // jr z,  e
		case 0x30:   // jr nc, e
		case 0x38: { // jr c,  e
			int8_t offset = (int8_t)cpu_read_u8_at_pc();
			ticks+=8;
			if (
				(instr==0x18) // unconditional jr
				|| (instr==0x20 && !FLAG_ISSET(FLAG_ZERO)) // nz
				|| (instr==0x28 && FLAG_ISSET(FLAG_ZERO)) // z
				|| (instr==0x30 && !FLAG_ISSET(FLAG_CARRY)) // nc
				|| (instr==0x38 && FLAG_ISSET(FLAG_CARRY)) // c
			) {
				ticks+=4;
				gb_cpu_registers.pc+=offset;
			}
			break;
		}
		case 0x0A:   // ld a, [bc]
		case 0x1A:   // ld a, [de]
		case 0x2A:   // ld a, [hl+]
		case 0x3A: { // ld a, [hl-]
			uint8_t ind = (instr>>4)&0b11;
			gb_cpu_registers.a = get_ind(ind);
			ticks+=8;
			break;
		}
		case 0x0B:   // dec bc
		case 0x1B:   // dec de
		case 0x2B:   // dec hl
		case 0x3B: { // dec sp
			uint8_t reg = (instr>>4)&0b11;
			set_reg16(reg,0,get_reg16(reg,0)-1);
			ticks+=8;
			break;
		}
		case 0x04:   // inc b
		case 0x0C:   // inc c
		case 0x14:   // inc d
		case 0x1C:   // inc e
		case 0x24:   // inc h
		case 0x2C:   // inc l
		case 0x34:   // inc [hl]
		case 0x3C: { // inc a
			uint8_t reg = (instr>>3)&0b111;
			uint8_t val = get_reg8(reg);
			FLAG_SETIF((val&0x0f)==0x0f,FLAG_HALFCARRY);
			val++;
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_CLEAR(FLAG_NEGATIVE);
			ticks+=4;
			if (reg==6) ticks+=8;
			break;
		}
		case 0x05:   // dec b
		case 0x0D:   // dec c
		case 0x15:   // dec d
		case 0x1D:   // dec e
		case 0x25:   // dec h
		case 0x2D:   // dec l
		case 0x35:   // dec [hl]
		case 0x3D: { // dec a
			uint8_t reg = (instr>>3)&0b111;
			uint8_t val = get_reg8(reg);
			FLAG_CLEARIF((val&0x0f),FLAG_HALFCARRY);
			val--;
			FLAG_CLEARIF(val,FLAG_ZERO);
			set_reg8(reg,val);
			FLAG_SET(FLAG_NEGATIVE);
			ticks+=4;
			if (reg==6) ticks+=8;
			break;
		}
		case 0x06:   // ld b,    n
		case 0x0E:   // ld c,    n
		case 0x16:   // ld d,    n
		case 0x1E:   // ld e,    n
		case 0x26:   // ld h,    n
		case 0x2E:   // ld l,    n
		case 0x36:   // ld [hl], n
		case 0x3E: { // ld a,    n
			uint8_t reg = (instr>>3)&0b111;
			set_reg8(reg,cpu_read_u8_at_pc());
			ticks+=8;
			break;
		}
		case 0x76: { // halt
			trace("Haven't implemented HALT yet...",-1);
			tic80_exit();
			gb_exit=1;
			break;
		}
		// sorry in advance:
		case 0x40:   // ld b,    b
		case 0x41:   // ld b,    c
		case 0x42:   // ld b,    d
		case 0x43:   // ld b,    e
		case 0x44:   // ld b,    h
		case 0x45:   // ld b,    l
		case 0x46:   // ld b,    [hl]
		case 0x47:   // ld b,    a
		case 0x48:   // ld c,    b
		case 0x49:   // ld c,    c
		case 0x4A:   // ld c,    d
		case 0x4B:   // ld c,    e
		case 0x4C:   // ld c,    h
		case 0x4D:   // ld c,    l
		case 0x4E:   // ld c,    [hl]
		case 0x4F:   // ld c,    a
		case 0x50:   // ld d,    b
		case 0x51:   // ld d,    c
		case 0x52:   // ld d,    d
		case 0x53:   // ld d,    e
		case 0x54:   // ld d,    h
		case 0x55:   // ld d,    l
		case 0x56:   // ld d,    [hl]
		case 0x57:   // ld d,    a
		case 0x58:   // ld e,    b
		case 0x59:   // ld e,    c
		case 0x5A:   // ld e,    d
		case 0x5B:   // ld e,    e
		case 0x5C:   // ld e,    h
		case 0x5D:   // ld e,    l
		case 0x5E:   // ld e,    [hl]
		case 0x5F:   // ld e,    a
		case 0x60:   // ld h,    b
		case 0x61:   // ld h,    c
		case 0x62:   // ld h,    d
		case 0x63:   // ld h,    e
		case 0x64:   // ld h,    h
		case 0x65:   // ld h,    l
		case 0x66:   // ld h,    [hl]
		case 0x67:   // ld h,    a
		case 0x68:   // ld l,    b
		case 0x69:   // ld l,    c
		case 0x6A:   // ld l,    d
		case 0x6B:   // ld l,    e
		case 0x6C:   // ld l,    h
		case 0x6D:   // ld l,    l
		case 0x6E:   // ld l,    [hl]
		case 0x6F:   // ld l,    a
		case 0x70:   // ld [hl], b
		case 0x71:   // ld [hl], c
		case 0x72:   // ld [hl], d
		case 0x73:   // ld [hl], e
		case 0x74:   // ld [hl], h
		case 0x75:   // ld [hl], l    (0x76 is halt...)
		case 0x77:   // ld [hl], a
		case 0x78:   // ld a,    b
		case 0x79:   // ld a,    c
		case 0x7A:   // ld a,    d
		case 0x7B:   // ld a,    e
		case 0x7C:   // ld a,    h
		case 0x7D:   // ld a,    l
		case 0x7E:   // ld a,    [hl]
		case 0x7F: { // ld a,    a
			uint8_t x = (instr>>3)&0b111;
			uint8_t y = instr&0b111;
			set_reg8(x,get_reg8(y));
			ticks+=4;
			if (x==6 || y==6) ticks+=4;
			break;
		}
		case 0x80:   // add a, b
		case 0x81:   // add a, c
		case 0x82:   // add a, d
		case 0x83:   // add a, e
		case 0x84:   // add a, h
		case 0x85:   // add a, l
		case 0x86:   // add a, [hl]
		case 0x87: { // add a, a
			uint8_t reg = instr&0b111;
			gb_cpu_registers.a = add(gb_cpu_registers.a,get_reg8(reg));
			ticks+=4;
			if (reg==6) ticks+=4;
		}
		case 0x88:   // adc a, b
		case 0x89:   // adc a, c
		case 0x8A:   // adc a, d
		case 0x8B:   // adc a, e
		case 0x8C:   // adc a, h
		case 0x8D:   // adc a, l
		case 0x8E:   // adc a, [hl]
		case 0x8F: { // adc a, a
			uint8_t reg = instr&0b111;
			gb_cpu_registers.a = add(gb_cpu_registers.a,get_reg8(reg)+(FLAG_ISSET(FLAG_CARRY) ? 1 : 0));
			ticks+=4;
			if (reg==6) ticks+=4;
		}
		case 0x90:   // sub a, b
		case 0x91:   // sub a, c
		case 0x92:   // sub a, d
		case 0x93:   // sub a, e
		case 0x94:   // sub a, h
		case 0x95:   // sub a, l
		case 0x96:   // sub a, [hl]
		case 0x97: { // sub a, a
			uint8_t reg = instr&0b111;
			gb_cpu_registers.a = add(gb_cpu_registers.a,get_reg8(reg));
			ticks+=4;
			if (reg==6) ticks+=4;
		}
		case 0x98:   // sbc a, b
		case 0x99:   // sbc a, c
		case 0x9A:   // sbc a, d
		case 0x9B:   // sbc a, e
		case 0x9C:   // sbc a, h
		case 0x9D:   // sbc a, l
		case 0x9E:   // sbc a, [hl]
		case 0x9F: { // sbc a, a
			uint8_t reg = instr&0b111;
			gb_cpu_registers.a = add(gb_cpu_registers.a,get_reg8(reg)+(FLAG_ISSET(FLAG_CARRY) ? 1 : 0));
			ticks+=4;
			if (reg==6) ticks+=4;
		}
		case 0xAF: { // xor a
			_xor(gb_cpu_registers.a);
			ticks+=4;
			break;
		}
		case 0xCB: { // PREFIX BYTE!
			tick_cb_opcode();
			break;
		}
		case 0xC0:   // ret  nz
		case 0xC8:   // ret  z
		case 0xC9:   // ret
		case 0xD0:   // ret  nc
		case 0xD8:   // ret  c
		case 0xD9: { // reti
			ticks+=8;
			if (
				(instr == 0xc9) // unconditional ret
				|| (instr == 0xd9) // unconditional reti
				|| (instr == 0xc0 && !FLAG_ISSET(FLAG_ZERO)) // nz
				|| (instr == 0xc8 && FLAG_ISSET(FLAG_ZERO)) // z
				|| (instr == 0xd0 && !FLAG_ISSET(FLAG_CARRY)) // nc
				|| (instr == 0xd8 && FLAG_ISSET(FLAG_CARRY)) // c
			) {
				ticks+=8;
				// taken conditional rets are 20 t-states
				// instead of 16 for some reason
				if (!((instr&0x0f)==0x09)) ticks+=4;
				gb_cpu_registers.pc = _pop();
				// reti returns from interrupt handler
				if (instr==0xd9) gb_cpu_interrupt.master = 1;
			}
			break;
		}
		case 0xC2:   // jp nz, nn
		case 0xC3:   // jp     nn
		case 0xCA:   // jp z,  nn
		case 0xD2:   // jp nc, nn
		case 0xDA: { // jp c,  nn
			ticks+=12;
			uint16_t addr = cpu_read_u16_at_pc();
			if (
				(instr==0xC3) // unconditional jp
				|| (instr==0xC2 && !FLAG_ISSET(FLAG_ZERO)) // nz
				|| (instr==0xCA && FLAG_ISSET(FLAG_ZERO)) // z
				|| (instr==0xD2 && !FLAG_ISSET(FLAG_CARRY)) // nc
				|| (instr==0xDA && FLAG_ISSET(FLAG_CARRY)) // c
			) {
				ticks+=4;
				gb_cpu_registers.pc = addr;
			}
			break;
		}
		case 0xC4:   // call nz, nn
		case 0xCC:   // call z,  nn
		case 0xCD:   // call     nn
		case 0xD4:   // call nc, nn
		case 0xDC: { // call c,  nn
			uint16_t addr = cpu_read_u16_at_pc();
			ticks+=12;
			if (
				(instr == 0xCD) // unconditional call
				|| (instr == 0xC4 && !FLAG_ISSET(FLAG_ZERO)) // nz
				|| (instr == 0xCC && FLAG_ISSET(FLAG_ZERO)) // z
				|| (instr == 0xD4 && !FLAG_ISSET(FLAG_CARRY)) // nc
				|| (instr == 0xDC && FLAG_ISSET(FLAG_CARRY)) // c
			) {
				ticks+=12;
				_call(addr);
			}
			break;
		}
		case 0xE0: { // ldh [n], a
			uint8_t n = cpu_read_u8_at_pc();
			ticks+=12;
			gb_memory_write(0xff00|n,gb_cpu_registers.a);
			break;
		}
		case 0xE2: { // ldh [C], a
			ticks+=8;
			gb_memory_write(0xff00|gb_cpu_registers.c,gb_cpu_registers.a);
			break;
		}
		case 0xE9: { // jp hl
			ticks+=4;
			gb_cpu_registers.pc=gb_cpu_registers.hl;
			break;
		}
		case 0xEA: { // ld [nn], a
			uint16_t addr = cpu_read_u16_at_pc();
			ticks+=16;
			gb_memory_write(addr,gb_cpu_registers.a);
			break;
		}
		case 0xEE: { // xor a, n
			_xor(cpu_read_u8_at_pc());
			ticks+=8;
			break;
		}
		case 0xF0: { // ldh a, [n]
			uint8_t n = cpu_read_u8_at_pc();
			ticks+=12;
			gb_cpu_registers.a = gb_memory_read(0xff00|n);
			break;
		}
		case 0xC1:   // pop bc
		case 0xD1:   // pop de
		case 0xE1:   // pop hl
		case 0xF1: { // pop af
			uint8_t reg = (instr>>4)&0b11;
			set_reg16(reg,1,_pop());
			ticks+=12;
			break;
		}
		case 0xF2: { // ldh a, [C]
			ticks+=8;
			gb_cpu_registers.a = gb_memory_read(0xff00|gb_cpu_registers.c);
			break;
		}
		case 0xF3: { // di
			gb_cpu_interrupt.master = 0;
			ticks+=4;
			return;
		}
		case 0xC5:   // push bc
		case 0xD5:   // push de
		case 0xE5:   // push hl
		case 0xF5: { // push af
			uint8_t reg = (instr>>4)&0b11;
			_push(get_reg16(reg,1));
			ticks+=16;
			break;
		}
		case 0xFA: { // ld a, [nn]
			uint16_t addr = cpu_read_u16_at_pc();
			ticks+=16;
			gb_cpu_registers.a = gb_memory_read(addr);
			break;
		}
		case 0xFB: { // ei
			gb_cpu_interrupt.master_scheduled = 1; // enable interrupts after next instr
			ticks+=4;
		}
		case 0xFE: { // cp a, n
			sub(gb_cpu_registers.a,cpu_read_u8_at_pc());
			ticks+=8;
			break;
		}
		case 0xC7:   // rst 00
		case 0xCF:   // rst 08
		case 0xD7:   // rst 10
		case 0xDF:   // rst 18
		case 0xE7:   // rst 20
		case 0xEF:   // rst 28
		case 0xF7:   // rst 30
		case 0xFF: { // rst 38
			uint16_t new_address = ((instr>>3)&0b111)*8;
			_call(new_address);
			ticks+=16;
			break;
		}
		default: {
			debug_gb_value("Unimplemented instruction",instr);
			debug_gb_address("PC",gb_cpu_registers.pc-1);
			tic80_exit();
			gb_exit=1;
			return;
		}
	}
	if (!(ticks>old_ticks)) {
		debug_gb_value("Instruction doesn't increment ticks",instr);
	}
}

void gb_cpu_trace_pc() {
	debug_gb_address("Current PC:",gb_cpu_registers.pc);
}
