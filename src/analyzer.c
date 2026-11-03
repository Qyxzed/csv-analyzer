#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void calculate_column_stats(const Table *tbl, size_t c, int is_report, FILE *f)
{
#define printf(...)                   \
    do                               \
    {                                \
        if (is_report)               \
            fprintf(f, __VA_ARGS__); \
        else                         \
            printf(__VA_ARGS__);     \
    } while (0)

    DataType col_type = tbl->cols[c].type;
    const char *type_str = "NULL";
    if (col_type == TYPE_INT)
        type_str = "INT";
    else if (col_type == TYPE_DOUBLE)
        type_str = "DOUBLE";
    else if (col_type == TYPE_STRING)
        type_str = "STRING";

    printf("column %zu\nname \"", c);
    char *cell_name = tbl->cols[c].name;
    for (size_t i = 0; i < strlen(cell_name); i++){
        if (cell_name[i]=='"') printf("\"");
        printf("%c", cell_name[i]);
    }
    printf("\"\ntype %s\n", type_str);

    if (col_type == TYPE_NULL)
    {
        printf("rows %zu\n", tbl->num_rows);
        return;
    }

    size_t nulls = 0;
    int64_t min_int = 0, max_int = 0;
    long double sum_int = 0.0L;
    size_t valid_int_count = 0;
    size_t positive_int = 0, negative_int = 0, even_int = 0;

    double min_dbl = 0.0, max_dbl = 0.0;
    long double sum_dbl = 0.0L;
    size_t valid_dbl_count = 0;
    size_t positive_dbl = 0, negative_dbl = 0, integer_dbl = 0;

    size_t min_len = 0, max_len = 0;
    long double sum_len = 0.0L;
    size_t valid_str_count = 0;
    size_t empty_str_count = 0;
    bool first_str = true;

    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        const Cell *cell = &tbl->cells[r][c];
        if (cell->is_null || cell->type == TYPE_NULL)
        {
            nulls++;
            continue;
        }

        if (col_type == TYPE_INT)
        {
            if (cell->type == TYPE_INT)
            {
                int64_t val = cell->int_val;
                if (valid_int_count == 0)
                    min_int = max_int = val;
                else
                {
                    if (val < min_int)
                        min_int = val;
                    if (val > max_int)
                        max_int = val;
                }
                sum_int += val;
                valid_int_count++;

                if (val > 0)
                    positive_int++;
                if (val < 0)
                    negative_int++;
                if (val % 2 == 0)
                    even_int++;
            }
        }
        else if (col_type == TYPE_DOUBLE)
        {
            double val = 0.0;
            bool has_val = false;

            if (cell->type == TYPE_DOUBLE)
            {
                val = cell->double_val;
                has_val = true;
            }
            else if (cell->type == TYPE_INT)
            {
                val = (double)cell->int_val;
                has_val = true;
            }

            if (has_val)
            {
                if (valid_dbl_count == 0)
                    min_dbl = max_dbl = val;
                else
                {
                    if (val < min_dbl)
                        min_dbl = val;
                    if (val > max_dbl)
                        max_dbl = val;
                }
                valid_dbl_count++;
                sum_dbl += val;

                if (val > 0.0)
                    positive_dbl++;
                if (val < 0.0)
                    negative_dbl++;

                double intpart;
                if (modf(val, &intpart) == 0.0)
                    integer_dbl++;
            }
        }
        else if (col_type == TYPE_STRING)
        {
            const char *s = cell->text_val ? cell->text_val : "";
            size_t len = strlen(s);
            if (len == 0)
                empty_str_count++;

            if (first_str)
            {
                min_len = max_len = len;
                first_str = false;
            }
            else
            {
                if (len < min_len)
                    min_len = len;
                if (len > max_len)
                    max_len = len;
            }
            sum_len += (long double)len;
            valid_str_count++;
        }
    }

    printf("nulls %zu\n", nulls);

    if (col_type == TYPE_INT)
    {
        double mean = valid_int_count > 0 ? (double)(sum_int / (long double)valid_int_count) : 0.0;
        printf("min %lld\n", (long long)(valid_int_count > 0 ? min_int : 0));
        printf("max %lld\n", (long long)(valid_int_count > 0 ? max_int : 0));
        printf("mean %f\n", mean);
        printf("positive %zu\n", positive_int);
        printf("negative %zu\n", negative_int);
        printf("even %zu\n", even_int);
    }
    else if (col_type == TYPE_DOUBLE)
    {
        double mean = valid_dbl_count > 0 ? (double)(sum_dbl / (long double)valid_dbl_count) : 0.0;
        double vmin = valid_dbl_count > 0 ? min_dbl : 0.0;
        double vmax = valid_dbl_count > 0 ? max_dbl : 0.0;

        if (vmin == 0.0)
            vmin = 0.0;
        if (vmax == 0.0)
            vmax = 0.0;
        if (mean == 0.0)
            mean = 0.0;

        printf("min %f\n", vmin);
        printf("max %f\n", vmax);
        printf("mean %f\n", mean);
        printf("positive %zu\n", positive_dbl);
        printf("negative %zu\n", negative_dbl);
        printf("integer %zu\n", integer_dbl);
    }
    else if (col_type == TYPE_STRING)
    {
        double mean_len = valid_str_count > 0 ? (double)(sum_len / (long double)valid_str_count) : 0.0;
        printf("min_length %zu\n", valid_str_count > 0 ? min_len : 0);
        printf("max_length %zu\n", valid_str_count > 0 ? max_len : 0);
        printf("mean_length %f\n", mean_len);
        printf("empty %zu\n", empty_str_count);
    }
#undef printf
}

void cmd_stats(const Table *tbl)
{
    if (!tbl)
        return;

    printf("rows %zu\n", tbl->num_rows);
    printf("columns %zu\n\n", tbl->num_cols);

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        calculate_column_stats(tbl, c, 0, NULL);
        if (c + 1 < tbl->num_cols)
        {
            printf("\n");
        }
    }
}

int cmd_report(const Table *tbl, const char *filename)
{
    if (!filename || !tbl)
        return 0;

    FILE *f = fopen(filename, "w");
    if (!f)
        return 0;

    fprintf(f, "=== CSV STATS REPORT ===\n");
    fprintf(f, "rows %zu\n", tbl->num_rows);
    fprintf(f, "columns %zu\n\n", tbl->num_cols);

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        calculate_column_stats(tbl, c, 1, f);
        if (c + 1 < tbl->num_cols)
        {
            fprintf(f, "\n");
        }
    }

    fclose(f);
    return 1;
}
