#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING
} DataType;

typedef struct {
    char *text_val;      
    int64_t int_val;    
    double double_val;  
    DataType type;      
    bool is_null;       
    bool is_empty_str;  
    bool is_quoted;
    size_t len_bytes;   
} Cell;

typedef struct {
    char *name;         
    DataType type;      
} Column;

typedef struct {
    size_t num_rows;
    size_t num_cols;
    Column *cols;
    Cell **cells;      
} Table;

typedef struct {
    char delimiter;
    bool multiline;
    bool scientific;
} Options;

void init_options(Options *opts);
void free_table(Table *tbl);

#endif