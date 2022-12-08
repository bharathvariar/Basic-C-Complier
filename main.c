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
    ST_WRITE
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
// TODO
// Implement parse_stmt
// Implement lookup
// implement run
struct operator opstack[1024];
int len2 = 0;
enode_symbol *stack[1024];
int len = 0;
FILE *input;
struct c_symbol curr;
int _cstack[100];
int _csp = 0;
int osp, esp;

// I COPIED THIS
// Might mess up the code needs to be fixed later
//		 +,  -,  *,  /,  %, ||, &&,  <,  >, <=, >=, ==, !=,  =,  (,  )
int prec[] = {60, 60, 65, 65, 65, 20, 25, 50, 50, 50, 50, 45, 45, 10, 0, 0};
int uprec[] = {75, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int dir[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
int udir[] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void error_(char *why)
{
    fprintf(stderr, "%s\n", why);
    exit(1);
}
void push_enode(enode_symbol *e)
{
    if (len == 1024)
        error_("E-node stack overflow");
    stack[len++] = e;
}

enode_symbol *pop_enode(void)
{
    if (len <= 0)
        error_("E-node stack underflow");
    return stack[--len];
}

void push_op(t_symbol op, int unary, int prec, int dir)
{
    if (len2 == 1024)
        error_("Op stack overflow");
    opstack[len2++] = (struct operator){.op = op, .unary = unary, .prec = prec, .dir = dir};
}

struct operator pop_op(void)
{
    if (len <= 0)
        error_("Op stack underflow");
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
    curr.sym = lex(&curr.value, curr.arr);
}
enode_symbol e_zero = {.op = T_NUMBER, .value = 0, .left = NULL, .right = NULL};

var_t *vars = NULL;
struct variable *lookup(char *name)
{
    var_t *v;

    for (v = vars; v != NULL; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v;
    v = malloc(sizeof(struct variable));
    v->name = strdup(name);
    v->value = 0;
    v->next = vars;
    vars = v;

    return v;
}
enode_symbol *new_enode(t_symbol tok, long long int value)
{
    enode_symbol *e = malloc(sizeof(enode_symbol));
    e->op = tok;
    e->value = value;
    e->left = e->right = NULL;
    return e;
}

void free_enode(enode_symbol *e)
{
    if (e->left)
        free_enode(e->left);
    if (e->right)
        free_enode(e->right);
    if (e != &e_zero)
        free(e);
}

void action(void)
{
    struct operator oper;
    enode_symbol *left, *right;
    long long int n;
    oper = pop_op();
    right = pop_enode();
    if (oper.unary == 0)
        left = pop_enode();
    else
        left = &e_zero;

    if (uprec[oper.op] == -1 && oper.unary)
        error_("Malformed unary expression in action.");
    if (left->op == T_NUMBER && right->op == T_NUMBER)
    {
        switch (oper.op)
        {
        case T_PLUS:
            n = left->value + right->value;
            break;
        case T_MINUS:
            n = left->value - right->value;
            break;
        case T_MULT:
            n = left->value * right->value;
            break;
        case T_DIV:
            n = left->value / right->value;
            break;
        case T_LT:
            n = left->value < right->value;
            break;
        case T_GT:
            n = left->value > right->value;
            break;
        case T_EQUAL:
            n = left->value == right->value;
            break;
        case T_NEQUAL:
            n = left->value != right->value;
            break;
        default:
            break;
        }
        free_enode(left);
        free_enode(right);
        push_enode(new_enode(T_NUMBER, n));
        return;
    }

    enode_symbol *e = new_enode(oper.op, 0);
    e->left = left;
    if (oper.op == T_ASSIGN && left->op != T_IDENTIFIER)
        error_("Left side of assignment is not an lval.\n");
    e->right = right;
    push_enode(e);

    return;
}
enode_symbol *expr(int *empty)
{
    enode_symbol *e;
    int stop = 0, parens = 0, p, d;

    t_flag flag = INTEGER;
    osp = esp = 0;
    do
    {
        printf("%s-%d", curr.arr, curr.sym);
        switch (curr.sym)
        {
        case T_NUMBER:
            if (flag != INTEGER)
                error_("Syntax error");
            push_enode(new_enode(T_NUMBER, curr.value));
            flag = OPERATION;
            break;
        case T_IDENTIFIER:
            e = new_enode(curr.sym, 0);
            e->var = lookup(curr.arr);
            push_enode(e);
            flag = OPERATION;
            break;
        case T_OPAREN:
            parens++;
            push_op(T_OPAREN, 0, 0, 0);
            flag = INTEGER;
            break;
        case T_CPAREN:
            if (parens-- == 0)
            {
                stop = 1;
                break;
            }
            while ((curr.sym = peek_op()) != T_UNKNOWN)
            {
                if (curr.sym == T_OPAREN)
                    break;
                action();
            }
            if (curr.sym == T_UNKNOWN)
                error_("Mismatched parenthesis.");
            pop_op();
            flag = OPERATION;
            break;
        case T_PLUS:
        case T_MINUS:
        case T_MULT:
        case T_DIV:
        case T_LT:
        case T_GT:
        case T_EQUAL:
        case T_NEQUAL:
        case T_ASSIGN:
            p = (flag == INTEGER) ? uprec[curr.sym] : prec[curr.sym];
            if (p == -1)
                error_("Malformed expression, operator not a valid unary operator.");
            d = (flag == INTEGER) ? udir[curr.sym] : dir[curr.sym];
            if (d == 1)
            {
                while (peek_prec() > p)
                    action();
            }
            else
            {
                while (peek_prec() >= p)
                    action();
            }
            push_op(curr.sym, flag == INTEGER, p, d);
            flag = INTEGER;
            break;
        default:
            stop = 1;
            break;
        }
    } while (!stop && ((curr.sym = lex(&curr.value, curr.arr)) != T_EOI));
    while (peek_op() != T_UNKNOWN)
        action();

    e = pop_enode();
    *empty = (esp == 0);
    return e;
}
stnode_symbol *new_stnode(stmt_symbol type)
{
    printf("%d", type);
    stnode_symbol *n = malloc(sizeof(stmt_symbol));
    n->type = type;
    switch (type)
    {
    case ST_FOR:
        n->f = malloc(sizeof(struct for_stmt));
        break;
    default:
        break;
    }
    n->next = NULL;
    return n;
}
void match(t_symbol tok)
{
    if (tok == curr.sym)
        curr.sym = lex(&curr.value, curr.arr);
    else
    {
        printf("Syntax error");
        exit(1);
    }
}
int accept(t_symbol tok)
{
    if (curr.sym == tok)
    {
        match(tok);
        return 1;
    }
    return 0;
}

stnode_symbol *parse_stmt(void)
{
    int empty;
    stnode_symbol *hp = NULL, *ep = NULL, *st;
    switch (curr.sym)
    {
    case T_FOR:
        match(T_FOR);
        st = new_stnode(ST_FOR);
        match(T_OPAREN);
        st->f->expr1 = expr(&empty);
        match(T_EOS);
        st->f->expr2 = expr(&empty);
        match(T_EOS);
        st->f->expr3 = expr(&empty);
        match(T_CPAREN);
        st->f->body = parse_stmt();
        match(T_EOS);
        break;
    case T_WRITE:
        match(T_WRITE);
        st = new_stnode(ST_WRITE);
        st->expr = expr(&empty);
        match(T_EOS);
        break;
    case T_READ:
        match(T_READ);
        st = new_stnode(ST_READ);
        st->expr = expr(&empty);
        match(T_EOS);
        break;
    case T_OCBRACE:
        match(T_OCBRACE);
        while (!accept(T_CCBRACE))
        {
            st = parse_stmt();
            if (hp == NULL)
                hp = ep = st;
            else
            {
                ep->next = st;
                ep = st;
            }
        }
        st = hp;
        break;
    default:
        st = new_stnode(ST_EXPRESSION);
        st->expr = expr(&empty);
        match(T_EOS);
        break;
    }

    return st;
}

int main(int argc, char *argv[])
{
    FILE *fp = fopen("input.txt", "r");
    stnode_symbol *head_p = NULL, *tail_p = NULL, *st;
    if (argc > 1)
    {
        if ((fp = fopen(argv[1], "r")) == NULL)
        {
            perror("fopen");
            exit(1);
        }
    }
    startlex(fp);
    while (curr.sym != T_EOI)
    {
        st = parse_stmt();
        printf("\n");
        if (head_p == NULL)
            head_p = tail_p = st;
        else
        {
            tail_p->next = st;
            tail_p = st;
        }
    }

    while (head_p != NULL)
    {
        printf("%d ", head_p->type);
        head_p = head_p->next;
    }
}