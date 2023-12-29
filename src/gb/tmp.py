r8 = "b c d e h l [hl] a".split()

def name(opcode):
	x = (opcode>>3)&0b111
	y = opcode&0b111
	return f"ld {r8[x]+',':<5} {r8[y]}"

for i in range(0x40,0x7F):
	if i==0x76: continue
	print(f"\t\tcase 0x{i:02X}:   // {name(i)}")
print("\t\tcase 0x7F: { // "+name(0x7f))
