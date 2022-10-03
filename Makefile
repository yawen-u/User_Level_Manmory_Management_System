
all : test playground

test: ../my_vm.h
	gcc test.c -L../ -lmy_vm -m32 -o test
	gcc multi_test.c -L../ -lmy_vm -m32 -o mtest -lpthread

playground: ../my_vm.h
	gcc playground.c -L../ -lmy_vm -m32 -o playground

clean:
	rm -rf test mtest playground
