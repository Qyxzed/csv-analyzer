#include "binary_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char MAGIC_SIGNATURE[4] = {'C', 'S', 'V', 'B'};
static const uint32_t CURRENT_VERSION = 1;

int export_binary_file(const char *filename, const Table *tbl, const Options *opts)
{
    if (!filename || !tbl)
        return 0;

    FILE *f = fopen(filename, "wb");
    if (!f)
        return 0;

    setvbuf(f, NULL, _IOFBF, 1 << 20);

    if (fwrite(MAGIC_SIGNATURE, sizeof(char), 4, f) != 4 ||
        fwrite(&CURRENT_VERSION, sizeof(uint32_t), 1, f) != 1)
    {
        fclose(f);
        return 0;
    }

    Options default_opts;
    init_options(&default_opts);
    const Options *o = opts ? opts : &default_opts;

    if (fwrite(&o->delimiter, sizeof(char), 1, f) != 1 ||
        fwrite(&o->multiline, sizeof(bool), 1, f) != 1 ||
        fwrite(&o->scientific, sizeof(bool), 1, f) != 1 ||
        fwrite(&tbl->num_rows, sizeof(size_t), 1, f) != 1 ||
        fwrite(&tbl->num_cols, sizeof(size_t), 1, f) != 1)
    {
        fclose(f);
        return 0;
    }

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        if (fwrite(&tbl->cols[c].type, sizeof(DataType), 1, f) != 1)
        {
            fclose(f);
            return 0;
        }
        size_t name_len = tbl->cols[c].name ? strlen(tbl->cols[c].name) : 0;
        if (fwrite(&name_len, sizeof(size_t), 1, f) != 1)
        {
            fclose(f);
            return 0;
        }
        if (name_len > 0 && fwrite(tbl->cols[c].name, sizeof(char), name_len, f) != name_len)
        {
            fclose(f);
            return 0;
        }
    }

    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        for (size_t c = 0; c < tbl->num_cols; c++)
        {
            const Cell *cell = &tbl->cells[r][c];

            if (fwrite(&cell->type, sizeof(DataType), 1, f) != 1 ||
                fwrite(&cell->is_null, sizeof(bool), 1, f) != 1 ||
                fwrite(&cell->is_quoted, sizeof(bool), 1, f) != 1 ||
                fwrite(&cell->is_empty_str, sizeof(bool), 1, f) != 1 ||
                fwrite(&cell->int_val, sizeof(int64_t), 1, f) != 1 ||
                fwrite(&cell->double_val, sizeof(double), 1, f) != 1)
            {
                fclose(f);
                return 0;
            }

            size_t text_len = cell->text_val ? strlen(cell->text_val) : 0;
            if (fwrite(&text_len, sizeof(size_t), 1, f) != 1)
            {
                fclose(f);
                return 0;
            }
            if (text_len > 0 && fwrite(cell->text_val, sizeof(char), text_len, f) != text_len)
            {
                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return 1;
}

int import_binary_file(const char *filename, Table *tbl, Options *opts)
{
    if (!filename || !tbl)
        return 0;

    tbl->cols = NULL;
    tbl->cells = NULL;
    tbl->num_rows = 0;
    tbl->num_cols = 0;

    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;

    setvbuf(f, NULL, _IOFBF, 1 << 20);

    char sig[4];
    if (fread(sig, sizeof(char), 4, f) != 4 || memcmp(sig, MAGIC_SIGNATURE, 4) != 0)
    {
        fclose(f);
        fprintf(stderr, "Error: invalid file format or damaged signature\n");
        return 0;
    }

    uint32_t file_version;
    if (fread(&file_version, sizeof(uint32_t), 1, f) != 1 || file_version != CURRENT_VERSION)
    {
        fclose(f);
        fprintf(stderr, "Error: unsupported version\n");
        return 0;
    }

    Options temp_opts;
    if (fread(&temp_opts.delimiter, sizeof(char), 1, f) != 1 ||
        fread(&temp_opts.multiline, sizeof(bool), 1, f) != 1 ||
        fread(&temp_opts.scientific, sizeof(bool), 1, f) != 1 ||
        fread(&tbl->num_rows, sizeof(size_t), 1, f) != 1 ||
        fread(&tbl->num_cols, sizeof(size_t), 1, f) != 1)
    {
        fclose(f);
        return 0;
    }
    if (opts)
        *opts = temp_opts;

    if (tbl->num_cols > 1000000 || tbl->num_rows > 100000000)
    {
        fclose(f);
        fprintf(stderr, "Error: file dimensions too large\n");
        return 0;
    }

    tbl->cols = malloc(sizeof(Column) * (tbl->num_cols ? tbl->num_cols : 1));
    if (!tbl->cols)
    {
        fclose(f);
        return 0;
    }

    for (size_t c = 0; c < tbl->num_cols; c++)
    {
        if (fread(&tbl->cols[c].type, sizeof(DataType), 1, f) != 1)
            goto fail_cleanup;

        size_t name_len;
        if (fread(&name_len, sizeof(size_t), 1, f) != 1)
            goto fail_cleanup;

        if (name_len > 1048576)
            goto fail_cleanup;

        tbl->cols[c].name = malloc(name_len + 1);
        if (!tbl->cols[c].name)
            goto fail_cleanup;

        if (name_len > 0 && fread(tbl->cols[c].name, sizeof(char), name_len, f) != name_len)
            goto fail_cleanup;
        tbl->cols[c].name[name_len] = '\0';
    }

    tbl->cells = malloc(sizeof(Cell *) * (tbl->num_rows ? tbl->num_rows : 1));
    if (!tbl->cells)
        goto fail_cleanup;

    for (size_t r = 0; r < tbl->num_rows; r++)
    {
        tbl->cells[r] = malloc(sizeof(Cell) * (tbl->num_cols ? tbl->num_cols : 1));
        if (!tbl->cells[r])
            goto fail_cleanup;

        for (size_t c = 0; c < tbl->num_cols; c++)
        {
            Cell *cell = &tbl->cells[r][c];

            cell->text_val = NULL;
            cell->int_val = 0;
            cell->double_val = 0.0;
            cell->len_bytes = 0;
            cell->is_null = false;
            cell->is_empty_str = false;
            cell->is_quoted = false;

            if (fread(&cell->type, sizeof(DataType), 1, f) != 1 ||
                fread(&cell->is_null, sizeof(bool), 1, f) != 1 ||
                fread(&cell->is_quoted, sizeof(bool), 1, f) != 1 ||
                fread(&cell->is_empty_str, sizeof(bool), 1, f) != 1 ||
                fread(&cell->int_val, sizeof(int64_t), 1, f) != 1 ||
                fread(&cell->double_val, sizeof(double), 1, f) != 1)
            {
                goto fail_cleanup;
            }

            size_t text_len;
            if (fread(&text_len, sizeof(size_t), 1, f) != 1)
                goto fail_cleanup;

            if (text_len > 1048576)
                goto fail_cleanup;

            cell->text_val = malloc(text_len + 1);
            if (!cell->text_val)
                goto fail_cleanup;

            if (text_len > 0 && fread(cell->text_val, sizeof(char), text_len, f) != text_len)
            {
                goto fail_cleanup;
            }
            cell->text_val[text_len] = '\0';
            cell->len_bytes = text_len;
        }
    }

    fclose(f);
    return 1;

fail_cleanup:
    fclose(f);
    free_table(tbl);
    fprintf(stderr, "Error: corrupted binary file data\n");
    return 0;
}