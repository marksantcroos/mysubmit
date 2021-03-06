from cffi import FFI
ffi = FFI()

ffi.set_source("ompi_cffi", """

//#include "opal/util/opal_environ.h"
#include "orte/mca/plm/plm.h"

#include "mysubmit.h"

""",
    libraries=[
        "submit",
        "open-pal",
        "open-rte"
    ],
    library_dirs=[
        "../installed/DEBUG/lib",
        "/Users/mark/proj/openmpi/mysubmit"
    ],
    include_dirs=[
        "../src/ompi/orte/include",
        "../src/ompi/build/opal/include",
        "../src/ompi/opal/include",
        "../src/ompi",
        "../src/ompi/opal/mca/event/libevent2022/libevent",
        "../src/ompi/build/opal/mca/hwloc/hwloc1111/hwloc/include",
        "../src/ompi/build/opal/mca/event/libevent2022/libevent/include",
        "../src/ompi/opal/mca/event/libevent2022/libevent/include",
        "../src/ompi/opal/mca/hwloc/hwloc1111/hwloc/include"
    ])

ffi.cdef("""

/* Constants */
#define OPAL_EVLOOP_ONCE ...

/* Functions */
int submit_job(char *argv[], void (*launch_cb)(int, void *), void (*finish_cb)(int, int, void *), void *cbdata);
int opal_event_loop(struct event_base *, int);

/* Callbacks */
extern "Python" void launch_cb(int, void *);
extern "Python" void finish_cb(int, int, void *);

/* Variables */
typedef struct event_base opal_event_base_t;
opal_event_base_t *orte_event_base = {0};

""")

if __name__ == "__main__":
    ffi.compile()
