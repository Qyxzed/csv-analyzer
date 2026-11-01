#include "types.h"
#include <stdlib.h>

void init_options(Options *opts) {
    if (!opts) return;
    opts->delimiter = ',';
    opts->multiline = false;
    opts->scientific = false;
}

void free_table(Table *tbl) {
    if (!tbl) return;

    if (tbl->cols) {
        for (size_t i = 0; i < tbl->num_cols; i++) {
            free(tbl->cols[i].name);
        }
        free(tbl->cols);
    }

    if (tbl->cells) {
        for (size_t r = 0; r < tbl->num_rows; r++) {
            if (tbl->cells[r]) {
                for (size_t c = 0; c < tbl->num_cols; c++) {
                    free(tbl->cells[r][c].text_val);
                }
                free(tbl->cells[r]);
            }
        }
        free(tbl->cells);
    }

    tbl->cols = NULL;
    tbl->cells = NULL;
    tbl->num_rows = 0;
    tbl->num_cols = 0;
}