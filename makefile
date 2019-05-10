.PHONY: default run build_prod build_dev debug clean

default: run

run: ./rls/output
	./rls/output

build_prod: ./rls/output

build_dev: ./dbg/output

./rls/output: ./rls main.c
	gcc main.c -o ./rls/output -lpthread -lrt

debug: ./dbg/output
	gdb ./dbg/output

./dbg/output: ./dbg main.c
	gcc main.c -o ./dbg/output -Wall -Werror -g

./rls:
	mkdir ./rls

./dbg:
	mkdir ./dbg

clean:
	rm -Rf ./rls ./dbg lines_written_file.shf
