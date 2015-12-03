all: main 

clean:
	rm -rf libsubmit.so mysubmit.o main mysubmit

mysubmit.o: mysubmit.c
	gcc -fPIC -Wall -g -c mysubmit.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal
	#gcc -Wall -g -c mysubmit.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal

libsubmit.so: mysubmit.o
	gcc -Wall -g -shared -o libsubmit.so mysubmit.o -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal

main: main.c libsubmit.so
	#gcc -Wall -g -o main main.c mysubmit.o -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal
	gcc -Wall -g -o main main.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal -lsubmit -L.

mysubmit: mysubmit.c
	gcc -Wall -g -o mysubmit mysubmit.c -I ../src/ompi/orte/include -I ../src/ompi/build/opal/include -I ../src/ompi/opal/include -I ../src/ompi -I ../src/ompi/opal/mca/event/libevent2022/libevent -L ../installed/DEBUG/lib -lopen-rte -lopen-pal
