#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"
#include "orte/mca/errmgr/errmgr.h"

int mywait;
int myspawn;

#define TASKS 4096
#define CORES "1"

void launch_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d launched with status: %d!\n", index, ret);
    myspawn--;

    if (ret < 0)
        mywait--;
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    if (ret == 0)
        printf("Task %d completed succesfully!\n", index);
    else if (ret > 0)
        printf("Task %d failed with error %d!\n", index, ret);
    else if (ret == ORTE_ERR_JOB_CANCELLED)
        printf("Task %d was cancelled!\n", index);
    else
        printf("Task %d failed with error %d (%s)!\n", index, ret, ORTE_ERROR_NAME(ret));

    mywait--;
}

int main()
{
    int i;
    int index;
    int rc;
    int argc;
    char **argv = NULL;


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
    
    for (i = 0; i < TASKS; i++) {
        char **cmd = NULL; // Required for the functioning of opal_argv_command
        opal_argv_append_nosize(&cmd, "orte-submit");
        opal_argv_append_nosize(&cmd, "--np");
        opal_argv_append_nosize(&cmd, CORES);
        opal_argv_append_nosize(&cmd, "bash");
        //opal_argv_append_nosize(&cmd, "blash");
        opal_argv_append_nosize(&cmd, "-c");

        char *arg;
        //asprintf(&arg, "t=%d; echo $t; sleep $t", i);
        //asprintf(&arg, "sleep 30");
        //asprintf(&arg, "false");
        //asprintf(&arg, "true");
        asprintf(&arg, "hostname");
        opal_argv_append_nosize(&cmd, arg);
        free(arg);

        rc = orte_submit_job(cmd, &index, launch_cb, NULL, finish_cb, NULL);

        if (rc == 0) {
            printf("Task %d submitted!\n", index);
            myspawn++;
            mywait++;
        } else {
            printf("Task submission failed!\n");
        }
        opal_argv_free(cmd);

        //if (index == 2) {
        //    printf("Sleeping before killing task ...\n");
        //    sleep(1);
        //    printf("Awake ... killing task!\n");
        //    orte_submit_cancel(index);
       // }
    }

    while (myspawn > 0 || mywait > 0) {
        usleep(10000);
    }

    //printf("Shutting down DVM\n");
    //orte_submit_halt();
    //sleep(1);

    orte_submit_finalize();

    exit(orte_exit_status);
}
