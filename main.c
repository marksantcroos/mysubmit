#include "mysubmit.h"

extern int mywait;
extern int myspawn;

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
        opal_argv_append_nosize(&tmpargv, arg);

        submit_job(tmpargv);
    }

    while (myspawn > 0 || mywait > 0) {
        opal_event_loop(orte_event_base, OPAL_EVLOOP_ONCE);
    }

    printf("DONE\n");
    /* cleanup and leave */
    orte_finalize();

    exit(orte_exit_status);
}
