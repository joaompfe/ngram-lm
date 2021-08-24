// Copyright (c) 2021, João Fé, All rights reserved.

#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <c/util/log.h>
#include "trie.h"

const char *argp_program_version =
        "ngram-lm 0.1";
const char *argp_program_bug_address =
        "<joaofe2000@gmail.com>";

static char doc[] = "This programs builds a trie-based ngram language model from an ARPA file.";

static char args_doc[] = "ARPA_FILE OUT_FILE";

static struct argp_option options[] = {
        { "order", 'n', "ORDER", 0, "N-gram order", 0 },
        { 0 }
};

struct arguments {
    int order;
    char *file;
    char *out;
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key) {
        case 'n':
            sscanf(arg, "=%d", &arguments->order);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 2)
                argp_usage(state);
            switch (state->arg_num) {
                case 0:
                    arguments->file = arg;
                    break;
                case 1:
                    arguments->out = arg;
                    break;
            }
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 2)
                argp_usage(state);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void build_trie_from_arpa(const char *arpa_path, unsigned short order,
                          const char *out_path)
{
    struct trie *t = trie_new_from_arpa(order, arpa_open(arpa_path));
    FILE *f = fopen(out_path, "wb");
    if (f == NULL) {
        log_error("File '%s' could not be opened: %s.\n", out_path,
                  strerror(errno));
        exit(EXIT_FAILURE);
    }
    trie_fwrite(t, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    struct arguments arguments;

    arguments.order = 0;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    log_info("Building the %d-gram trie...", arguments.order);
    build_trie_from_arpa(arguments.file, arguments.order, arguments.out);
    log_info("Language model successfully build");

    exit(0);
}
