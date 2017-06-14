#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"
#include "orte/mca/errmgr/errmgr.h"

int active;
int remaining;

#define TASKS 1000
#define CORES 8
#define SLEEP "0"
#define TASK_SIZE 1

void launch_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d launched with status: %d!\n", index, ret);

    int tid = *(int *)cbdata;

    if (ret < 0) {
        printf("Task %d launch failed with status: %d!\n", tid, ret);
        exit(-1);
        active -= TASK_SIZE;
    } else {
        printf("Task %d launch successful with status: %d!\n", tid, ret);
    }
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {

    int tid = *(int *)cbdata;

    if (ret == 0) {
        printf("Task %d completed succesfully!\n", tid);
    } else if (ret > 0) {
        printf("Task %d failed with error %d!\n", tid, ret);
    } else if (ret == ORTE_ERR_JOB_CANCELLED) {
        printf("Task %d was cancelled!\n", tid);
    } else {
        printf("Task %d failed with error %d (%s)!\n", tid, ret, ORTE_ERROR_NAME(ret));
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

    remaining = TASKS;

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    opal_argv_append_nosize(&argv, "radical-pilot");
    opal_argv_append_nosize(&argv, "--hnp");
    opal_argv_append_nosize(&argv, "file:dvm_uri");
    opal_argv_append_nosize(&argv, "--mca");
    opal_argv_append_nosize(&argv, "timer_require_monotonic");
    opal_argv_append_nosize(&argv, "false");

    argc = 6;

    rc = orte_submit_init(argc, argv, NULL);
    if (rc > 0) {
        printf("init failed!\n");
        exit(rc);
    }

    while (remaining > 0 || active > 0) {
        if ((active * TASK_SIZE) < CORES && remaining > 0) {
            char **cmd = NULL; // Required for the functioning of opal_argv_command
            opal_argv_append_nosize(&cmd, "orte-submit");
            opal_argv_append_nosize(&cmd, "--np");
            opal_argv_append_nosize(&cmd, "1");
            opal_argv_append_nosize(&cmd, "--output-filename");
            //opal_argv_append_nosize(&cmd, "./:nojobid,nocopy");
            opal_argv_append_nosize(&cmd, "output:nocopy");
            opal_argv_append_nosize(&cmd, "/bin/date");
            //opal_argv_append_nosize(&cmd, "sleep");
            //opal_argv_append_nosize(&cmd, SLEEP);
            //opal_argv_append_nosize(&cmd, "sh");
            //opal_argv_append_nosize(&cmd, "-c");
            //opal_argv_append_nosize(&cmd, "lsof -p $(pidof orted)");

            tids[tid] = tid;
            rc = orte_submit_job(cmd, &index, launch_cb, &tids[tid], finish_cb, &tids[tid]);

            if (rc == 0) {
                printf("Task %d submitted!\n", tid);
                remaining--;
                active += TASK_SIZE;
            } else {
                printf("Task submission failed!\n");
            }
            opal_argv_free(cmd);

            tid++;
        }


        //usleep(10000);
    }


    orte_submit_finalize();
    exit(orte_exit_status);
}

