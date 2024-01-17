INC = -I. -Iinc -Ilibs/libesp-now_linux
OPT = -g -Werror

all: src/keg

src/keg: proto/mqtt-pub.pb-c.o src/keg.o src/encrypt.o src/transport.o src/mqtt.o
	gcc -o $@ $(OPT) $(INC) $^ -lpthread -lcrypto -lhiredis -lesp-now_linux -lprotobuf-c -lmosquitto

proto/%.pb-c.c: proto/%.proto
	protoc --c_out=. $<

%.o: %.c
	gcc -c -o $@ $(OPT) $(INC) $<

clean:
	rm -rf proto/*.pb-c.* src/*.o src/keg

.PHONY: clean