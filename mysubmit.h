#include "orte/mca/plm/plm.h"

int submit_job(char *argv[], void (*launch_cb)(int), void (*finish_cb)(int, int));
