#ifndef BINARY_IO_H
#define BINARY_IO_H

#include "types.h"

int export_binary_file(const char *filename, const Table *tbl, const Options *opts);
int import_binary_file(const char *filename, Table *tbl, Options *opts);

#endif 