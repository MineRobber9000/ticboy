#pragma once

#include "common.h"

WASM_IMPORT("srand")
void srand(unsigned int);

WASM_IMPORT("rand")
int rand(void);
