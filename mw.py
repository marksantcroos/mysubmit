#!/usr/bin/env python


import os
import time
import argparse

from orte_cffi import ffi, lib

from radical.pilot.utils.prof_utils import Profiler, timestamp as util_timestamp
from radical.pilot import Session
from radical.pilot.states import *
from radical.pilot.utils import inject_metadata
import radical.utils as ru

DVM_URI = "file:../dvm_uri"
GTOD = "$HOME/gtod"

CORES=4096
TASKS=4096
SLEEP=64

@ffi.def_extern()
def launch_cb(index, jdata, status, cbdata):

    handle = ffi.from_handle(cbdata)
    instance = handle['instance']
    task = handle['task']

    instance.session.prof.prof('passed', msg="ExecWatcher picked up unit", uid=task, name='AgentExecutingComponent')

    instance.session.prof.prof('advance', uid=task, state=EXECUTING, name='AgentExecutingComponent')

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

    instance.session.prof.prof('unschedule', msg='released', uid=task, name='AgentSchedulingComponent')

    instance.session.prof.prof('exec', msg='execution complete', uid=task, name='AgentExecutingComponent')

    if not status:
        instance.session.prof.prof('final', msg='execution succeeded', uid=task, name='AgentExecutingComponent')
    else:
        instance.session.prof.prof('final', msg='execution failed', uid=task, name='AgentExecutingComponent')

    instance.session.prof.prof('advance', uid=task, state=AGENT_STAGING_OUTPUT_PENDING, name='AgentExecutingComponent')
    instance.session.prof.prof(event='put', state=AGENT_STAGING_OUTPUT_PENDING, uid=task, name='AgentExecutingComponent')

class RP():

    # Dictionary to find class instance from task id
    task_instance_map = {}
    active = 0

    def __init__(self, session):
        self.session = session
        os.mkdir(session.uid)
        os.chdir(session.uid)

    def run(self, cores, tasks, runtime):

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
        while task_no <= tasks or self.active > 0:

            if task_no <= tasks and self.active < cores:

                task_id = 'unit.%.6d' % task_no
                cu_tmpdir = '%s' % task_id

                #
                # ASIC
                #
                self.session.prof.prof(event='get', state=AGENT_STAGING_INPUT_PENDING, uid=task_id, name='AgentStagingInputComponent')
                self.session.prof.prof(event='work start', state=AGENT_STAGING_INPUT_PENDING, uid=task_id, name='AgentStagingInputComponent')
                self.session.prof.prof('advance', uid=task_id, state=AGENT_STAGING_INPUT, name='AgentStagingInputComponent')
                os.mkdir('%s' % cu_tmpdir)
                self.session.prof.prof('advance', uid=task_id, state=ALLOCATING_PENDING, name='AgentStagingInputComponent')
                self.session.prof.prof(event='work done', state=AGENT_STAGING_INPUT_PENDING, uid=task_id, name='AgentStagingInputComponent')
                self.session.prof.prof(event='put', state=ALLOCATING_PENDING, uid=task_id, name='AgentStagingInputComponent')

                #
                # ASC
                #
                self.session.prof.prof(event='get', state=ALLOCATING_PENDING, uid=task_id, name='AgentSchedulingComponent')
                self.session.prof.prof(event='work start', state=ALLOCATING_PENDING, uid=task_id, name='AgentSchedulingComponent')
                self.session.prof.prof('advance', uid=task_id, state=ALLOCATING, name='AgentSchedulingComponent')
                self.session.prof.prof('schedule', msg='try', uid=task_id, name='AgentSchedulingComponent')
                self.session.prof.prof('schedule', msg='allocated', uid=task_id, name='AgentSchedulingComponent')
                self.session.prof.prof('advance', uid=task_id, state=EXECUTING_PENDING, name='AgentSchedulingComponent')
                self.session.prof.prof(event='put', state=EXECUTING_PENDING, uid=task_id, name='AgentSchedulingComponent')
                self.session.prof.prof(event='work done', state=ALLOCATING_PENDING, uid=task_id, name='AgentSchedulingComponent')

                #
                # AEC
                #

                self.session.prof.prof(event='get', state=EXECUTING_PENDING, uid=task_id, name='AgentExecutingComponent')
                self.session.prof.prof(event='work start', state=EXECUTING_PENDING, uid=task_id, name='AgentExecutingComponent')
                self.session.prof.prof('exec', msg='unit launch', uid=task_id, name='AgentExecutingComponent')
                self.session.prof.prof('spawn', msg='unit spawn', uid=task_id, name='AgentExecutingComponent')

                argv_keepalive = [
                    ffi.new("char[]", "RADICAL-Pilot"),
                    ffi.new("char[]", "--np"), ffi.new("char[]", "1"),
                ]

                # Let the orted write stdout and stderr to rank-based output files
                argv_keepalive.append(ffi.new("char[]", "--output-filename"))
                argv_keepalive.append(ffi.new("char[]", "%s:nojobid,nocopy" % str(cu_tmpdir)))

                argv_keepalive.append(ffi.new("char[]", "sh"))
                argv_keepalive.append(ffi.new("char[]", "-c"))

                task_command = 'sleep %d' % runtime

                # Wrap in (sub)shell for output redirection
                task_command = "echo script start_script `%s` >> %s/PROF; " % (GTOD, cu_tmpdir) + \
                      task_command + \
                      "; echo script after_exec `%s` >> %s/PROF" % (GTOD, cu_tmpdir)
                argv_keepalive.append(ffi.new("char[]", str("%s; exit $RETVAL" % str(task_command))))

                argv_keepalive.append(ffi.NULL) # NULL Termination Required
                argv = ffi.new("char *[]", argv_keepalive)

                self.session.prof.prof('command', msg='launch command constructed', uid=task_id, name='AgentExecutingComponent')

                struct = {'instance': self, 'task': task_id}
                cbdata = ffi.new_handle(struct)

                lib.orte_submit_job(argv, index_ptr, lib.launch_cb, cbdata, lib.finish_cb, cbdata)

                index = index_ptr[0] # pointer notation
                self.task_instance_map[index] = cbdata

                self.session.prof.prof('spawn', msg='spawning passed to orte', uid=task_id, name='AgentExecutingComponent')
                self.session.prof.prof(event='work done', state=EXECUTING_PENDING, uid=task_id, name='AgentExecutingComponent')

                print "Task %s submitted!" % task_id

                self.active += 1
                task_no += 1

            else:
                time.sleep(0.001)

        print("Execution done.")
        print()
        print("Collecting profiles ...")
        for task_no in range(tasks):
            task_id = 'unit.%.6d' % task_no
            self.session.prof.prof('advance', uid=task_id, state=AGENT_STAGING_OUTPUT, name='AgentStagingOutputComponent')
            cu_tmpdir = '%s' % task_id
            if os.path.isfile("%s/PROF" % cu_tmpdir):
                try:
                    with open("%s/PROF" % cu_tmpdir, 'r') as prof_f:
                        txt = prof_f.read()
                        for line in txt.split("\n"):
                            if line:
                                x1, x2, x3 = line.split()
                                self.session.prof.prof(x1, msg=x2, timestamp=float(x3), uid=task_id, name='AgentStagingOutputComponent')
                except Exception as e:
                    print("Pre/Post profiling file read failed: `%s`" % e)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Run a ORTE MW experiment')
    parser.add_argument('--cores', type=int, help='the number of cores to run on', required=True)
    parser.add_argument('--tasks', type=int, help='the number of tasks to run', required=True)
    parser.add_argument('--runtime', type=int, help='the individual task runtime', required=True)

    args = parser.parse_args()

    # TODO: enable profiling

    report = ru.LogReporter(name='mw')

    # Request to create a background asynchronous event loop
    os.environ["OMPI_MCA_ess_tool_async_progress"] = "enabled"

    # Make sure we enable profiling
    os.environ["RADICAL_PILOT_PROFILE"] = "enabled"

    metadata = {
        'backend': 'ORTE',
        'pilot_cores': args.cores,
        'pilot_runtime': 0,
        'cu_runtime': args.runtime,
        'cu_cores': 1,
        'cu_count': args.tasks,
        'generations': args.tasks/args.cores,
        'barriers': [],
        'profiling': True,
        'label': 'mw',
        'repetitions': 1,
        'iteration': 1,
        'exclusive_agent_nodes': False,
        'num_sub_agents': 0,
        'num_exec_instances_per_sub_agent': 0,
        'effective_cores': args.cores,
    }

    sid = ru.generate_id('mw.session', mode=ru.ID_PRIVATE)
    session = Session(name=sid)
    report.info("Inserting meta data into session ...\n")
    inject_metadata(session, metadata)

    rp = RP(session=session)
    rp.run(cores=args.cores, tasks=args.tasks, runtime=args.runtime)

    session.close(cleanup=False, terminate=True)

    print "%s complete: %d cores, %d tasks, %d runtime" % (sid, args.cores, args.tasks, args.runtime)
