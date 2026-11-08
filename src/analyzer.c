#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
    DataType col_type;
    size_t nulls;

    int64_t min_int, max_int;
    long double sum_int;
    size_t valid_int_count;
    size_t positive_int, negative_int, even_int;

    double min_dbl, max_dbl;
    long double sum_dbl;
    size_t valid_dbl_count;
    size_t positive_dbl, negative_dbl, integer_dbl;

    size_t min_len, max_len;
    long double sum_len;
    size_t valid_str_count;
    size_t empty_str_count;
    bool first_str;
} ColStats;

static void init_col_stats(ColStats *stats, DataType type)
{
    stats->col_type = type;
    stats->nulls = 0;
    stats->min_int = 0;
    stats->max_int = 0;
    stats->sum_int = 0.0L;
    stats->valid_int_count = 0;
    stats->positive_int = 0;
    stats->negative_int = 0;
    stats->even_int = 0;

    stats->min_dbl = 0.0;
    stats->max_dbl = 0.0;
    stats->sum_dbl = 0.0L;
    stats->valid_dbl_count = 0;
    stats->positive_dbl = 0;
    stats->negative_dbl = 0;
    stats->integer_dbl = 0;

    stats->min_len = 0;
    stats->max_len = 0;
    stats->sum_len = 0.0L;
    stats->valid_str_count = 0;
    stats->empty_str_count = 0;
    stats->first_str = true;
}

static void update_col_stats(ColStats *stats, const Cell *cell)
{
    if (cell->is_null || cell->type == TYPE_NULL)
    {
        stats->nulls++;
        return;
    }

    if (stats->col_type == TYPE_INT)
    {
        if (cell->type == TYPE_INT)
        {
            int64_t val = cell->int_val;
            if (stats->valid_int_count == 0)
                stats->min_int = stats->max_int = val;
            else
            {
                if (val < stats->min_int)
                    stats->min_int = val;
                if (val > stats->max_int)
                    stats->max_int = val;
            }
            stats->sum_int += val;
            stats->valid_int_count++;

            if (val > 0)
                stats->positive_int++;
            if (val < 0)
                stats->negative_int++;
            if (val % 2 == 0)
                stats->even_int++;
        }
    }
    else if (stats->col_type == TYPE_DOUBLE)
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
            if (stats->valid_dbl_count == 0)
                stats->min_dbl = stats->max_dbl = val;
            else
            {
                if (val < stats->min_dbl)
                    stats->min_dbl = val;
                if (val > stats->max_dbl)
                    stats->max_dbl = val;
            }
            stats->valid_dbl_count++;
            stats->sum_dbl += val;

            if (val > 0.0)
                stats->positive_dbl++;
            if (val < 0.0)
                stats->negative_dbl++;

            double intpart;
            if (modf(val, &intpart) == 0.0)
                stats->integer_dbl++;
        }
    }
    else if (stats->col_type == TYPE_STRING)
    {
        size_t len = cell->len_bytes;
        if (len == 0)
            stats->empty_str_count++;

        if (stats->first_str)
        {
            stats->min_len = stats->max_len = len;
            stats->first_str = false;
        }
        else
        {
            if (len < stats->min_len)
                stats->min_len = len;
            if (len > stats->max_len)
                stats->max_len = len;
        }
        stats->sum_len += (long double)len;
        stats->valid_str_count++;
    }
}

static void print_col_stats(const Table *tbl, size_t c, const ColStats *stats, int is_report, FILE *f)
{
#define printf(...)                  \
    do                               \
    {                                \
        if (is_report)               \
            fprintf(f, __VA_ARGS__); \
        else                         \
            printf(__VA_ARGS__);     \
    } while (0)

    const char *type_str = "NULL";
    if (stats->col_type == TYPE_INT)
        type_str = "INT";
    else if (stats->col_type == TYPE_DOUBLE)
        type_str = "DOUBLE";
    else if (stats->col_type == TYPE_STRING)
        type_str = "STRING";

    printf("column %zu\nname \"", c);
    char *cell_name = tbl->cols[c].name;
    for (size_t i = 0; i < strlen(cell_name); i++)
    {
        if (cell_name[i] == '"')
            printf("\"");
        printf("%c", cell_name[i]);
    }
    printf("\"\ntype %s\n", type_str);

    if (stats->col_type == TYPE_NULL)
    {
        printf("rows %zu\n", tbl->num_rows);
        return;
    }

    printf("nulls %zu\n", stats->nulls);

    if (stats->col_type == TYPE_INT)
    {
        double mean = stats->valid_int_count > 0 ? (double)(stats->sum_int / (long double)stats->valid_int_count) : 0.0;
        printf("min %lld\n", (long long)(stats->valid_int_count > 0 ? stats->min_int : 0));
        printf("max %lld\n", (long long)(stats->valid_int_count > 0 ? stats->max_int : 0));
        printf("mean %f\n", mean);
        printf("positive %zu\n", stats->positive_int);
        printf("negative %zu\n", stats->negative_int);
        printf("even %zu\n", stats->even_int);
    }
    else if (stats->col_type == TYPE_DOUBLE)
    {
        double mean = stats->valid_dbl_count > 0 ? (double)(stats->sum_dbl / (long double)stats->valid_dbl_count) : 0.0;
        double vmin = stats->valid_dbl_count > 0 ? stats->min_dbl : 0.0;
        double vmax = stats->valid_dbl_count > 0 ? stats->max_dbl : 0.0;

        if (vmin == 0.0)
            vmin = 0.0;
        if (vmax == 0.0)
            vmax = 0.0;
        if (mean == 0.0)
            mean = 0.0;

        printf("min %f\n", vmin);
        printf("max %f\n", vmax);
        printf("mean %f\n", mean);
        printf("positive %zu\n", stats->positive_dbl);
        printf("negative %zu\n", stats->negative_dbl);
        printf("integer %zu\n", stats->integer_dbl);
    }
    else if (stats->col_type == TYPE_STRING)
    {
        double mean_len = stats->valid_str_count > 0 ? (double)(stats->sum_len / (long double)stats->valid_str_count) : 0.0;
        printf("min_length %zu\n", stats->valid_str_count > 0 ? stats->min_len : 0);
        printf("max_length %zu\n", stats->valid_str_count > 0 ? stats->max_len : 0);
        printf("mean_length %f\n", mean_len);
        printf("empty %zu\n", stats->empty_str_count);
    }
#undef printf
}

void cmd_stats(const Table *tbl)
{
    if (!tbl)
        return;

    printf("rows %zu\n", tbl->num_rows);
    printf("columns %zu\n\n", tbl->num_cols);

    if (tbl->num_cols == 0)
        return;

    ColStats *all_stats = malloc(sizeof(ColStats) * tbl->num_cols);
    if (!all_stats)
        return;

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        init_col_stats(&all_stats[c], tbl->cols[c].type);
    }

    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        for (size_t c = 0; c < tbl->num_cols; c++)
        {
            update_col_stats(&all_stats[c], &tbl->cells[r][c]);
        }
    }

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        print_col_stats(tbl, c, &all_stats[c], 0, NULL);
        if (c + 1 < tbl->num_cols)
        {
            printf("\n");
        }
    }

    free(all_stats);
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

    if (tbl->num_cols == 0)
    {
        fclose(f);
        return 1;
    }

    ColStats *all_stats = malloc(sizeof(ColStats) * tbl->num_cols);
    if (!all_stats)
    {
        fclose(f);
        return 0;
    }

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        init_col_stats(&all_stats[c], tbl->cols[c].type);
    }

    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        for (size_t c = 0; c < tbl->num_cols; c++)
        {
            update_col_stats(&all_stats[c], &tbl->cells[r][c]);
        }
    }

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        print_col_stats(tbl, c, &all_stats[c], 1, f);
        if (c + 1 < tbl->num_cols)
        {
            fprintf(f, "\n");
        }
    }

    free(all_stats);
    fclose(f);
    return 1;
}