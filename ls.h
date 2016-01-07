struct ls {
    unsigned char car;
    struct ls *cdr;
};
typedef struct ls ls;

ls *cons(char c, ls *b);

