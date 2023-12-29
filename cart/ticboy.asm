#include "tic80cart.asm"

chunk code 0 Metadata.size
Metadata:
metatag "name" "TICBoy"
metatag "desc" "A GameBoy emulator on TIC-80"
metatag "author" "khuxkm/MineRobber9000"
metatag "script" "wasm"
metatag "menu" "trace_pc"
#d "\n"
.size = $ - Metadata

chunk palette 0 Palette.size
Palette:
color 255 255 255
color 170 170 170
color 85  85  85
color 0   0   0
.size = $ - Palette

chunk map 0 GameboyROM.size
GameboyROM: ; the ROM we're loading
#d incbin("geometrix.gbc")
.size = $ - GameboyROM

chunk tiles 0 BootROM.size
BootROM: ; the GameBoy boot ROM
#d incbin("dmg_boot.bin")
.size = $ - BootROM

#include "binary.asm"
