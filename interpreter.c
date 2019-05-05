/* A BF intepreter.
 *
 * Date: May 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USAGE "Usage: %s <program.bf> [-zon]\n" \
    "Options:\n" \
    "\t-z\tInput a zero on EOF (default behavior)\n" \
    "\t-o\tInput a negative one on EOF\n" \
    "\t-n\tDo nothing on EOF (preserve existing value)\n"

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
    fpos_t *pos;
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

void push(pos_stack_t *stack, fpos_t *pos) {
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

fpos_t *peek(pos_stack_t *stack) {
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
void bracket_jump(FILE* prog) {
    int opening_brackets = 1;  // the one we just read
    for (int c = fgetc(prog); c != EOF; c = fgetc(prog)) {
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

int check_brackets(FILE* prog, char *error_msg) {
    int bracket_level = 0;
    for (int c = fgetc(prog); c != EOF; c = fgetc(prog)) {
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
    rewind(prog);
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

void eval(FILE* prog, eof_mode_t eof_mode) {
    {
        char error[100];
        if (!check_brackets(prog, error)) {
            printf("%s", error);
            return;
        }
    }

    node_t *pos = make_node();
    pos_stack_t *pos_stack = new_pos_stack();

    for (int c = fgetc(prog); c != EOF; c = fgetc(prog)) {
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
                    fpos_t *curr_pos = malloc(sizeof(fpos_t));
                    if (NULL == curr_pos) {
                        fprintf(stderr, "Error allocating for fpos_t.\n");
                        exit(1);
                    }
                    fgetpos(prog, curr_pos);
                    push(pos_stack, curr_pos);
                }
                break;
            case CLOSE:
                if (pos->val != 0) {
                    fpos_t *new_pos = peek(pos_stack);
                    fsetpos(prog, new_pos);
                } else {
                    pop(pos_stack);
                }
                break;
        }
    }
    free_tape(pos);
    free_pos_stack(pos_stack);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, USAGE, argv[0]);
        return 1;
    }

    eof_mode_t eof_mode = ZERO;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-z") == 0)
            eof_mode = ZERO;
        else if (strcmp(argv[i], "-o") == 0)
            eof_mode = NEG_ONE;
        else if (strcmp(argv[i], "-n") == 0)
            eof_mode = NOOP;
        else {
            fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            fprintf(stderr, USAGE, argv[0]);
            return 1;
        }
    }

    FILE *program = fopen(argv[1], "r");
    if (NULL == program) {
        fprintf(stderr, "Error opening program file.\n");
        return 1;
    }

    eval(program, eof_mode);

    if (fclose(program)) {
        fprintf(stderr, "Error closing program file.\n");
        return 1;
    }
}
