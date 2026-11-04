#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "csv_parser.h"
#include "binary_io.h"
#include "analyzer.h"

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s schema <file.csv> [options]\n", prog_name);
    fprintf(stderr, "  %s stats <file.csv> [options]\n", prog_name);
    fprintf(stderr, "  %s report <file.csv> <output.txt> [options]\n", prog_name);
    fprintf(stderr, "  %s bin-import <file.csv> <file.bin> [options]\n", prog_name);
    fprintf(stderr, "  %s bin-schema <file.bin>\n", prog_name);
    fprintf(stderr, "  %s bin-stats <file.bin>\n", prog_name);
    fprintf(stderr, "  %s bin-report <file.bin> <output.txt>\n", prog_name);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  --delimiter <char>   Field delimiter\n");
    fprintf(stderr, "  --multiline          Allow multiline fields inside quotes\n");
    fprintf(stderr, "  --scientific         Allow scientific number format\n");
}

static void parse_options(int argc, char **argv, int start_index, Options *opts)
{
    for (int i = start_index; i < argc; i++)
    {
        if (strcmp(argv[i], "--multiline") == 0)
        {
            opts->multiline = true;
        }
        else if (strcmp(argv[i], "--scientific") == 0)
        {
            opts->scientific = true;
        }
        else if (strcmp(argv[i], "--delimiter") == 0 && i + 1 < argc)
        {
            opts->delimiter = argv[i + 1][0];
            i++;
        }
    }
}

static void print_schema(const Table *tbl)
{
    printf("rows %zu\n", tbl->num_rows);
    printf("columns %zu\n", tbl->num_cols);
    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        const char *type_str = "NULL";
        if (tbl->cols[c].type == TYPE_INT)
            type_str = "INT";
        else if (tbl->cols[c].type == TYPE_DOUBLE)
            type_str = "DOUBLE";
        else if (tbl->cols[c].type == TYPE_STRING)
            type_str = "STRING";

        printf("%zu \"", c);
        char *cell_name = tbl->cols[c].name;
        for (size_t i = 0; i < strlen(cell_name); i++)
        {
            if (cell_name[i] == '"')
                printf("\"");
            printf("%c", cell_name[i]);
        }
        printf("\" %s\n", type_str);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "schema") == 0 || strcmp(command, "stats") == 0 ||
        strcmp(command, "report") == 0 || strcmp(command, "bin-import") == 0)
    {

        const char *csv_filename = argv[2];
        Options opts;
        init_options(&opts);

        int opt_start_idx = (strcmp(command, "report") == 0 || strcmp(command, "bin-import") == 0) ? 4 : 3;
        parse_options(argc, argv, opt_start_idx, &opts);

        Table tbl;
        if (parse_csv_file(csv_filename, &opts, &tbl) != 0)
        {
            fprintf(stderr, "Error: failed to parse CSV file '%s'.\n", csv_filename);
            return 1;
        }

        if (strcmp(command, "schema") == 0)
        {
            print_schema(&tbl);
        }
        else if (strcmp(command, "stats") == 0)
        {
            cmd_stats(&tbl);
        }
        else if (strcmp(command, "report") == 0)
        {
            if (argc < 4)
            {
                fprintf(stderr, "Error: report output file not specified.\n");
                free_table(&tbl);
                return 1;
            }
            if (!cmd_report(&tbl, argv[3]))
            {
                fprintf(stderr, "Error: failed to write report to '%s'.\n", argv[3]);
                free_table(&tbl);
                return 1;
            }
        }
        else if (strcmp(command, "bin-import") == 0)
        {
            if (argc < 4)
            {
                fprintf(stderr, "Error: binary output file not specified.\n");
                free_table(&tbl);
                return 1;
            }
            if (!export_binary_file(argv[3], &tbl, &opts))
            {
                fprintf(stderr, "Error: failed to export binary file '%s'.\n", argv[3]);
                free_table(&tbl);
                return 1;
            }
        }

        free_table(&tbl);
        return 0;
    }
    else if (strcmp(command, "bin-schema") == 0 || strcmp(command, "bin-stats") == 0 ||
             strcmp(command, "bin-report") == 0)
    {

        const char *bin_filename = argv[2];
        Table tbl;
        Options opts;

        if (!import_binary_file(bin_filename, &tbl, &opts))
        {
            fprintf(stderr, "Error: invalid binary file format, corrupted signature, or incompatible version ('%s').\n", bin_filename);
            return 1;
        }

        if (strcmp(command, "bin-schema") == 0)
        {
            print_schema(&tbl);
        }
        else if (strcmp(command, "bin-stats") == 0)
        {
            cmd_stats(&tbl);
        }
        else if (strcmp(command, "bin-report") == 0)
        {
            if (argc < 4)
            {
                fprintf(stderr, "Error: report output file not specified.\n");
                free_table(&tbl);
                return 1;
            }
            if (!cmd_report(&tbl, argv[3]))
            {
                fprintf(stderr, "Error: failed to write report to '%s'.\n", argv[3]);
                free_table(&tbl);
                return 1;
            }
        }

        free_table(&tbl);
        return 0;
    }

    fprintf(stderr, "Error: unknown command '%s'.\n", command);
    print_usage(argv[0]);
    return 1;
}