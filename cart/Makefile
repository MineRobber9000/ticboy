ticboy.tic: ticboy.asm ../build/cart.wasm
	python prepare.py
	customasm -o ticboy.tic ticboy.asm

../build/cart.wasm:
	make -C ..

.PHONY: clean
clean:
	rm -f binary.asm cart.wasm.*
