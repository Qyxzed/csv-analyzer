#include "csv_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

static bool is_valid_int(const char *str, int64_t *val_out) {
    if (!str || *str == '\0') return false;
    char *endptr = NULL;
    errno = 0;
    long long v = strtoll(str, &endptr, 10);
    if (errno != 0 || *endptr != '\0') return false;
    if (val_out) *val_out = (int64_t)v;
    return true;
}

static bool is_valid_double(const char *str, double *val_out, bool allow_sci) {
    if (!str || *str == '\0') return false;
    
    if (!allow_sci) {
        for (const char *p = str; *p; p++) {
            if (*p == 'e' || *p == 'E') return false;
        }
    }

    char *endptr = NULL;
    errno = 0;
    double v = strtod(str, &endptr);
    if (errno != 0 || *endptr != '\0') return false;
    if (isnan(v) || isinf(v)) return false;

    if (val_out) *val_out = v;
    return true;
}

static void determine_cell_type(Cell *cell, const Options *opts) {
    if (cell->is_null)
    {
        cell->type = TYPE_NULL;
        return;
    }

    if (is_valid_int(cell->text_val, &cell->int_val))
    {
        cell->type = TYPE_INT;
        cell->double_val = (double)cell->int_val;
        return;
    }

    if (is_valid_double(cell->text_val, &cell->double_val, opts->scientific))
    {
        cell->type = TYPE_DOUBLE;
        return;
    }

    cell->type = TYPE_STRING;
}

int parse_csv_file(const char *filename, const Options *opts, Table *tbl) {
    FILE *f = fopen(filename, "rb");
    if (!f){
        fprintf(stderr, "Error: cannot open file\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        fprintf(stderr, "Error: incorrect stream or stream pointer to opened file\n");
        return 1;
    }

    char *buffer = (char *)malloc((size_t)fsize + 1);
    if (!buffer) {
        fclose(f);
        fprintf(stderr, "Error: memory allocation error");
        return 1;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)fsize, f);
    fclose(f);
    buffer[read_bytes] = '\0';

    size_t max_rows = 16, max_cols = 16;
    tbl->num_rows = 0;
    tbl->num_cols = 0;
    
    char ***raw_table = malloc(sizeof(char **) * max_rows);
    bool **raw_quoted = malloc(sizeof(bool *) * max_rows);
    size_t *row_col_counts = malloc(sizeof(size_t) * max_rows);

    size_t cur_row = 0;
    size_t cur_col = 0;
    
    char **cur_row_fields = malloc(sizeof(char *) * max_cols);
    bool *cur_row_quoted = malloc(sizeof(bool) * max_cols);

    size_t field_cap = 64;
    size_t field_len = 0;
    char *field_buf = malloc(field_cap);

    bool in_quotes = false;
    bool field_is_quoted = false;

    for (size_t i = 0; i <= read_bytes; i++) {
        char c = buffer[i];

        if (c == '\0' && i < read_bytes) continue;

        if (in_quotes) {
            if (c == '\0') {
                free(buffer);
                for (size_t k = 0; k < cur_row; k++)
                {
                    for (size_t j = 0; j < row_col_counts[k]; j++){
                        free(raw_table[k][j]);
                    }
                    free(raw_table[k]);
                    free(raw_quoted[k]);
                }
                free(raw_quoted);
                free(row_col_counts);
                free(cur_row_fields);
                free(cur_row_quoted);
                free(field_buf);
                fprintf(stderr, "Error: unclosed quotation mark\n");
                return 1;
            }
            if (c == '"') {
                if (i + 1 < read_bytes && buffer[i + 1] == '"') {
                    if (field_len + 1 >= field_cap) {
                        field_cap *= 2;
                        field_buf = realloc(field_buf, field_cap);
                    }
                    field_buf[field_len++] = '"';
                    i++;
                } else {
                    in_quotes = false;
                }
            } else if (c == '\n' && !opts->multiline) {
                free(buffer);
                for (size_t k = 0; k < cur_row; k++)
                {
                    for (size_t j = 0; j < row_col_counts[k]; j++)
                    {
                        free(raw_table[k][j]);
                    }
                    free(raw_table[k]);
                    free(raw_quoted[k]);
                }
                free(raw_quoted);
                free(row_col_counts);
                free(cur_row_fields);
                free(cur_row_quoted);
                free(field_buf);
                fprintf(stderr, "Error: line translation without --multiline\n");
                return 1;
            } else {
                if (field_len + 1 >= field_cap) {
                    field_cap *= 2;
                    field_buf = realloc(field_buf, field_cap);
                }
                field_buf[field_len++] = c;
            }
        } else {
            if (c == '"') {
                if (field_len == 0 && !field_is_quoted) {
                    in_quotes = true;
                    field_is_quoted = true;
                } else {
                    free(buffer);
                    for (size_t k = 0; k < cur_row; k++)
                    {
                        for (size_t j = 0; j < row_col_counts[k]; j++)
                        {
                            free(raw_table[k][j]);
                        }
                        free(raw_table[k]);
                        free(raw_quoted[k]);
                    }
                    free(raw_quoted);
                    free(row_col_counts);
                    free(cur_row_fields);
                    free(cur_row_quoted);
                    free(field_buf);
                    fprintf(stderr, "Quotation format error\n");
                    return 1;
                }
            } else if (c == opts->delimiter || c == '\n' || c == '\r' || c == '\0') {
                if (c == '\r')
                {
                    if (i + 1 < read_bytes && buffer[i + 1] == '\n')
                        i++;
                    c = '\n';
                }

                field_buf[field_len] = '\0';

                if (field_len > 0 && field_buf[field_len - 1] == '\r')
                {
                    field_buf[field_len - 1] = '\0';
                    field_len--;
                }

                if (cur_col >= max_cols)
                {
                    max_cols *= 2;
                    cur_row_fields = realloc(cur_row_fields, sizeof(char *) * max_cols);
                    cur_row_quoted = realloc(cur_row_quoted, sizeof(bool) * max_cols);
                }

                cur_row_fields[cur_col] = strdup(field_buf);
                cur_row_quoted[cur_col] = field_is_quoted;
                cur_col++;

                field_len = 0;
                field_is_quoted = false;

                if (c == '\n' || c == '\0') {
                    if (c == '\0' && cur_col == 1 && strlen(cur_row_fields[0]) == 0 && cur_row == 0) {
                        free(cur_row_fields[0]);
                        cur_col = 0;
                        break;
                    }

                    if (cur_row >= max_rows)
                    {
                        max_rows *= 2;
                        raw_table = realloc(raw_table, sizeof(char **) * max_rows);
                        raw_quoted = realloc(raw_quoted, sizeof(bool *) * max_rows); // Исправлен размер типа
                        row_col_counts = realloc(row_col_counts, sizeof(size_t) * max_rows);
                    }

                    raw_table[cur_row] = cur_row_fields;
                    raw_quoted[cur_row] = cur_row_quoted;
                    row_col_counts[cur_row] = cur_col;
                    cur_row++;

                    cur_col = 0;
                    max_cols = 16;
                    cur_row_fields = malloc(sizeof(char *) * max_cols);
                    cur_row_quoted = malloc(sizeof(bool) * max_cols);

                    if (c == '\0') break;
                }
            } else {
                if (field_len + 1 >= field_cap) {
                    field_cap *= 2;
                    field_buf = realloc(field_buf, field_cap);
                }
                field_buf[field_len++] = c;
            }
        }
    }

    free(field_buf);
    free(cur_row_fields);
    free(cur_row_quoted);
    free(buffer);

    if (cur_row == 0) {
        free(raw_table);
        free(raw_quoted);
        free(row_col_counts);
        fprintf(stderr, "There is something terrible.\n");
        return 1;
    }

    size_t num_cols = row_col_counts[0];
    for (size_t r = 1; r < cur_row; r++) {
        if (row_col_counts[r] != num_cols) {
            for (size_t i = 0; i < cur_row; i++) {
                for (size_t j = 0; j < row_col_counts[i]; j++) free(raw_table[i][j]);
                free(raw_table[i]);
                free(raw_quoted[i]);
            }
            free(raw_table);
            free(raw_quoted);
            free(row_col_counts);
            fprintf(stderr, "Error: mismatched number of columns\n");
            return 1;
        }
    }

    tbl->num_cols = num_cols;
    tbl->num_rows = cur_row - 1;
    tbl->cols = malloc(sizeof(Column) * num_cols);

    for (size_t c = 0; c < num_cols; c++) {
        tbl->cols[c].name = strdup(raw_table[0][c]);
        tbl->cols[c].type = TYPE_NULL;
    }

    tbl->cells = malloc(sizeof(Cell *) * tbl->num_rows);
    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        tbl->cells[r] = malloc(sizeof(Cell) * num_cols);
        for (size_t c = 0; c < num_cols; c++)
        {
            char *raw_val = raw_table[r + 1][c];
            bool was_quoted = raw_quoted[r + 1][c];
            Cell *cell = &tbl->cells[r][c];

            cell->text_val = strdup(raw_val);
            cell->len_bytes = strlen(raw_val);
            cell->is_empty_str = (cell->len_bytes == 0);
            cell->is_quoted = was_quoted;
            cell->is_null = (cell->is_empty_str && !cell->is_quoted);

            determine_cell_type(cell, opts);
        }
    }

    for (size_t c = 0; c < num_cols; c++) {
        DataType col_type = TYPE_NULL;
        for (size_t r = 0; r < tbl->num_rows; r++) {
            DataType cell_type = tbl->cells[r][c].type;
            if (cell_type == TYPE_NULL) continue;

            if (col_type == TYPE_NULL) {
                col_type = cell_type;
            } else if (col_type == TYPE_INT && cell_type == TYPE_DOUBLE) {
                col_type = TYPE_DOUBLE;
            } else if (col_type == TYPE_DOUBLE && cell_type == TYPE_INT) {
                // Остаётся DOUBLE
            } else if (col_type != cell_type) {
                col_type = TYPE_STRING;
            }
        }
        tbl->cols[c].type = col_type;
    }

    for (size_t i = 0; i < cur_row; i++) {
        for (size_t j = 0; j < row_col_counts[i]; j++) free(raw_table[i][j]);
        free(raw_table[i]);
        free(raw_quoted[i]);
    }
    free(raw_table);
    free(raw_quoted);
    free(row_col_counts);

    return 0;
}
