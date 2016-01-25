#include "opal/util/opal_environ.h"
#include "orte/mca/plm/plm.h"

#include "mysubmit.h"

int mywait;
int myspawn;

void launch_cb(int task) {
    printf("Task %d launched!\n", task);
    myspawn--;
}

void finish_cb(int task, int status) {
    printf("Task %d returned %d!\n", task, status);
    mywait--;
}

int main(int argc, char *argv[])
{
    int i;
    int task;

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    for (i = 0; i < 10; i++) {
        char *arg;
        char ** tmpargv;
        void * cb;

        tmpargv = opal_argv_copy(argv);

        opal_argv_append_nosize(&tmpargv, "bash");
        opal_argv_append_nosize(&tmpargv, "-c");
        asprintf(&arg, "t=%d; echo $t; sleep $t", i);
        opal_argv_append_nosize(&tmpargv, arg);

        task = submit_job(tmpargv, &launch_cb, &finish_cb, cb);
        printf("Task %d submitted!\n", task);
        myspawn++;
        mywait++;
    }

    while (myspawn > 0 || mywait > 0) {
        sleep(1);
    }

    printf("DONE\n");
    /* cleanup and leave */
    orte_finalize();

    exit(orte_exit_status);
}
