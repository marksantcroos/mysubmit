#!/usr/bin/env python

from ompi_cffi import ffi, lib

mywait = 0
myspawn = 0

@ffi.callback("void(int)")
def f(task):
    print "Task %d is started!" % task
    global myspawn
    myspawn -= 1

@ffi.callback("void(int, int)")
def g(task, status):
    print "Task %d is completed with status %d!" % (task, status)
    global mywait
    mywait -= 1

for i in range(10):
    argv_keepalive = [
        ffi.new("char[]", "orte-submit"),
        ffi.new("char[]", "--hnp"),
        ffi.new("char[]", "file:/Users/mark/proj/openmpi/mysubmit/dvm_uri"),
        ffi.new("char[]", "--np"),
        ffi.new("char[]", "1"),
        ffi.new("char[]", "bash"),
        ffi.new("char[]", "-c"),
        ffi.new("char[]", "t=%d; echo $t; sleep $t" % i),
        ffi.NULL,
    ]
    tmpargv = ffi.new("char *[]", argv_keepalive)
    task = lib.submit_job(tmpargv, f, g)
    mywait += 1
    myspawn += 1
    print "Task %d submitted!" % task

while myspawn > 0 or mywait > 0:
    lib.opal_event_loop(lib.orte_event_base, lib.OPAL_EVLOOP_ONCE)

print("Done!")
