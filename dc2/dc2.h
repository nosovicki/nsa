#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../hash/sprite.h"
#include "../hash/hash.h"
#include "../ac/ac.h"

#define DC_BLOCK_SZ 1712

void dc_encode(unsigned char *inbuf, ac_encoder *enc, size_t len, int *outsz);
int dc_decode(ac_decoder *decoder, unsigned char *outbuf, size_t outsz);
void show(char *buf, size_t len);

long infile_size;

struct ac_models {
    ac_model blocksize, distance, blockstart, pos, symb, skip;
} mod;

