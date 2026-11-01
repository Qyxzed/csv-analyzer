#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void cmd_stats(const Table *tbl)
{
    if (!tbl)
        return;

    printf("=== TABLE STATISTICS ===\n");
    printf("Total rows: %zu\n", tbl->num_rows);
    printf("Total columns: %zu\n", tbl->num_cols);
    printf("------------------------\n");

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        printf("Column %zu: \"%s\"\n", c, tbl->cols[c].name);

        size_t null_count = 0;
        size_t int_count = 0;
        size_t double_count = 0;
        size_t string_count = 0;

        int64_t min_int = 0, max_int = 0;
        double min_double = 0.0, max_double = 0.0;
        double sum_double = 0.0;
        bool first_int = true;
        bool first_double = true;

        for (size_t r = 0; r < tbl->num_rows; r++)
        {
            const Cell *cell = &tbl->cells[r][c];
            if (cell->is_null || cell->type == TYPE_NULL)
            {
                null_count++;
                continue;
            }

            if (cell->type == TYPE_INT)
            {
                int_count++;
                if (first_int)
                {
                    min_int = max_int = cell->int_val;
                    first_int = false;
                }
                else
                {
                    if (cell->int_val < min_int)
                        min_int = cell->int_val;
                    if (cell->int_val > max_int)
                        max_int = cell->int_val;
                }
                sum_double += (double)cell->int_val;
            }
            else if (cell->type == TYPE_DOUBLE)
            {
                double_count++;
                if (first_double)
                {
                    min_double = max_double = cell->double_val;
                    first_double = false;
                }
                else
                {
                    if (cell->double_val < min_double)
                        min_double = cell->double_val;
                    if (cell->double_val > max_double)
                        max_double = cell->double_val;
                }
                sum_double += cell->double_val;
            }
            else
            {
                string_count++;
            }
        }

        printf("  NULL values: %zu\n", null_count);
        printf("  INT values: %zu", int_count);
        if (!first_int)
        {
            printf(" (min: %lld, max: %lld)", (long long)min_int, (long long)max_int);
        }
        printf("\n");

        printf("  DOUBLE values: %zu", double_count);
        if (!first_double)
        {
            printf(" (min: %f, max: %f)", min_double, max_double);
        }
        printf("\n");

        printf("  STRING values: %zu\n", string_count);
        printf("------------------------\n");
    }
}

// Генерация текстового отчёта в файл (для команд report и bin-report)
int cmd_report(const Table *tbl, const char *filename)
{
    if (!filename || !tbl)
        return 0;

    FILE *f = fopen(filename, "w");
    if (!f)
        return 0;

    fprintf(f, "=== CSV STATS REPORT ===\n");
    fprintf(f, "Rows: %zu\n", tbl->num_rows);
    fprintf(f, "Columns: %zu\n", tbl->num_cols);
    fprintf(f, "========================\n\n");

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        fprintf(f, "Column [%zu]: \"%s\"\n", c, tbl->cols[c].name);

        size_t null_count = 0;
        size_t int_count = 0;
        size_t double_count = 0;
        size_t string_count = 0;

        int64_t min_int = 0, max_int = 0;
        double min_double = 0.0, max_double = 0.0;
        bool first_int = true;
        bool first_double = true;

        for (size_t r = 0; r < tbl->num_rows; r++)
        {
            const Cell *cell = &tbl->cells[r][c];
            if (cell->is_null || cell->type == TYPE_NULL)
            {
                null_count++;
                continue;
            }

            if (cell->type == TYPE_INT)
            {
                int_count++;
                if (first_int)
                {
                    min_int = max_int = cell->int_val;
                    first_int = false;
                }
                else
                {
                    if (cell->int_val < min_int)
                        min_int = cell->int_val;
                    if (cell->int_val > max_int)
                        max_int = cell->int_val;
                }
            }
            else if (cell->type == TYPE_DOUBLE)
            {
                double_count++;
                if (first_double)
                {
                    min_double = max_double = cell->double_val;
                    first_double = false;
                }
                else
                {
                    if (cell->double_val < min_double)
                        min_double = cell->double_val;
                    if (cell->double_val > max_double)
                        max_double = cell->double_val;
                }
            }
            else
            {
                string_count++;
            }
        }

        fprintf(f, "  - NULL count: %zu\n", null_count);
        fprintf(f, "  - INT count: %zu", int_count);
        if (!first_int)
        {
            fprintf(f, " | Min: %lld, Max: %lld", (long long)min_int, (long long)max_int);
        }
        fprintf(f, "\n");

        fprintf(f, "  - DOUBLE count: %zu", double_count);
        if (!first_double)
        {
            fprintf(f, " | Min: %f, Max: %f", min_double, max_double);
        }
        fprintf(f, "\n");

        fprintf(f, "  - STRING count: %zu\n", string_count);
        fprintf(f, "----------------------------------------\n");
    }

    fclose(f);
    return 1;
}