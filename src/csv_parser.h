#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "types.h"

int parse_csv_file(const char *filename, const Options *opts, Table *tbl);

#endif