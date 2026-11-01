#ifndef ANALYZER_H
#define ANALYZER_H

#include "types.h"

void print_escaped_string(const char *str);
void cmd_schema(const Table *tbl);
void cmd_stats(const Table *tbl);
int cmd_report(const Table *tbl, const char *out_filename);

#endif