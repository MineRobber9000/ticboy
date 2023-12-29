#include "tic80.h"

// ---------------------------
//      Pointers
// ---------------------------

VRAM* FRAMEBUFFER = (VRAM*)0;
uint8_t* TILES = (uint8_t*)0x04000;
uint8_t* SPRITES = (uint8_t*)0x06000;
uint8_t* MAP = (uint8_t*)0x08000;
uint8_t* GAMEPADS = (uint8_t*)0x0FF80;
uint8_t* MOUSE = (uint8_t*)0x0FF84;
uint8_t* KEYBOARD = (uint8_t*)0x0FF88;
uint8_t* SFX_STATE = (uint8_t*)0x0FF8C;
uint8_t* SOUND_REGISTERS = (uint8_t*)0x0FF9C;
uint8_t* WAVEFORMS = (uint8_t*)0x0FFE4;
uint8_t* SFX = (uint8_t*)0x100E4;
uint8_t* MUSIC_PATTERNS = (uint8_t*)0x11164;
uint8_t* MUSIC_TRACKS = (uint8_t*)0x13E64;
uint8_t* SOUND_STATE = (uint8_t*)0x13FFC;
uint8_t* STEREO_VOLUME = (uint8_t*)0x14000;
uint8_t* PERSISTENT_MEMORY = (uint8_t*)0x14004;
uint8_t* SPRITE_FLAGS = (uint8_t*)0x14404;
uint8_t* SYSTEM_FONT = (uint8_t*)0x14604;
uint8_t* WASM_FREE_RAM = (uint8_t*)0x18000; // 160kb

// ---------------------------
//      Constants
// ---------------------------

const uint32_t TILES_SIZE = 0x2000;
const uint32_t SPRITES_SIZE = 0x2000;
const uint32_t MAP_SIZE = 32640;
const uint32_t GAMEPADS_SIZE = 4;
const uint32_t MOUSE_SIZE = 4;
const uint32_t KEYBOARD_SIZE = 4;
const uint32_t SFX_STATE_SIZE = 16;
const uint32_t SOUND_REGISTERS_SIZE = 72;
const uint32_t WAVEFORMS_SIZE = 256;
const uint32_t SFX_SIZE = 4224;
const uint32_t MUSIC_PATTERNS_SIZE = 11520;
const uint32_t MUSIC_TRACKS_SIZE = 408;
const uint32_t SOUND_STATE_SIZE = 4;
const uint32_t STEREO_VOLUME_SIZE = 4;
const uint32_t PERSISTENT_MEMORY_SIZE = 1024;
const uint32_t SPRITE_FLAGS_SIZE = 512;
const uint32_t SYSTEM_FONT_SIZE = 2048;
const uint32_t WASM_FREE_RAM_SIZE = 163840; // 160kb
