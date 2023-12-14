INC = -Iinc -Ilibs/kcp -Ilibs/libesp-now_linux
OPT = -g

all: src/keg

src/keg: src/keg.o src/encrypt.c libs/kcp/ikcp.o
	gcc -o $@ $(OPT) $(INC) $^ -lcrypto -lhiredis

%.o: %.c
	gcc -c -o $@ $(OPT) $(INC) $<

clean:
	rm -rf src/*.o src/keg
