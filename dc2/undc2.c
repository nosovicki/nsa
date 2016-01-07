#include "dc2.h"

struct alpha {
    unsigned char c;
    size_t last_offset;
    struct alpha *next;
};
typedef struct alpha alpha;

void show(char *buf, size_t len) {
    fputc('\n', stderr);
    while (len-- > 0)
        fputc(buf[len]? buf[len]: '.', stderr);
    fputc('\n', stderr);
}

int dc_decode_subblock(ac_decoder *dec, unsigned char *outbuf, size_t outsz) {
    /***********************************************************
     * File: log2(Block_size), (Block [, ...])                 *
     * Block: ([Last_block_size], Distance [, ...], Block_end) *
     * Block_end: Block_size, BWT_EOF_position          *
     ***********************************************************/
    /************************************************************
     *  bwt_eof_pos and last_block_size use mod.pos.            *
     *  Last_block_size appears only in the last block and only *
     *  when there are more than one block.                     *
     ************************************************************/
    int x, i;
    alpha bet[256];
    alpha *a, *prev;
    memset(outbuf, 0, outsz);
    // initialize alphabet
    for (i = 0; i < 255; i++) {
        bet[i].next = &bet[i+1];
        bet[i].c = i;
        bet[i].last_offset = 0;
    }
    bet[255].next = &bet[0];
    bet[255].c = 255;
    bet[255].last_offset = 0;
    x = ac_decode_symbol(dec, &mod.symb);
    outbuf[0] = x;
    prev = &(bet[x]);
    a = prev->next;
    while (1) {
        //show(outbuf, outsz);
        if (infile_size == ftell(dec->fp)) return 0;
        x = ac_decode_symbol(dec, &mod.distance);
        if (x == outsz)
            return 1; //end of compressed block
        else if (x == 0) {
            //fprintf(stderr, "!%c! ", a->c);
            int num = ac_decode_symbol(dec, &mod.skip);
            while (num-- > 0) {
                prev->next = a = a->next; 
            }
        } else {
            int pos = x + a->last_offset;
            //fprintf(stderr, " '%c' %d ", a->c, pos);
            outbuf[pos] = a->c;
            a->last_offset = pos;
            prev = a;
            a = a->next;
        }
    }
}

int dc_decode(ac_decoder *dec, unsigned char *outbuf, size_t remains) {
    /***********************************************************
     * File: log2(Block_size), (Block [, ...])                 *
     * Block: ([Last_block_size], Distance [, ...], Block_end) *
     * Block_end: Block_size, BWT_EOF_position          *
     ***********************************************************/
    /************************************************************
     *  bwt_eof_pos and last_block_size use mod.pos.            *
     *  Last_block_size appears only in the last block and only *
     *  when there are more than one block.                     *
     ************************************************************/
    while (remains > DC_BLOCK_SZ) {
        dc_decode_subblock(dec, outbuf, DC_BLOCK_SZ);
        outbuf += DC_BLOCK_SZ;
        remains -= DC_BLOCK_SZ;
    }
    dc_decode_subblock(dec, outbuf, remains);
}
