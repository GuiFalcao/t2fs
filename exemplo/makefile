all: teste1 exemplo

teste1: ../lib/libt2fs.a ./teste1.c
	gcc -o teste1 teste1.c -L../lib -lt2fs -Wall -m32 -g

exemplo: ../lib/libt2fs.a ./t2fstst.c
	gcc -o exemplo t2fstst.c -L../lib -lt2fs -Wall -m32 -g
