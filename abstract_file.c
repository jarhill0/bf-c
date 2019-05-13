/* Abstract a file away to either a real file or a char array.
 *
 * Date: May 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "abstract_file.h"

enum file_kind {
    CHAR_ARR,
    REAL_FILE,
};

struct abstract_file_s {
    enum file_kind data_kind;
    long pos, len;
    union {
        FILE *real_file;
        const char *char_arr;
    } data;
};

abstract_file *malloc_abstract_file() {
    abstract_file *file = malloc(sizeof(abstract_file));
    if (NULL == file) {
        fprintf(stderr, "Error allocating memory for abstract_file.\n");
        exit(1);
    }
    return file;
}

abstract_file *open_char_arr(const char *char_arr) {
    abstract_file *file = malloc_abstract_file();
    file->data_kind = CHAR_ARR;
    file->pos = 0;
    file->len = strlen(char_arr);
    file->data.char_arr = char_arr;
    return file;
}

abstract_file *open_real_file(FILE* real_file) {
    abstract_file *file = malloc_abstract_file();
    file->data_kind = REAL_FILE;
    file->data.real_file = real_file;
    return file;
}

int abs_fgetc(abstract_file *file) {
    switch(file->data_kind) {
        case CHAR_ARR:
            return file->pos == file->len
                ? EOF
                : file->data.char_arr[file->pos++];
        case REAL_FILE:
            return fgetc(file->data.real_file);
    }
}

void abs_rewind(abstract_file *file) {
    switch(file->data_kind) {
        case CHAR_ARR:
            file->pos = 0;
            break;
        case REAL_FILE:
            rewind(file->data.real_file);
            break;
    }
}

void abs_fgetpos(abstract_file *file, apos_t *apos) {
    switch(file->data_kind) {
        case CHAR_ARR:
            apos->arr_ind = file->pos;
            break;
        case REAL_FILE:
            apos->f_pos = malloc(sizeof(fpos_t));
            if (NULL == apos->f_pos) {
                fprintf(stderr, "Error allocating memory for fpos_t.\n");
                exit(1);
            }
            fgetpos(file->data.real_file, apos->f_pos);
            break;
    }
}

void abs_fsetpos(abstract_file *file, apos_t *apos) {
    switch(file->data_kind) {
        case CHAR_ARR:
            file->pos = apos->arr_ind;
            break;
        case REAL_FILE:
            fsetpos(file->data.real_file, apos->f_pos);
            break;
    }
}
