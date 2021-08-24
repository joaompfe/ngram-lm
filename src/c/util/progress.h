// Copyright (c) 2021, João Fé, All rights reserved.

#ifndef NGRAM_LM_PROGRESS_H
#define NGRAM_LM_PROGRESS_H

#include "log.h"

extern inline void progress_bar(const char *desc, uint64_t i, uint64_t total)
{
    if (i % (total / 101) == 0 || i == (total - 1)) {
        const uint64_t prog = (i + 1) * 100 / total;
        log_info("%s: %d%%", desc, (int) prog);
        if (prog < 100) {
            printf("\033[1A");
            fflush(stdout);
        }
    }
}

#endif //NGRAM_LM_PROGRESS_H
