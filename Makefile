all: main ompi_cffi.so

clean:
	rm -rf libsubmit.so mysubmit.o main mysubmit ompi_cffi.so ompi_cffi.o ompi_cffi.c main.dSYM

mysubmit.o: mysubmit.c mysubmit.h
	gcc -fPIC -Wall -g -c mysubmit.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal

libsubmit.so: mysubmit.o
	gcc -Wall -g -shared -o /Users/mark/proj/openmpi/mysubmit/libsubmit.so mysubmit.o -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal

main: main.c libsubmit.so mysubmit.h
	gcc -Wall -g -o main main.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -L /Users/mark/proj/openmpi/mysubmit -lopen-rte -lopen-pal -lsubmit

ompi_cffi.so: build_ompi_cffi.py libsubmit.so
	python build_ompi_cffi.py
