#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"
#include "orte/mca/errmgr/errmgr.h"
#include "opal/class/opal_pointer_array.h"

int active;
int remaining;

#define TASKS 1
#define CORES 4
#define SLEEP "0"
#define TASK_SIZE 1

void launch_cb_bulk(int index, orte_job_t *jdata, int ret, void *cbdata) {

    int tid = *(int *)cbdata;

    if (ret < 0) {
        printf("Task %d (index: %d) launch failed with status: %d!\n", tid, index, ret);
        exit(-1);
        active -= TASK_SIZE;
    } else {
        printf("Task %d (index: %d) launch successful with status: %d!\n", tid, index, ret);
    }
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {

    int tid = *(int *)cbdata;

    if (ret == 0) {
        printf("Task %d (index: %d) completed succesfully!\n", tid, index);
    } else if (ret > 0) {
        printf("Task %d (index: %d) failed with error %d!\n", tid, index, ret);
    } else if (ret == ORTE_ERR_JOB_CANCELLED) {
        printf("Task %d (index: %d) was cancelled!\n", tid, index);
    } else {
        printf("Task %d (index: %d) failed with error %d (%s)!\n", tid, index, ret, ORTE_ERROR_NAME(ret));
    }

    active -= TASK_SIZE;
}

int main()
{
    int index;
    int rc;
    int argc;
    char **argv = NULL;
    int tids[TASKS];
    int tid = 0;
    int i=0;

    opal_pointer_array_t cmds;
    OBJ_CONSTRUCT(&cmds, opal_pointer_array_t);
    opal_pointer_array_init(&cmds, 10, INT_MAX, 1);

    remaining = TASKS;

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    opal_argv_append_nosize(&argv, "radical-pilot");
    opal_argv_append_nosize(&argv, "--hnp");
    opal_argv_append_nosize(&argv, "file:dvm_uri");
    //opal_argv_append_nosize(&argv, "--mca");
    //opal_argv_append_nosize(&argv, "timer_require_monotonic");
    //opal_argv_append_nosize(&argv, "false");

    argc = 3;

    rc = orte_submit_init(argc, argv, NULL);
    if (rc > 0) {
        printf("init failed!\n");
        exit(rc);
    }

    int N = 2;

    for (i=0; i<=N; i++) {

        char **cmd = NULL; // Required for the functioning of opal_argv_command
        opal_argv_append_nosize(&cmd, "orterun");
        opal_argv_append_nosize(&cmd, "--np");
        opal_argv_append_nosize(&cmd, "1");
        //opal_argv_append_nosize(&cmd, "--output-filename");
        //opal_argv_append_nosize(&cmd, "./:nojobid,nocopy");
        //opal_argv_append_nosize(&cmd, "output:nocopy");
        opal_argv_append_nosize(&cmd, "/bin/date");
        //opal_argv_append_nosize(&cmd, "sleep");
        //opal_argv_append_nosize(&cmd, SLEEP);
        //opal_argv_append_nosize(&cmd, "sh");
        //opal_argv_append_nosize(&cmd, "-c");
        //opal_argv_append_nosize(&cmd, "lsof -p $(pidof orted)");

        index = opal_pointer_array_add(&cmds, cmd);
    }

    tids[tid] = tid;
    // Basically need to use the index, as the cdata's are required in RP to pass the handle
    rc = orte_submit_job_bulk(cmds, &index, launch_cb_bulk, &tids[tid], finish_cb, &tids[tid]);

    if (rc == 0) {
        printf("Task %d (index: %d) submitted!\n", tid, index);
        remaining--;
        active += TASK_SIZE;
    } else {
        printf("Task submission failed!\n");
    }
//    opal_argv_free(cmd);




    orte_submit_finalize();
    exit(orte_exit_status);
}

