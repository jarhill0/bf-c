/* A BF interpreter.
 *
 * Date: May 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abstract_file.h"

#define USAGE "Usage: %s <program.bf> [-zon]\n" \
    "Options:\n" \
    "\t-z\tInput a zero on EOF (default behavior)\n" \
    "\t-o\tInput a negative one on EOF\n" \
    "\t-n\tDo nothing on EOF (preserve existing value)\n\n" \
    "\t-b\tBenchmark program (count number of instructions executed)\n\n"

// symbolic constants for BF program characters
#define LEFT '<'
#define RIGHT '>'
#define INC '+'
#define DEC '-'
#define INP ','
#define OUTP '.'
#define OPEN '['
#define CLOSE ']'

typedef struct node_s {
    unsigned char val;
    struct node_s *left, *right;
} node_t;

// Create and initialize a new node.
node_t *make_node() {
    node_t *node = malloc(sizeof(node_t));
    if (NULL == node) {
        fprintf(stderr, "Error allocating node. Exiting.");
        exit(1);
    }
    node->val = 0;
    node->left = node->right = NULL;
    return node;
}

// Get the node to the left of the parameter, or create a new node if no left
// node exists.
node_t *left(node_t *node) {
    if (NULL == node->left) {
        node->left = make_node();
        node->left->right = node;
    }
    return node->left;
}

// Get the node to the right of the parameter, or create a new node if no right
// node exists.
node_t *right(node_t *node) {
    if (NULL == node->right) {
        node->right = make_node();
        node->right->left = node;
    }
    return node->right;
}

// Free an entire linked list, given a pointer to any of its elements.
void free_tape(node_t *node) {
    node_t *start = node;
    while (start->left != NULL) {
        start = start->left;
    }
    while (start->right != NULL) {
        start = start->right;
        free(start->left);
    }
    free(start);
}

// Represent a stack of file positions.
typedef struct pos_stack_node_s {
    apos_t *pos;
    struct pos_stack_node_s *down;
} pos_stack_node_t;
typedef struct {
    pos_stack_node_t *top;
} pos_stack_t;

pos_stack_t *new_pos_stack() {
    pos_stack_t *stack = malloc(sizeof(pos_stack_t));
    if (NULL == stack) {
        fprintf(stderr, "Error allocating position stack. Exiting.\n");
        exit(1);
    }
    stack->top = NULL;
    return stack;
}

void push(pos_stack_t *stack, apos_t *pos) {
    if (NULL == stack) {
        fprintf(stderr, "Cannot operate on null stack. Exiting.\n");
        exit(1);
    }

    pos_stack_node_t *new_node = malloc(sizeof(pos_stack_node_t));
    if (NULL == new_node) {
        fprintf(stderr, "Error allocating position node. Exiting.\n");
        exit(1);
    }
    new_node->pos = pos;
    new_node->down = stack->top;
    stack->top = new_node;
}

// Pop from the stack WITHOUT returning the value.
void pop(pos_stack_t *stack) {
    if (NULL == stack) {
        fprintf(stderr, "Cannot operate on null stack. Exiting.\n");
        exit(1);
    }
    if (NULL == stack->top) {
        fprintf(stderr, "Cannot pop empty stack. Exiting.\n");
        exit(1);
    }

    pos_stack_node_t *top = stack->top;
    stack->top = top->down;
    free(top->pos);
    free(top);
}

apos_t *peek(pos_stack_t *stack) {
    if (NULL == stack) {
        fprintf(stderr, "Cannot operate on null stack. Exiting.\n");
        exit(1);
    }
    if (NULL == stack->top) {
        fprintf(stderr, "Cannot peek empty stack. Exiting.\n");
        exit(1);
    }

    return stack->top->pos;
}

void free_pos_stack(pos_stack_t *stack) {
    if (stack != NULL) {
        while (stack->top != NULL) {
            pos_stack_node_t *prev_top = stack->top;
            stack->top = prev_top->down;
            free(prev_top->pos);
            free(prev_top);
        }
        free(stack);
    }
}

// Seek forward in the file until the closing bracket
// matching the opening bracket at the current position in the file.
void bracket_jump(abstract_file* prog) {
    int opening_brackets = 1;  // the one we just read
    for (int c = abs_fgetc(prog); c != EOF; c = abs_fgetc(prog)) {
        switch(c) {
            case OPEN:
                opening_brackets++;
                break;
            case CLOSE:
                opening_brackets--;
                if (0 == opening_brackets) {
                    return;  // we have seeked to the right place
                }
                break;
        }
    }
    fprintf(stderr, "\nError: mismatching brackets. Exiting.\n");
    exit(1);
}

int check_brackets(abstract_file* prog, char *error_msg) {
    int bracket_level = 0;
    for (int c = abs_fgetc(prog); c != EOF; c = abs_fgetc(prog)) {
        switch(c) {
            case OPEN:
                bracket_level++;
                break;
            case CLOSE:
                bracket_level--;
        }

        if (bracket_level < 0)
            break;
    }
    abs_rewind(prog);
    if (bracket_level < 0) {
        strcpy(error_msg, "Error: unmatched closing bracket!\n");
    } else if (bracket_level > 0) {
        strcpy(error_msg, "Error: unmatched opening bracket!\n");
    }
    return 0 == bracket_level;
}

typedef enum {
    ZERO,
    NEG_ONE,
    NOOP,
} eof_mode_t;

void eval(abstract_file* prog, eof_mode_t eof_mode, int benchmark) {
    {
        char error[100];
        if (!check_brackets(prog, error)) {
            printf("%s", error);
            return;
        }
    }

    node_t *pos = make_node();
    pos_stack_t *pos_stack = new_pos_stack();
    long long int instr_count = 0;

    for (int c = abs_fgetc(prog); c != EOF; c = abs_fgetc(prog)) {
        instr_count++;
        switch(c) {
            case LEFT:
                pos = left(pos);
                break;
            case RIGHT:
                pos = right(pos);
                break;
            case INC:
                pos->val++;
                break;
            case DEC:
                pos->val--;
                break;
            case INP:
                {
                    char inp = getchar();
                    if (EOF == inp) {
                        switch(eof_mode) {
                            case ZERO:
                                pos->val = 0;
                                break;
                            case NEG_ONE:
                                pos->val = -1;
                                break;
                            case NOOP:
                                break;
                        }
                    } else {
                        pos->val = inp;
                    }
                }
                break;
            case OUTP:
                putchar(pos->val);
                break;
            case OPEN:
                if (0 == pos->val) {
                    bracket_jump(prog);
                } else  {
                    apos_t *curr_pos = malloc(sizeof(apos_t));
                    if (NULL == curr_pos) {
                        fprintf(stderr, "Error allocating memory for apos_t\n");
                        exit(1);
                    }
                    abs_fgetpos(prog, curr_pos);
                    push(pos_stack, curr_pos);
                }
                break;
            case CLOSE:
                if (pos->val != 0) {
                    apos_t *new_pos = peek(pos_stack);
                    abs_fsetpos(prog, new_pos);
                } else {
                    pop(pos_stack);
                }
                break;
            default:
                instr_count--;  // comments don't count as instructions...
        }
    }
    if (benchmark)
        printf("\n%lld instructions executed.\n", instr_count);

    free_tape(pos);
    free_pos_stack(pos_stack);
}

int eval_file(const char *filename, eof_mode_t eof_mode, int benchmark) {
    FILE *program = fopen(filename, "r");
    if (NULL == program) {
        fprintf(stderr, "Error opening program file.\n");
        return 1;
    }

    abstract_file *program_wrapped = open_real_file(program);

    eval(program_wrapped, eof_mode, benchmark);

    free(program_wrapped);
    program_wrapped = NULL;

    if (fclose(program)) {
        fprintf(stderr, "Error closing program file.\n");
        return 1;
    }
    return 0;
}

int eval_str(const char *str, eof_mode_t eof_mode, int benchmark) {
    abstract_file *program = open_char_arr(str);
    eval(program, eof_mode, benchmark);
    free(program);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, USAGE, argv[0]);
        return 1;
    }

    eof_mode_t eof_mode = ZERO;
    int benchmark = 0;
    for (int i = 2; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                char c = argv[i][j];
                switch(c) {
                    case 'z':
                        eof_mode = ZERO;
                        break;
                    case 'o':
                        eof_mode = NEG_ONE;
                        break;
                    case 'n':
                        eof_mode = NOOP;
                        break;
                    case 'b':
                        benchmark = 1;
                        break;
                    default:
                        fprintf(stderr, "Unknown option '%c'\n", c);
                        fprintf(stderr, USAGE, argv[0]);
                        return 1;
                        break;
                }
            }
        } else {
            fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            fprintf(stderr, USAGE, argv[0]);
            return 1;
        }
    }

    return eval_file(argv[1], eof_mode, benchmark);
}
