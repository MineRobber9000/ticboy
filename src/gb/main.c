#include "main.h"

void gb_init() {
	gb_memory_init();
	gb_cpu_init();
}

void gb_tick() {
	cls(3);
	do {
		gb_cpu_tick();
		gb_gpu_tick();
	} while (!(gb_exit || INTERRUPT_ISSET(INTERRUPT_VBLANK)));
}
