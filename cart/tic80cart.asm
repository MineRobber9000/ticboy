; TIC-80 cart ruledef for customasm

#subruledef tic80_chunk
{
	tiles => 1`5
	sprites => 2`5
	map => 4`5
	code => 5`5
	flags => 6`5
	sfx => 9`5
	waveform => 10`5
	palette => 12`5
	music => 14`5
	patterns => 15`5
	default => 17`5
	screen => 18`5
	binary => 19`5
	; deprecated chunk types, uncomment to add
	; cover_dep => 3`5
	; patterns_dep => 13`5
	; code_zip => 16`5
}

#ruledef tic80
{
	chunk {type: tic80_chunk} {bank: u8} {size: u24} => {
		assert(bank<8,"Bank number must be 0-7!")
		assert(size<=65535||((type==5||type==19)&&size<=65536),"Chunk too big!")
		bank`3 @ type @ le(size`16) @ 0`8
	}

	metatag {comment} {name} {value} => {
		comment @ " " @ name @ ": " @ value @ "\n"
	}

	metatag {name} {value} => asm {
		metatag "--" {name} {value}
	}

	color {rgb: u24} => rgb

	color {r: u8} {g: u8} {b: u8} => r @ g @ b
}
