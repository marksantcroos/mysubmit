# Introduction

Hacked up version of orte-submit with the goal of reusing the socket for many submissions.

# How to compile
``` bash
gcc -g -o mysubmit mysubmit.c \
  -I ../src/ompi/orte/include \
  -I ../src/ompi/build/opal/include \
  -I ../src/ompi/opal/include \
  -I ../src/ompi \
  -I ../src/ompi/opal/mca/event/libevent2022/libevent \
  -L ../installed/DEBUG/lib -lopen-rte -lopen-pal
```

# How to run

``` bash
./mysubmit --hnp file:/Users/mark/proj/openmpi/mysubmit/dvm_uri -np 1 /bin/bash -c 't=$[($RANDOM % 10)]; echo $t; sleep $t'
```
