/* -*- C -*-
 *
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007-2009 Sun Microsystems, Inc. All rights reserved.
 * Copyright (c) 2007-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2013-2015 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif  /* HAVE_STRINGS_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif  /* HAVE_SYS_WAIT_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif  /* HAVE_SYS_TIME_H */
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "opal/dss/dss.h"
#include "opal/mca/event/event.h"
#include "opal/mca/installdirs/installdirs.h"
#include "opal/mca/hwloc/base/base.h"
#include "opal/mca/base/base.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "opal/util/basename.h"
#include "opal/util/cmd_line.h"
#include "opal/util/opal_environ.h"
#include "opal/util/opal_getcwd.h"
#include "opal/util/show_help.h"
#include "opal/util/fd.h"
#include "opal/sys/atomic.h"
#if OPAL_ENABLE_FT_CR == 1
#include "opal/runtime/opal_cr.h"
#endif

#include "opal/version.h"
#include "opal/runtime/opal.h"
#include "opal/runtime/opal_info_support.h"
#include "opal/util/os_path.h"
#include "opal/util/path.h"
#include "opal/class/opal_pointer_array.h"
#include "opal/dss/dss.h"

#include "orte/mca/odls/odls_types.h"
#include "orte/mca/plm/plm.h"
#include "orte/mca/rmaps/rmaps_types.h"
#include "orte/mca/rmaps/base/base.h"

#include "orte/mca/schizo/schizo.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/rml/base/rml_contact.h"
#include "orte/mca/routed/routed.h"

#include "orte/runtime/runtime.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"
#include "orte/runtime/orte_quit.h"
#include "orte/util/show_help.h"

/*
 * Globals
 */
static char **global_mca_env = NULL;
static orte_std_cntr_t total_num_apps = 0;
static bool want_prefix_by_default = (bool) ORTE_WANT_ORTERUN_PREFIX_BY_DEFAULT;
extern int mywait;
extern int myspawn;
opal_pointer_array_t tool_jobs;

/*
 * Globals
 */
static struct {
    bool help;
    bool version;
    char *report_pid;
    char *stdin_target;
    bool index_argv;
    bool preload_binaries;
    char *preload_files;
    char *appfile;
    int num_procs;
    char *hnp;
    char *wdir;
    bool set_cwd_to_session_dir;
    char *path;
    bool enable_recovery;
    char *personality;
    char *prefix;
    bool terminate;
    bool nolocal;
    bool no_oversubscribe;
    bool oversubscribe;
    int cpus_per_proc;
    bool pernode;
    int npernode;
    bool use_hwthreads_as_cpus;
    int npersocket;
    char *mapping_policy;
    char *ranking_policy;
    char *binding_policy;
    bool report_bindings;
    char *slot_list;
    bool debug;
    bool run_as_root;
} myglobals;

static opal_cmd_line_init_t cmd_line_init[] = {
    /* Various "obvious" options */
    { NULL, 'h', NULL, "help", 0,
      &myglobals.help, OPAL_CMD_LINE_TYPE_BOOL,
      "This help message" },
    { NULL, 'V', NULL, "version", 0,
      &myglobals.version, OPAL_CMD_LINE_TYPE_BOOL,
      "Print version and exit" },

    { NULL, '\0', "report-pid", "report-pid", 1,
      &myglobals.report_pid, OPAL_CMD_LINE_TYPE_STRING,
      "Printout pid on stdout [-], stderr [+], or a file [anything else]" },

    /* select stdin option */
    { NULL, '\0', "stdin", "stdin", 1,
      &myglobals.stdin_target, OPAL_CMD_LINE_TYPE_STRING,
      "Specify procs to receive stdin [rank, all, none] (default: 0, indicating rank 0)" },

    /* request that argv[0] be indexed */
    { NULL, '\0', "index-argv-by-rank", "index-argv-by-rank", 0,
      &myglobals.index_argv, OPAL_CMD_LINE_TYPE_BOOL,
      "Uniquely index argv[0] for each process using its rank" },

    /* Preload the binary on the remote machine */
    { NULL, 's', NULL, "preload-binary", 0,
      &myglobals.preload_binaries, OPAL_CMD_LINE_TYPE_BOOL,
      "Preload the binary on the remote machine before starting the remote process." },

    /* Preload files on the remote machine */
    { NULL, '\0', NULL, "preload-files", 1,
      &myglobals.preload_files, OPAL_CMD_LINE_TYPE_STRING,
      "Preload the comma separated list of files to the remote machines current working directory before starting the remote process." },

    /* Use an appfile */
    { NULL, '\0', NULL, "app", 1,
      &myglobals.appfile, OPAL_CMD_LINE_TYPE_STRING,
      "Provide an appfile; ignore all other command line options" },

    /* Number of processes; -c, -n, --n, -np, and --np are all
       synonyms */
    { NULL, 'c', "np", "np", 1,
      &myglobals.num_procs, OPAL_CMD_LINE_TYPE_INT,
      "Number of processes to run" },
    { NULL, '\0', "n", "n", 1,
      &myglobals.num_procs, OPAL_CMD_LINE_TYPE_INT,
      "Number of processes to run" },

    /* uri of the dvm, or at least where to get it */
    { NULL, '\0', "hnp", "hnp", 1,
      &myglobals.hnp, OPAL_CMD_LINE_TYPE_STRING,
      "Specify the URI of the Open MPI server, or the name of the file (specified as file:filename) that contains that info" },

    /* tell the dvm to terminate */
    { NULL, '\0', "terminate", "terminate", 0,
      &myglobals.terminate, OPAL_CMD_LINE_TYPE_BOOL,
      "Terminate the DVM" },


    /* Export environment variables; potentially used multiple times,
       so it does not make sense to set into a variable */
    { NULL, 'x', NULL, NULL, 1,
      NULL, OPAL_CMD_LINE_TYPE_NULL,
      "Export an environment variable, optionally specifying a value (e.g., \"-x foo\" exports the environment variable foo and takes its value from the current environment; \"-x foo=bar\" exports the environment variable name foo and sets its value to \"bar\" in the started processes)" },

      /* Mapping controls */
    { NULL, 'H', "host", "host", 1,
      NULL, OPAL_CMD_LINE_TYPE_STRING,
      "List of hosts to invoke processes on" },
    { NULL, '\0', "nolocal", "nolocal", 0,
      &myglobals.nolocal, OPAL_CMD_LINE_TYPE_BOOL,
      "Do not run any MPI applications on the local node" },
    { NULL, '\0', "nooversubscribe", "nooversubscribe", 0,
      &myglobals.no_oversubscribe, OPAL_CMD_LINE_TYPE_BOOL,
      "Nodes are not to be oversubscribed, even if the system supports such operation"},
    { NULL, '\0', "oversubscribe", "oversubscribe", 0,
      &myglobals.oversubscribe, OPAL_CMD_LINE_TYPE_BOOL,
      "Nodes are allowed to be oversubscribed, even on a managed system, and overloading of processing elements"},
    { NULL, '\0', "cpus-per-proc", "cpus-per-proc", 1,
      &myglobals.cpus_per_proc, OPAL_CMD_LINE_TYPE_INT,
      "Number of cpus to use for each process [default=1]" },

    /* Nperxxx options that do not require topology and are always
     * available - included for backwards compatibility
     */
    { NULL, '\0', "pernode", "pernode", 0,
      &myglobals.pernode, OPAL_CMD_LINE_TYPE_BOOL,
      "Launch one process per available node" },
    { NULL, '\0', "npernode", "npernode", 1,
      &myglobals.npernode, OPAL_CMD_LINE_TYPE_INT,
      "Launch n processes per node on all allocated nodes" },
    { NULL, '\0', "N", NULL, 1,
      &myglobals.npernode, OPAL_CMD_LINE_TYPE_INT,
        "Launch n processes per node on all allocated nodes (synonym for npernode)" },

    /* declare hardware threads as independent cpus */
    { NULL, '\0', "use-hwthread-cpus", "use-hwthread-cpus", 0,
      &myglobals.use_hwthreads_as_cpus, OPAL_CMD_LINE_TYPE_BOOL,
      "Use hardware threads as independent cpus" },

    /* include npersocket for backwards compatibility */
    { NULL, '\0', "npersocket", "npersocket", 1,
      &myglobals.npersocket, OPAL_CMD_LINE_TYPE_INT,
      "Launch n processes per socket on all allocated nodes" },

    /* Mapping options */
    { NULL, '\0', NULL, "map-by", 1,
      &myglobals.mapping_policy, OPAL_CMD_LINE_TYPE_STRING,
      "Mapping Policy [slot | hwthread | core | socket (default) | numa | board | node]" },

      /* Ranking options */
    { NULL, '\0', NULL, "rank-by", 1,
      &myglobals.ranking_policy, OPAL_CMD_LINE_TYPE_STRING,
      "Ranking Policy [slot (default) | hwthread | core | socket | numa | board | node]" },

      /* Binding options */
    { NULL, '\0', NULL, "bind-to", 1,
      &myglobals.binding_policy, OPAL_CMD_LINE_TYPE_STRING,
      "Policy for binding processes. Allowed values: none, hwthread, core, l1cache, l2cache, l3cache, socket, numa, board (\"none\" is the default when oversubscribed, \"core\" is the default when np<=2, and \"socket\" is the default when np>2). Allowed qualifiers: overload-allowed, if-supported" },

    { NULL, '\0', "report-bindings", "report-bindings", 0,
      &myglobals.report_bindings, OPAL_CMD_LINE_TYPE_BOOL,
      "Whether to report process bindings to stderr" },

    /* slot list option */
    { NULL, '\0', "slot-list", "slot-list", 1,
      &myglobals.slot_list, OPAL_CMD_LINE_TYPE_STRING,
      "List of processor IDs to bind processes to [default=NULL]"},

    /* mpiexec-like arguments */
    { NULL, '\0', "wdir", "wdir", 1,
      &myglobals.wdir, OPAL_CMD_LINE_TYPE_STRING,
      "Set the working directory of the started processes" },
    { NULL, '\0', "wd", "wd", 1,
      &myglobals.wdir, OPAL_CMD_LINE_TYPE_STRING,
      "Synonym for --wdir" },
    { NULL, '\0', "set-cwd-to-session-dir", "set-cwd-to-session-dir", 0,
      &myglobals.set_cwd_to_session_dir, OPAL_CMD_LINE_TYPE_BOOL,
      "Set the working directory of the started processes to their session directory" },
    { NULL, '\0', "path", "path", 1,
      &myglobals.path, OPAL_CMD_LINE_TYPE_STRING,
      "PATH to be used to look for executables to start processes" },

    { NULL, '\0', "enable-recovery", "enable-recovery", 0,
      &myglobals.enable_recovery, OPAL_CMD_LINE_TYPE_BOOL,
      "Enable recovery (resets all recovery options to on)" },

    { NULL, '\0', "personality", "personality", 1,
      &myglobals.personality, OPAL_CMD_LINE_TYPE_STRING,
      "Programming model/language being used (default=\"ompi\")" },

    { NULL, 'd', "debug-devel", "debug-devel", 0,
      &myglobals.debug, OPAL_CMD_LINE_TYPE_BOOL,
      "Enable debugging of OpenRTE" },

    { NULL, '\0', "allow-run-as-root", "allow-run-as-root", 0,
      &myglobals.run_as_root, OPAL_CMD_LINE_TYPE_BOOL,
      "Allow execution as root (STRONGLY DISCOURAGED)" },

    /* End of list */
    { NULL, '\0', NULL, NULL, 0,
      NULL, OPAL_CMD_LINE_TYPE_NULL, NULL }
};

/*
 * Local functions
 */
static int create_app(int argc, char* argv[],
                      orte_job_t *jdata,
                      orte_app_context_t **app,
                      bool *made_app, char ***app_env);
static int init_globals(void);
static int parse_globals(int argc, char* argv[], opal_cmd_line_t *cmd_line);
static int parse_locals(orte_job_t *jdata, int argc, char* argv[]);
static void set_classpath_jar_file(orte_app_context_t *app, int index, char *jarfile);
static int parse_appfile(orte_job_t *jdata, char *filename, char ***env);
static void orte_timeout_wakeup(int sd, short args, void *cbdata);
static void local_recv(int status, orte_process_name_t* sender,
                       opal_buffer_t *buffer,
                       orte_rml_tag_t tag, void *cbdata);
static void spawn_recv(int status, orte_process_name_t* sender,
                       opal_buffer_t *buffer,
                       orte_rml_tag_t tag, void *cbdata);

void submit_job(char *argv[]);


int main(int argc, char *argv[])
{
    int i;

    for (i = 0; i < 10; i++) {
        char *arg;
        char ** tmpargv;

        tmpargv = opal_argv_copy(argv);

        opal_argv_append_nosize(&tmpargv, "bash");
        opal_argv_append_nosize(&tmpargv, "-c");
        asprintf(&arg, "t=%d; echo $t; sleep $t", i);
        //asprintf(&arg, "t=%d; echo $t; sleep $t", 1);
        opal_argv_append_nosize(&tmpargv, arg);

        submit_job(tmpargv);
    }

    while (myspawn > 0 || mywait > 0) {
        opal_event_loop(orte_event_base, OPAL_EVLOOP_ONCE);
    }

    printf("DONE\n");
    /* cleanup and leave */
    orte_finalize();

    if (orte_debug_flag) {
        fprintf(stderr, "exiting with status %d\n", orte_exit_status);
    }
    exit(orte_exit_status);
}

