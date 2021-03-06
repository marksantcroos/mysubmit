#!/usr/bin/env python

import os
import time

from orte_cffi import ffi, lib

DVM_URI = "file:/Users/mark/proj/openmpi/mysubmit/dvm_uri"

fourislaunched = False

@ffi.def_extern()
def launch_cb(task, jdata, status, cbdata):
    print "Task %d is started with %d!" % (task, status)
    instance = task_instance_map[task]
    instance.myspawn -= 1
    print "Map length: %d" % len(task_instance_map)
    global fourislaunched
    fourislaunched = True

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

        index = ffi.new("int[1]")

        for i in range(3):

            argv_keepalive = [
                ffi.new("char[]", "RADICAL-Pilot"),
                ffi.new("char[]", "--np"), ffi.new("char[]", "1"),
                ffi.new("char[]", "bash"), ffi.new("char[]", "-c"),
                #ffi.new("char[]", "t=%d; echo $t; touch TOUCHME; sleep $t; exit 0" % 10),
                ffi.new("char[]", "sleep %d" % 10),
                ffi.NULL, # Required
            ]
            argv = ffi.new("char *[]", argv_keepalive)
            rc = lib.orte_submit_job(argv, index, lib.launch_cb, ffi.NULL, lib.finish_cb, ffi.NULL)
            task = index[0]
            task_instance_map[task] = self
            self.mywait += 1
            self.myspawn += 1
            print "Task %d submitted!" % task


        while self.myspawn > 0 or self.mywait > 0:

            global fourislaunched
            if fourislaunched:
                print "Cancelling task ..."
                lib.orte_submit_cancel(1)
                fourislaunched = False

            time.sleep(0.1)

        print("Done!")

        lib.orte_submit_halt()

rp = RP()
rp.run()
