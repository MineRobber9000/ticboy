# The cart itself

The TICBoy cart is built using [customasm](https://github.com/hlorenzi/customasm). This allows me the flexibility to do all sorts of fun things.

## Files

- `dmg_boot.bin`: A GameBoy boot ROM (specifically, [the SameBoy boot ROM](https://github.com/LIJI32/SameBoy/blob/master/BootROMs/dmg_boot.asm), available under Expat license).
- `geometrix.gbc`: [Geometrix](https://github.com/AntonioND/geometrix), a 3-in-a-row matching game by AntonioND, provided under GPL 3 or later.
- `Makefile`: A makefile that builds `ticboy.tic`.
- `prepare.py`: A Python script that prepares the WASM binary for inclusion in `ticboy.tic`.
- `tic80cart.asm`: customasm ruledefs for tic80 carts.
- `ticboy.asm`: The actual TICBoy cart itself.

During the build process, a `binary.asm` file is created, as well as `cart.wasm.0`, `cart.wasm.1`, et cetera. These files allow me to include the binary chunk in the cart without needing to boot up TIC-80 and import `cart.wasm` that way.
