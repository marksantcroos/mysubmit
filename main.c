#include "opal/util/opal_environ.h"
#include "orte/mca/plm/plm.h"

int mywait;
int myspawn;

int orte_submit_job(char *argv[], void (*launch_cb)(int), void (*finish_cb)(int, int));

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
        char **tmpargv;

        tmpargv = opal_argv_copy(argv);

        opal_argv_append_nosize(&tmpargv, "bash");
        opal_argv_append_nosize(&tmpargv, "-c");
        asprintf(&arg, "t=%d; echo $t; sleep $t", i);
        opal_argv_append_nosize(&tmpargv, arg);

        task = orte_submit_job(tmpargv, &launch_cb, &finish_cb);
        if (task >= 0) {
            printf("Task %d submitted!\n", task);
            myspawn++;
            mywait++;
        } else printf("Task submission failed!\n");
    }

    while (myspawn > 0 || mywait > 0) {
        sleep(1);
    }

    printf("DONE\n");

    exit(orte_exit_status);
}
