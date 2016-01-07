#ifndef AC_HEADER
#define AC_HEADER

#define MODEL_NONE 0
#define MODEL_ADAPT 1
#define MODEL_COUNT 2
#define MODEL_BWT 3

#define AC_MAX 1712 // 2 ^ 13

#include <stdio.h>

typedef struct {
  FILE *fp;
  long long low;
  long long high;
  long long fbits;
  int buffer;
  int bits_to_go;
  long long total_bits;
} ac_encoder;

typedef struct {
  FILE *fp;
  long long value;
  long long low;
  long long high;
  int buffer;
  int bits_to_go;
  int garbage_bits;
} ac_decoder;

typedef struct {
  int nsym;
  int *freq;
  int *cfreq;
  int adapt;
} ac_model;

void ac_encoder_init (ac_encoder *, const char *);
void ac_encoder_done (ac_encoder *);
void ac_decoder_init (ac_decoder *, const char *);
void ac_decoder_done (ac_decoder *);
void ac_model_init (ac_model *, int, int *, int);
void ac_model_done (ac_model *);
long long ac_encoder_bits (ac_encoder *);
void ac_encode_symbol (ac_encoder *, ac_model *, int);
int ac_decode_symbol (ac_decoder *, ac_model *);
void ac_encode_symbol_extralarge (ac_encoder *, ac_model *, int);
int ac_decode_symbol_extralarge (ac_decoder *, ac_model *);

#endif
