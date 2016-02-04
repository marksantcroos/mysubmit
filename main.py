#!/usr/bin/env python

import os
import time

from orte_cffi import ffi, lib

DVM_URI = "file:/Users/mark/proj/openmpi/mysubmit/dvm_uri"

@ffi.def_extern()
def launch_cb(task, jdata, status, cbdata):
    print "Task %d is started!" % task
    instance = task_instance_map[task]
    instance.myspawn -= 1
    print "Map length: %d" % len(task_instance_map)

@ffi.def_extern()
def finish_cb(task, jdata, status, cbdata):
    print "Task %d is completed with status %d!" % (task, status)
    instance = task_instance_map[task]
    instance.mywait -= 1
    del task_instance_map[task]
    print "Map length: %d" % len(task_instance_map)

# Dictionary to find class instance from task id
task_instance_map = {}

# Request to create a background asynchronous event loop
os.putenv("OMPI_MCA_ess_tool_async_progress", "enabled")
os.putenv("OMPI_MCA_orte_abort_on_non_zero_status", "false")

class RP():

    mywait = 0
    myspawn = 0

    def run(self):

        argv_keepalive = [
            ffi.new("char[]", "RADICAL-Pilot"), # Will be stripped off by the library
            ffi.new("char[]", "--hnp"), ffi.new("char[]", DVM_URI),
            ffi.NULL, # Required
        ]
        argv = ffi.new("char *[]", argv_keepalive)
        lib.orte_submit_init(3, argv, ffi.NULL)

        for i in range(9):

            argv_keepalive = [
                ffi.new("char[]", "RADICAL-Pilot"),
                ffi.new("char[]", "--np"), ffi.new("char[]", "1"),
                ffi.new("char[]", "bash"), ffi.new("char[]", "-c"),
                ffi.new("char[]", "t=%d; echo $t; touch TOUCHME; sleep $t; exit 0" % i),
                ffi.NULL, # Required
            ]
            argv = ffi.new("char *[]", argv_keepalive)
            task = lib.orte_submit_job(argv, lib.launch_cb, ffi.NULL, lib.finish_cb, ffi.NULL)
            task_instance_map[task] = self
            self.mywait += 1
            self.myspawn += 1
            print "Task %d submitted!" % task

        while self.myspawn > 0 or self.mywait > 0:
            time.sleep(0.1)

        print("Done!")

rp = RP()
rp.run()
