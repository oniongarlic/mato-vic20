all: mato.prg mato3.prg

mato.prg: mato.c
	oscar64 -tm=vic20 -Os mato.c

mato3.prg: mato.c
	oscar64 -tm=vic20+3 -Os -o=mato3.prg mato.c

run: mato.prg
	/opt/vice/bin/xvic mato.prg

run3: mato3.prg
	/opt/vice/bin/xvic mato3.prg
