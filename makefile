BWT= bwt/globals.c bwt/helped.c bwt/ds.c bwt/shallow.c bwt/deep2.c bwt/blind2.c

all: nsa unsa makefile

test: all
	cp 1k 1k.cp && time -p ./nsa 1k.cp && time -p ./unsa 1k.cp.nsa && diff -qs 1k 1k.cp && ls -l 1k 1k.cp.nsa

acdbg: acdemo
	gdb -q -ex "run" ./acdemo

acrun: acdemo
	./acdemo

acdemo: ac/ac.c ac/acdemo.c ac/ac.h
	gcc -g -o acdemo ac/acdemo.c ac/ac.c

dbg: nsa unsa makefile
	#gdb nsa -q -ex "run 1k.cp"
	gdb unsa -q -ex "run 1k.cp.nsa"

nsa: nsa.c makefile $(BWT) dc2/dc2.c hash/hash.c ac/ac.c dc2/dc2.h ac/ac.h
	gcc -O2 -o nsa -DUNIX nsa.c -lm $(BWT) dc2/dc2.c hash/hash.c ac/ac.c

unsa: unsa.c makefile $(BWT) dc2/undc2.c hash/hash.c ac/ac.c dc2/dc2.h ac/ac.h
	gcc -O2 -o unsa -DUNIX unsa.c -lm $(BWT) dc2/undc2.c hash/hash.c ac/ac.c

clean:
	rm -f 1k.cp 1k.cp.nsa nsa unsa tags
