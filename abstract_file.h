/* Abstract a file away to either a real file or a char array.
 *
 * Date: May 2019
 */

#ifndef ABSTRACT_FILE_H
#define ABSTRACT_FILE_H

typedef struct abstract_file_s abstract_file;
typedef union abstract_pos_u {
    fpos_t *f_pos;
    long arr_ind;
} apos_t;

abstract_file *open_char_arr(const char*);
abstract_file *open_real_file(FILE*);

int abs_fgetc(abstract_file*);
void abs_rewind(abstract_file*);
void abs_fgetpos(abstract_file*, apos_t*);
void abs_fsetpos(abstract_file*, apos_t*);

#endif
