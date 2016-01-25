#!/usr/bin/env python

import os
import time

from ompi_cffi import ffi, lib

DVM_URI = "file:/Users/mark/proj/openmpi/mysubmit/dvm_uri"

mywait = 0
myspawn = 0

@ffi.callback("void(int, void *)")
def f(task, cbdata):
    print "Task %d is started!" % task
    global myspawn
    myspawn -= 1

@ffi.callback("void(int, int, void *)")
def g(task, status, cbdata):
    print "Task %d is completed with status %d!" % (task, status)
    global mywait
    mywait -= 1

# Request to create a background asynchronous event loop
os.putenv("OMPI_MCA_ess_tool_async_progress", "enabled")
os.putenv("OMPI_MCA_orte_abort_on_non_zero_status", "false")

for i in range(1):
    argv_keepalive = [
        ffi.new("char[]", "RADICAL-Pilot"), # Will be stripped off by the library
        ffi.new("char[]", "--hnp"), ffi.new("char[]", DVM_URI),
        ffi.new("char[]", "--np"), ffi.new("char[]", "1"),
        ffi.new("char[]", "bash"), ffi.new("char[]", "-c"),
        ffi.new("char[]", "t=%d; echo $t; touch TOUCHME; sleep $t; exit 0" % i),
        ffi.NULL, # Required
    ]
    argv = ffi.new("char *[]", argv_keepalive)
    task = lib.submit_job(argv, f, g, ffi.NULL)
    mywait += 1
    myspawn += 1
    print "Task %d submitted!" % task

while myspawn > 0 or mywait > 0:
    time.sleep(0.1)

print("Done!")
