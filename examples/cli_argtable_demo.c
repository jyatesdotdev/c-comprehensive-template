/**
 * @file cli_argtable_demo.c
 * @brief Structured CLI parsing with argtable3 — typed args, validation, help.
 *
 * Build: cmake -DUSE_ARGTABLE3=ON ..
 * Usage: ./example_cli_argtable --help
 *        ./example_cli_argtable -v --output result.txt --jobs 4 input1.c input2.c
 */
#include "argtable3.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    /* Define typed argument table */
    struct arg_lit  *help = arg_lit0("h", "help", "print this help and exit");
    struct arg_lit  *verbose = arg_lit0("v", "verbose", "enable verbose output");
    struct arg_str  *output = arg_str0("o", "output", "<file>", "output file (default: stdout)");
    struct arg_int  *jobs = arg_int0("j", "jobs", "<n>", "parallel jobs (default: 1)");
    struct arg_file *infiles = arg_filen(NULL, NULL, "<input>", 0, 10, "input source files");
    struct arg_end  *end = arg_end(20);

    void       *argtable[] = {help, verbose, output, jobs, infiles, end};
    const char *progname = "cli_argtable_demo";

    /* Verify table allocation */
    if (arg_nullcheck(argtable) != 0) {
        fprintf(stderr, "%s: insufficient memory\n", progname);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }

    /* Set defaults */
    jobs->ival[0] = 1;

    /* Parse */
    int nerrors = arg_parse(argc, argv, argtable);

    if (help->count > 0) {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        printf("\nA demo tool showing argtable3 structured argument parsing.\n\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
    }

    if (nerrors > 0) {
        arg_print_errors(stderr, end, progname);
        fprintf(stderr, "Try '%s --help' for more information.\n", progname);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }

    /* Validate: jobs must be positive */
    if (jobs->count > 0 && jobs->ival[0] < 1) {
        fprintf(stderr, "%s: --jobs must be >= 1\n", progname);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }

    /* Use parsed values */
    int         njobs = jobs->ival[0];
    const char *outfile = (output->count > 0) ? output->sval[0] : "(stdout)";

    if (verbose->count > 0) {
        printf("Verbose mode enabled\n");
        printf("Output:  %s\n", outfile);
        printf("Jobs:    %d\n", njobs);
        printf("Inputs:  %d file(s)\n", infiles->count);
    }

    for (int i = 0; i < infiles->count; i++) {
        printf("Processing [%d/%d]: %s (jobs=%d)\n", i + 1, infiles->count, infiles->filename[i],
               njobs);
    }

    if (infiles->count == 0) printf("No input files specified. Try '%s --help'.\n", progname);

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}
