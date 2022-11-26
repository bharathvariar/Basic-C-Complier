#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum
{
    T_PLUS,
    T_MINUS,
    T_MULT,
    T_DIV,
    T_LT,
    T_GT,
    T_EQUAL,
    T_NEQUAL,
    T_ASSIGN,
    T_OPAREN,
    T_CPAREN,
    T_NUMBER,
    T_IDENTIFIER,
    T_FOR,
    T_DEFAULT,
    T_WRITE,
    T_OCBRACE,
    T_CCBRACE,
    T_READ,
    T_EOS,
    T_EOI,
    T_UNKNOWN,
} t_symbol;

typedef enum
{
    ST_EXPRESSION,
    ST_FOR,
    ST_DEFAULT,
    ST_READ,
    ST_PRINT
} stmt_symbol;

struct c_symbol
{
    t_symbol sym;
    char arr[1024];
    long long int value;
};
typedef enum
{
    INTEGER,
    OPERATION
} t_flag;
typedef enum
{
    VALUE,
    VARIABLE,
    ENODE
} t_value;
typedef struct variable var_t;

struct operator
{
    t_symbol op;
    int unary, prec, dir;
};

struct variable
{
    char *name;
    long long int value;
    var_t *next;
};

typedef struct node enode_symbol;
typedef struct statement stnode_symbol;

struct node
{
    t_symbol op;
    union
    {
        long long int value;
        struct variable *var;
    };
    enode_symbol *left, *right;
};

struct for_stmt
{
    enode_symbol *expr1, *expr2, *expr3;
    stnode_symbol *body;
};

struct default_st
{
    stnode_symbol *body;
};

struct statement
{
    stmt_symbol type;
    union
    {
        enode_symbol *expr;
        struct for_stmt *f;
    };
    struct statement *next;
};

enode_symbol *stack[1024];
int len = 0;

void push_enode(enode_symbol *e)
{
    if (len == 1024)
        die("E-node stack overflow");
    stack[len++] = e;
}

enode_symbol *pop_enode(void)
{
    if (len <= 0)
        die("E-node stack underflow");
    return stack[--len];
}

struct operator opstack[1024];
int len2 = 0;

void push_op(t_symbol op, int unary, int prec, int dir)
{
    if (len2 == 1024)
        die("Op stack overflow");
    opstack[len2++] = (struct operator){.op = op, .unary = unary, .prec = prec, .dir = dir};
}

struct operator pop_op(void)
{
    if (len <= 0)
        die("Op stack underflow");
    return opstack[--len2];
}

t_symbol peek_op(void)
{
    if (len2 <= 0)
        return T_UNKNOWN;
    return opstack[len2 - 1].op;
}

int peek_prec(void)
{
    if (len2 <= 0)
        return -1;
    return opstack[len2 - 1].prec;
}
FILE *input;

struct c_symbol _cur;
int _cstack[100];
int _csp = 0;

int get(void)
{
    if (_csp)
        return _cstack[--_csp];
    return fgetc(input);
}
void unget(int c)
{
    if (c != EOF)
        _cstack[_csp++] = c;
}
int next(int c)
{
    int ch = get();
    if (ch == c)
        return 1;
    unget(ch);
    return 0;
}
t_symbol identifier(int c, char *buf)
{
    int p = 0;
    buf[p++] = c;
    for (c = get(); isalpha(c) || isdigit(c) || c == '_'; c = get())
        buf[p++] = c;
    buf[p] = '\0';
    unget(c);
    if (strcmp(buf, "for") == 0)
        return T_FOR;
    if (strcmp(buf, "default") == 0)
        return T_DEFAULT;
    if (strcmp(buf, "write") == 0)
        return T_WRITE;
    if (strcmp(buf, "read") == 0)
        return T_READ;
    return T_IDENTIFIER;
}
t_symbol lex(long long int *value, char *arr)
{
    int c, n;
    do
    {
        c = get();
    } while (isspace(c));

    switch (c)
    {
    case EOF:
        return T_EOI;
    case ';':
        return T_EOS;
    case '+':
        return T_PLUS;
    case '-':
        return T_MINUS;
    case '/':
        return T_DIV;
    case '(':
        return T_OPAREN;
    case ')':
        return T_CPAREN;
    case '{':
        return T_OCBRACE;
    case '}':
        return T_CCBRACE;
    case '<':
        return T_LT;
    case '>':
        return T_GT;
    case '*':
        return T_MULT;
    case '=':
        if (next('='))
            return T_EQUAL;
        return T_ASSIGN;
    case '!':
        if (next('='))
            return T_NEQUAL;
        return T_UNKNOWN;
    default:
        if (isalpha(c) || c == '_')
            return identifier(c, arr);
        if (isdigit(c))
        {
            n = 0;
            do
            {
                n *= 10;
                n += (c - '0');
            } while (isdigit(c = get()));
            unget(c);
            *value = n;
            return T_NUMBER;
        }
        return T_UNKNOWN;
    }
    return T_UNKNOWN;
}

void startlex(FILE *fp)
{
    input = fopen("input.txt", "r");
    _cur.sym = lex(&_cur.value, _cur.arr);
}

int switchresult = 0;
int switchmatch = 0;

long long int evaluateExp(enode_symbol *e)
{
    long long int l, r;
    if (e->left && e->op != T_ASSIGN)
        l = eval_expr(e->left);
    if (e->right)
        r = eval_expr(e->right);
    switch (e->op)
    {
    case T_NUMBER:
        return e->value;
    case T_IDENTIFIER:
        return e->var->value;
    case T_PLUS:
        return l + r;
    case T_MINUS:
        return l - r;
    case T_MULT:
        return l * r;
    case T_DIV:
        return l / r;
    case T_LT:
        return l < r;
    case T_GT:
        return l > r;
    case T_EQUAL:
        return l == r;
    case T_NEQUAL:
        return l != r;
    case T_ASSIGN:
        e->left->var->value = r;
        return r;
    default:
        return 0;
    }
    return 0;
}

void run(stnode_symbol *st, int *cont, int *brk)
{
    stnode_symbol *n;

    for (n = st; n != NULL; n = n->next)
    {
        switch (n->type)
        {
        case ST_EXPRESSION:
            eval_expr(n->expr);
            break;
        case ST_PRINT:
            printf("%ld\n", eval_expr(n->expr));
            break;
        case ST_READ:
            printf("%ld\n", eval_expr(n->expr));
            break;
        case ST_FOR:
            *brk = *cont = 0;
            for (eval_expr(n->f->expr1); eval_expr(n->f->expr2); eval_expr(n->f->expr3))
            {
                run(n->f->body, cont, brk);
                if (*brk)
                    break;
                if (*cont)
                {
                    *cont = 0;
                    continue;
                }
            }
            *brk = 0;
            break;
        }
    }
    return;
}

extern struct c_symbol _cur;
char *tokstr[] = {
    "+", "-", "*", "/", "<", ">", "==", "!=", "=", "(", ")",
    "#", "var", "while", "for", "write",
    "{", "}", ";", "EOI"};