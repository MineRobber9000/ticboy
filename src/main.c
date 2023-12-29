#include "tic80.h"
#include "gb/main.h"

WASM_EXPORT("BOOT")
void BOOT() {
	FRAMEBUFFER->BORDER_COLOR = 3;
	FRAMEBUFFER->SCREEN_OFFSET_X = 40;
	gb_init();
	return;
}

WASM_EXPORT("TIC")
void TIC() {
	gb_tick();
	return;
}

WASM_EXPORT("MENU")
void MENU(uint8_t index) {
	gb_cpu_trace_pc();
}
