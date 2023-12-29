with open("../build/cart.wasm","rb") as f:
	binary = f.read()

chunks = []
for i in range(0,len(binary),65536):
	chunks.append(binary[i:i+65536])

format = """chunk binary {bank} Binary{bank}.size
Binary{bank}:
#d incbin("cart.wasm.{bank}")
.size = $ - Binary{bank}

"""

binaryasm = ""

for i, chunk in enumerate(chunks):
	with open(f"cart.wasm.{i}","wb") as f:
		f.write(chunk)
	binaryasm+=format.format(bank=i)

with open("binary.asm","w") as f:
	f.write(binaryasm)
