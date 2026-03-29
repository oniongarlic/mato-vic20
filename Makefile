
all: mato.prg

mato.prg: mato.c
	oscar64 -tm=vic20 -Os mato.c

run: mato.prg
	/opt/vice/bin/xvic mato.prg
