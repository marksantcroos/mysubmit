#!/usr/bin/env python

import os
import sys
import time

from orte_cffi import ffi, lib

from radical.pilot.utils.prof_utils import Profiler, timestamp as util_timestamp
from radical.pilot import Session
from radical.pilot.states import EXECUTING, AGENT_STAGING_OUTPUT_PENDING
from radical.pilot.utils import inject_metadata
import radical.utils as ru

DVM_URI = "file:dvm_uri"

CORES=8
TASKS=32
SLEEP=2

@ffi.def_extern()
def launch_cb(index, jdata, status, cbdata):

    handle = ffi.from_handle(cbdata)
    instance = handle['instance']
    task = handle['task']

    instance.session.prof.prof('advance', uid=task, state=EXECUTING, timestamp=util_timestamp())

    print "Task %s with index %d is started with status %d!" % (task, index, status)

    print "Map length: %d" % len(instance.task_instance_map)


@ffi.def_extern()
def finish_cb(index, jdata, status, cbdata):

    handle = ffi.from_handle(cbdata)
    instance = handle['instance']
    task = handle['task']

    instance.active -= 1
    print "Task %s with index %d is completed with status %d!" % (task, index, status)

    del instance.task_instance_map[index]
    print "Map length: %d" % len(instance.task_instance_map)

    instance.session.prof.prof('advance', uid=task, state=AGENT_STAGING_OUTPUT_PENDING, timestamp=util_timestamp())


class RP():

    # Dictionary to find class instance from task id
    task_instance_map = {}
    active = 0

    def __init__(self, session):
        self.session = session


    def run(self, ):

        argv_keepalive = [
            ffi.new("char[]", "RADICAL-Pilot"), # Will be stripped off by the library
            ffi.new("char[]", "--hnp"),
            ffi.new("char[]", DVM_URI),
            ffi.NULL, # Required
        ]
        argv = ffi.new("char *[]", argv_keepalive)
        lib.orte_submit_init(3, argv, ffi.NULL)

        # Used for storing the task id that is returned by orte_submit_job
        index_ptr = ffi.new("int[1]")

        task_no = 1
        while task_no <= TASKS or self.active > 0:

            if task_no <= TASKS and self.active < CORES:

                task_id = 'unit.%d' % task_no

                argv_keepalive = [
                    ffi.new("char[]", "RADICAL-Pilot"),
                    ffi.new("char[]", "--np"), ffi.new("char[]", "1"),
                    # ffi.new("char[]", "true"),
                    ffi.new("char[]", "bash"), ffi.new("char[]", "-c"),
                    ffi.new("char[]", "sleep %d" % SLEEP),
                    ffi.NULL, # NULL Termination Required
                ]
                argv = ffi.new("char *[]", argv_keepalive)

                struct = {'instance': self, 'task': task_id}
                cbdata = ffi.new_handle(struct)

                lib.orte_submit_job(argv, index_ptr, lib.launch_cb, cbdata, lib.finish_cb, cbdata)

                self.active += 1

                index = index_ptr[0] # pointer notation
                self.task_instance_map[index] = cbdata

                print "Task %s submitted!" % task_id

                task_no += 1

            else:
                time.sleep(0.001)

        print("Done!")
        #self.session.prof.close()


if __name__ == '__main__':

    # TODO: enable profiling

    report = ru.LogReporter(name='mw')

    # Request to create a background asynchronous event loop
    os.environ["OMPI_MCA_ess_tool_async_progress"] = "enabled"

    # Make sure we enable profiling
    os.environ["RADICAL_PILOT_PROFILE"] = "enabled"

    metadata = {
        'backend': 'ORTE',
        'pilot_cores': CORES,
        'pilot_runtime': 0,
        'cu_runtime': SLEEP,
        'cu_cores': 1,
        'cu_count': TASKS,
        'generations': TASKS/CORES,
        'barriers': [],
        'profiling': True,
        'label': 'mw',
        'repetitions': 1,
        'iteration': 1,
        'exclusive_agent_nodes': False,
        'num_sub_agents': 0,
        'num_exec_instances_per_sub_agent': 0,
        'effective_cores': CORES,
    }

    # if len(sys.argv) == 1:
    #     client = 'mw'
    # else:
    #     client = sys.argv[1]

    sid = ru.generate_id('mw.session', mode=ru.ID_PRIVATE)
    session = Session(name=sid)
    report.info("Inserting meta data into session ...\n")
    inject_metadata(session, metadata)

    rp = RP(session=session)
    rp.run()

    session.close(cleanup=False, terminate=True)
