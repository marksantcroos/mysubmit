#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"
#include "orte/mca/errmgr/errmgr.h"

int active;
int remaining;

#define TASKS 640
#define CORES 128

void launch_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    //printf("Task %d launched with status: %d!\n", index, ret);

    if (ret < 0) {
        printf("Task %d launch failed with status: %d!\n", index, ret);
        active--;
    }
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    if (ret == 0) {
        //printf("Task %d completed succesfully!\n", index);
    } else if (ret > 0) {
        printf("Task %d failed with error %d!\n", index, ret);
    } else if (ret == ORTE_ERR_JOB_CANCELLED) {
        printf("Task %d was cancelled!\n", index);
    } else {
        printf("Task %d failed with error %d (%s)!\n", index, ret, ORTE_ERROR_NAME(ret));
    }

    active--;
}

int main()
{
    int index;
    int rc;
    int argc;
    char **argv = NULL;

    remaining = TASKS;

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    opal_argv_append_nosize(&argv, "radical-pilot");
    opal_argv_append_nosize(&argv, "--hnp");
    opal_argv_append_nosize(&argv, "file:dvm_uri");
    argc = 3;

    rc = orte_submit_init(argc, argv, NULL);
    if (rc > 0) {
        printf("init failed!\n");
        exit(rc);
    }
    
    while (remaining > 0 || active > 0) {
        if (active < CORES && remaining > 0) {
            char **cmd = NULL; // Required for the functioning of opal_argv_command
            opal_argv_append_nosize(&cmd, "orte-submit");
            opal_argv_append_nosize(&cmd, "--np");
            opal_argv_append_nosize(&cmd, "1");
            opal_argv_append_nosize(&cmd, "sleep");
            opal_argv_append_nosize(&cmd, "60");

            rc = orte_submit_job(cmd, &index, launch_cb, NULL, finish_cb, NULL);

            if (rc == 0) {
                //printf("Task %d submitted!\n", index);
                remaining--;
                active++;
            } else {
                printf("Task submission failed!\n");
            }
            opal_argv_free(cmd);
        }

        usleep(10000);
    }


    orte_submit_finalize();
    exit(orte_exit_status);
}
