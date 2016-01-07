#include "dc2.h"

struct ls {
    size_t fr;
    struct ls *cdr;
};
typedef struct ls ls;

struct alpha {
    unsigned char c;
    size_t last_offset;
    struct alpha *next;
    struct ls *head;
    struct ls *tail;
};
typedef struct alpha alpha;

void add_offset(alpha *a, size_t fr) {
    //fprintf(stderr, "*** %d - %d ***\n", a->c, fr);
    ls *off = malloc(sizeof(ls));
    off->fr = fr;
    off->cdr = NULL;
    if (a->head == NULL)
        a->head = off;
    else
        a->tail->cdr = off;
    a->tail = off;
}

alpha *count_runs(unsigned char *buf, size_t len, size_t *outsize) {
    int sz = 0;
    int i = 0;
    // Represent alphabet as a cyclic list
    alpha *runcount = calloc(256, sizeof(alpha));
    while (i < 255) {
        ++i;
        runcount[i-1].next = &runcount[i];
        runcount[i].c = i;
    }
    runcount[i].next = &runcount[0];
    // Fill offsets
    unsigned char a, b;
    a = buf[0];
    int fr = 0;
    for (i = 1; i < len; i++) {
        b = buf[i];
        if (a != b) {
            add_offset(&runcount[a], fr);
            ++sz;
            a = b;
            fr = i;
        }
    }
    add_offset(&runcount[a], fr);
    (*outsize) = sz + 1 + 256;
    return runcount;
}

void dc_encode_subblock(unsigned char *inbuf, ac_encoder *enc, size_t len, int *outsz) {
    size_t compressed_size;
    alpha *runcount = count_runs(inbuf, len, &compressed_size);
    int i = 0;
    alpha *prev = NULL;
    alpha *a = (&runcount[(unsigned char)inbuf[0]]);
    int last_pos = 0;
    while (a) {
        //fprintf(stderr, "*** %c %d ***", a->c, a->c);
        if (a->head == NULL) {
            if (a->next == a) // last alpha
                break;
            ac_encode_symbol(enc, &mod.distance, 0), ++i;
            int num = 0;
            while (a->next != a && a->head == NULL) {
                prev->next = a = a->next;
                ++num;
            }
            ac_encode_symbol(enc, &mod.skip, num);
            //fprintf(stderr, " X ***\n", a->c, a->c);
        } else {
            ls *offset = a->head;
            //fprintf(stderr, " %d ***\n",offset->fr - a->last_offset);
            if (!offset->fr)
                ac_encode_symbol(enc, &mod.symb, inbuf[0]), ++i;
            else {
                int distance = offset->fr - a->last_offset;
                ac_encode_symbol(enc, &mod.distance, distance), ++i;
            }
            a->last_offset = offset->fr;
            a->head = offset->cdr;
            free(offset);
            compressed_size = i;
            prev = a;
            a = a->next;
        }
    }
    ac_encode_symbol(enc, &mod.distance, len);
    (*outsz) = compressed_size + 1;;
    free(runcount);
}

void dc_encode(unsigned char *inbuf, ac_encoder *enc, size_t remains, int *outsz) {
    int sz;
    while (remains > DC_BLOCK_SZ) {
        dc_encode_subblock(inbuf, enc, DC_BLOCK_SZ, &sz);
        remains -= DC_BLOCK_SZ;
        inbuf += DC_BLOCK_SZ;
        (*outsz) += sz;
    }
    dc_encode_subblock(inbuf, enc, remains, &sz);
    (*outsz) += sz;
}
