// Copyright (c) 2021, João Fé, All rights reserved.

#include <argp.h>
#include <stdlib.h>
#include "ngram_trie.h"


const char *argp_program_version =
        "ngram-lm 0.1";
const char *argp_program_bug_address =
        "<joaofe2000@gmail.com>";

static char doc[] = "This programs builds a trie-based ngram language model from an ARPA file.";

static char args_doc[] = "ARPA_FILE OUT_FILE";

static struct argp_option options[] = {
        {"order",   'n', "ORDER",  0,"N-gram order" },
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

int main (int argc, char **argv)
{
    struct arguments arguments;

    arguments.order = 0;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf ("Building the %d-gram trie...", arguments.order);
    build_trie_from_arpa(arguments.file, arguments.order, arguments.out);

    exit(0);
}