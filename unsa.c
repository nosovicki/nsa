#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <assert.h>
#include <math.h>

#include "dc2/dc2.h"
#include "ac/ac.h"

typedef unsigned char UChar;
// ---- struct containing the (uncompressed) bwt 
typedef struct {
    UChar *bwt;
    size_t size;
    size_t eof_pos;
} bwt_data;

// ----- global variables -------
int verbose = 0;         // default: quiet mode
char *outfile_name; 
char *infile_name; 

// ----- prototypes ----------
void fatal_error(char *);
void out_of_mem(char *f);

void print_plate() {
    fprintf(stderr,"\n Not a State of the Art decoder 1.0\n");
    fprintf(stderr,"Compiled on %s at %s from %s\n",__DATE__,__TIME__,__FILE__);
}

FILE *ensure_files()
{
    if (infile_name==NULL)
        fatal_error("Please provide compressed file name (open_files)\n");
    FILE *fp = fopen(infile_name, "rb");
    if (fseek(fp,0,SEEK_END)!=0) {
        perror(infile_name); exit(2);
    }
    infile_size = ftell(fp);
    if (infile_size == -1) perror(infile_name), exit(3);
    if (infile_size == 0) fatal_error("Input file is empty (open_files)\n");
    fclose(fp);

    if (outfile_name == NULL) {
        /* if outfile was not given strip ".nsa" from infile_name */
        char *p = strstr(infile_name, ".nsa");
        if (NULL == p || strlen(p) != 4)
            fatal_error("Cannot strip '.nsa' from input file name");
        outfile_name = (char *) malloc(strlen(infile_name)-3);
        outfile_name = strncpy(outfile_name, infile_name, strlen(infile_name) - 4);
    }
    return fopen(outfile_name, "wb");
}

/* ***************************************************************
   Tool for inverting the bwt computed by the ds suffix sorter 
 *************************************************************** */
int main(int argc, char *argv[])
{  
    double getTime ( void );
    void unbwt_file(void);
    void decode(FILE *outfile, bwt_data *b);
    double end, start;
    int c;
    extern char *optarg;
    extern int optind, opterr, optopt;
    bwt_data bb, *b=&bb;

    if (argc<2) {
        fprintf(stderr, "Usage:\n\t%s infile ",argv[0]);
        fprintf(stderr,"[-v][-o outfile] \n");
        fprintf(stderr,"\t-v           verbose output\n");
        fprintf(stderr,"\t-o outfile   output file\n");
        fprintf(stderr,"If no output file name is given default is ");
        fprintf(stderr,"{infile}.y\n\n");
        exit(0);
    }

    /* ------------- read command line options ----------- */
    verbose = 0;
    infile_name = outfile_name = NULL;
    opterr = 0;
    while ((c = getopt(argc, argv, "do:n:v")) != -1) {
        switch (c)
        {
            case 'o': outfile_name = optarg; break;
            case 'v': ++verbose; break;
            case '?': fprintf(stderr,"Unknown option: %c -main-\n", optopt), exit(4);
        }
    }
    if (optind<argc)
        infile_name = argv[optind];
    if (verbose>2)
        print_plate();

    FILE *outfile = ensure_files();
    start = getTime();
    decode(outfile, b);
    end = getTime();
    if (verbose) fprintf(stderr,"Elapsed time: %f seconds.\n", end-start);
    return 0;
}


// use bwt and rank_next map to compute the inverse
// bwt and write it to f 
void write_inverse_bwt_rn(bwt_data *b, int *rank_next, FILE *f)
{
    int i,j, written = 0;
    // in the following, i is an index in the last column (the bwt)
    // and j is an index in the first column
    j = b->eof_pos;   // this is the position of the first text char 
    while(1) {      
        // get last column index corresponding to j
        i = rank_next[j];
        assert(i>=0 && i<b->size);
        putc(b->bwt[i],f);  // output char
        if (++written==b->size) break;  // if all chars have been written stop
        // now we move to the first column staying in the same row
        // hence we get j such that f[j] is the character following l[i]
        if (i<=b->eof_pos) j=i-1;
        else j=i;
    }
    // check that the last written char was bwt[0]
    if (i!=0) 
        fatal_error("Error writing inverse bwt (write_inverse_bwt_rn)\n");  
}

// compute the rank_next map  and store it into rn[] 
// which must be an already allocated array of size b->size 
// rn[i] is the position in the bwt (the last column of the BWT matrix)
// of the character f[i] (the first column of the BWT matrix)
// ignoring the eof symbol
void compute_rn_map(bwt_data *b, int *rn)
{
    int i, occ[256];
    // clear occ
    for(i=0;i<256;i++) occ[i]=0;
    // compute occ
    for(i=0;i<b->size;i++)
        occ[b->bwt[i]]++;
    // accumulate occ
    for(i=1;i<256;i++)
        occ[i] += occ[i-1];
    // shift occ
    assert(occ[255]==b->size);
    for(i=255;i>0;i--)
        occ[i] = occ[i-1];
    occ[0]=0;
    /* ----- compute rank_next mapping -------- */
    assert(rn!=NULL);
    for(i=0;i<b->size;i++)
        rn[occ[b->bwt[i]]++] = i;
}

inline int min(int a, int b) {return a<b?a:b;}

// read bwt from file fp and store it into b
void decode(FILE *outfile, bwt_data *b)
{
    int i,c,fsize;
    int max_bsize_log = 30;
    // bwt_eof_pos and last_block_size use mod.pos, 
    ac_decoder decoder;
    ac_decoder_init(&decoder, infile_name);
    ac_model_init(&mod.blocksize, max_bsize_log, NULL, MODEL_NONE);
    short bsize_log = ac_decode_symbol(&decoder, &mod.blocksize);
    if (bsize_log < 1 || bsize_log > max_bsize_log) fatal_error("Wrong file format");
    size_t block_size = (int) exp(log(2) * bsize_log);
    ac_model_init(&mod.blockstart, 2, NULL, MODEL_ADAPT);
    ac_model_init(&mod.distance, DC_BLOCK_SZ + 1, NULL, MODEL_ADAPT);
    ac_model_init(&mod.pos, AC_MAX, NULL, MODEL_ADAPT);
    ac_model_init(&mod.symb, 256, NULL, MODEL_ADAPT);
    ac_model_init(&mod.skip, 256, NULL, MODEL_ADAPT);
    if (NULL == (b->bwt = malloc(block_size * sizeof(char)))) out_of_mem("decode");
    int *rank_next = malloc(block_size * sizeof(int *));
    if (rank_next==NULL)  out_of_mem("unbwt_file");
    //while (fread(b->bwt, sizeof(char), block_size, fp)) {
    unsigned char last, cur;
    int tmp = 0;
    while (1) {
        int is_last = ac_decode_symbol(&decoder, &mod.blockstart);
        if (is_last)
            block_size = ac_decode_symbol_extralarge(&decoder, &mod.pos);
        dc_decode(&decoder, b->bwt, block_size);
        b->size = block_size;
        int i = 0;
        while (i < b->size) {
            cur = b->bwt[i];
            if (cur)
                last = cur;
            else
                cur = last;
            b->bwt[i++] = cur;
        }
        //fwrite(b->bwt, block_size, 1, fopen("check2", "w"));
        //show(b->bwt, b->size);
        b->eof_pos = ac_decode_symbol_extralarge(&decoder, &mod.pos);
        //if (++tmp >0) fprintf(stderr, "*** %d %d %d %d ***\n",tmp,block_size, mod.pos.cfreq[0], b->eof_pos);
        if (b->eof_pos<0 || b->eof_pos>=b->size) 
            fatal_error("Illegal eof in bwt file (decode)\n");
        if (verbose>1) fprintf(stderr,"bwt successfully read!\n");
        compute_rn_map(b, rank_next);
        if (verbose>1) fprintf(stderr,"rank_next map computed!\n");
        write_inverse_bwt_rn(b, rank_next, outfile);
        if (ftell(decoder.fp) == infile_size) break; // End of file
    }
    ac_decoder_done(&decoder);
    free(rank_next);
    free(b->bwt);
}



void fatal_error(char *s)
{
    fprintf(stderr,"%s\n",s);
    exit(5);
}

void out_of_mem(char *f)
{
    fprintf(stderr, "Out of memory in function %s!\n", f);
    exit(6);
}

double getTime ( void )
{
    double usertime,systime;
    struct rusage usage;

    getrusage ( RUSAGE_SELF, &usage );

    usertime = (double)usage.ru_utime.tv_sec +
        (double)usage.ru_utime.tv_usec / 1000000.0;

    systime = (double)usage.ru_stime.tv_sec +
        (double)usage.ru_stime.tv_usec / 1000000.0;

    return(usertime+systime);
}

