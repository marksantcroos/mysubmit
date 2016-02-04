#include "opal/util/opal_environ.h"
#include "orte/orted/orted_submit.h"

int mywait;
int myspawn;

#define TASKS 9
#define CORES "1"

void launch_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d launched with status: %d!\n", index, ret);
    myspawn--;
}

static void finish_cb(int index, orte_job_t *jdata, int ret, void *cbdata) {
    printf("Task %d returned %d!\n", index, ret);
    mywait--;
}

struct orterun_globals_t {
    bool help;
    bool version;
    bool verbose;
    char *report_pid;
    char *report_uri;
    bool exit;
    bool debugger;
    int num_procs;
    char *env_val;
    char *appfile;
    char *wdir;
    bool set_cwd_to_session_dir;
    char *path;
    char *preload_files;
    bool sleep;
    char *stdin_target;
    char *prefix;
    char *path_to_mpirun;
#if OPAL_ENABLE_FT_CR == 1
    char *sstore_load;
#endif
    bool disable_recovery;
    bool preload_binaries;
    bool index_argv;
    bool run_as_root;
    char *personality;
    bool dvm;
} orterun_globals;

static opal_cmd_line_init_t cmd_line_init[] = {
    { "orte_execute_quiet", 'q', NULL, "quiet", 0,
      NULL, OPAL_CMD_LINE_TYPE_BOOL,
      "Suppress helpful messages" },

    { NULL, '\0', "report-pid", "report-pid", 1,
      &orterun_globals.report_pid, OPAL_CMD_LINE_TYPE_STRING,
      "Printout pid on stdout [-], stderr [+], or a file [anything else]" },
    { NULL, '\0', "report-uri", "report-uri", 1,
      &orterun_globals.report_uri, OPAL_CMD_LINE_TYPE_STRING,
      "Printout URI on stdout [-], stderr [+], or a file [anything else]" },

    /* exit status reporting */
    { "orte_report_child_jobs_separately", '\0', "report-child-jobs-separately", "report-child-jobs-separately", 0,
      NULL, OPAL_CMD_LINE_TYPE_BOOL,
      "Return the exit status of the primary job only" },

    /* select XML output */
    { "orte_xml_output", '\0', "xml", "xml", 0,
      NULL, OPAL_CMD_LINE_TYPE_BOOL,
      "Provide all output in XML format" },
    { "orte_xml_file", '\0', "xml-file", "xml-file", 1,
      NULL, OPAL_CMD_LINE_TYPE_STRING,
      "Provide all output in XML format to the specified file" },

    /* tag output */
    { "orte_tag_output", '\0', "tag-output", "tag-output", 0,
      NULL, OPAL_CMD_LINE_TYPE_BOOL,
      "Tag all output with [job,rank]" },
    { "orte_timestamp_output", '\0', "timestamp-output", "timestamp-output", 0,
      NULL, OPAL_CMD_LINE_TYPE_BOOL,
      "Timestamp all application process output" },
    { "orte_output_filename", '\0', "output-filename", "output-filename", 1,
      NULL, OPAL_CMD_LINE_TYPE_STRING,
      "Redirect output from application processes into filename.rank" },
    { "orte_xterm", '\0', "xterm", "xterm", 1,
      NULL, OPAL_CMD_LINE_TYPE_STRING,
      "Create a new xterm window and display output from the specified ranks there" },

    /* End of list */
    { NULL, '\0', NULL, NULL, 0,
      NULL, OPAL_CMD_LINE_TYPE_NULL, NULL }
};


int main()
{
    int i;
    int task;
    int rc;
    int argc;
    char **argv = NULL;
    opal_cmd_line_t cmd_line;


    /* setup our cmd line */
    opal_cmd_line_create(&cmd_line, cmd_line_init);
    mca_base_cmd_line_setup(&cmd_line);

    opal_setenv("OMPI_MCA_ess_tool_async_progress", "1", true, &environ);

    opal_argv_append_nosize(&argv, "radical-pilot");
    opal_argv_append_nosize(&argv, "--hnp");
    opal_argv_append_nosize(&argv, "file:dvm_uri");
    argc = 3;

    rc = orte_submit_init(argc, argv, &cmd_line);
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
