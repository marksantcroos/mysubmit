#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"

int mywait;
int myspawn;

#define TASKS 9
#define CORES "1"

void launch_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d launched!\n", index);
    myspawn--;
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d returned %d!\n", index, ret);
    mywait--;
}

int main()
{
    int i;
    int task;
    int rc;
    int argc;
    char **argv = NULL;

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    opal_argv_append_nosize(&argv, "radical-pilot");
    opal_argv_append_nosize(&argv, "--hnp");
    opal_argv_append_nosize(&argv, "file:dvm_uri");
    argc = 3;

    rc = orte_submit_init(argc, argv);
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
        opal_argv_append_nosize(&cmd, "-c");

        char *arg;
        asprintf(&arg, "t=%d; echo $t; sleep $t", i);
        opal_argv_append_nosize(&cmd, arg);
        free(arg);

        task = orte_submit_job(cmd, launch_cb, NULL, finish_cb, NULL);

        if (task >= 0) {
            printf("Task %d submitted!\n", task);
            myspawn++;
            mywait++;
        } else {
            printf("Task submission failed!\n");
        }

        opal_argv_free(cmd);
    }

    while (myspawn > 0 || mywait > 0) {
        usleep(10000);
    }

    printf("DONE\n");

    exit(orte_exit_status);
}
