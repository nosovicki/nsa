#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if UNIX
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#endif


/* --- read proptotypes and typedef for ds_ssort --- */
#include "bwt/ds_ssort.h"
#include "dc2/dc2.h"
#include <math.h>

// ----- global variables -------
static int Verbose;
char *infile_name;
char *outfile_name;

// ----- prototypes ----------
typedef unsigned char UChar;
void fatal_error(char *);
void out_of_mem(char *f);

/* ***************************************************************
   this procedures demostrates the use of the ds_ssort routine
   It computes the suffix array for the content of Infile and use
   it to compute the Burrows-Wheeler Transform, which is then
   written to outfile.
   The original file can be retreived using the unbwt tool.
 *************************************************************** */
/********************* DC2 encoding format *****************
 * File: log2(Block_size), Block [, ...].
 * Block: [Last_block_size], Distance [, ...], Block_end.
 * Block_end: max(Block_size)+1, BWT_EOF_position.
 ***********************************************************/
/************************************************************
 *  bwt_eof_pos and last_block_size use mod.pos.  
 *  Last_block_size appears only in the last block and only 
 *  when there are more than one block. To provide this
 *  branching, all blocks except the first one start with
 *  'is_last' bit.
 ************************************************************/
void encode(FILE *infile, unsigned short bsize_log)
{
    int *sa, len, eof_pos=0, i;
    // ----- init ds suffix sort routine
    int overshoot=init_ds_ssort(500,2000);
    size_t block_size = (int) exp(log(2) * bsize_log);
    size_t n = block_size;
    if(overshoot==0)
        fatal_error("ds initialization failed! (encode)\n");
    // ----- allocate text and suffix array
    sa=malloc((n)*sizeof *sa);                     // suffix array
    UChar *text=malloc((n+overshoot)*sizeof *text);       // text
    unsigned char *compr1 = malloc(sizeof(char) * n + 4);
    unsigned char *compr2 = malloc(sizeof(char) * n + 256 * 256);
    if (! sa || ! text) out_of_mem("encode");
    // ----- read text and build suffix array
    ac_encoder enc;
    ac_encoder_init(&enc, outfile_name);
    int max_bsize_log = 30;
    ac_model_init(&mod.blocksize, max_bsize_log, NULL, MODEL_NONE);
    ac_model_init(&mod.blockstart, 2, NULL, MODEL_ADAPT);
    ac_model_init(&mod.distance, DC_BLOCK_SZ + 1, NULL, MODEL_ADAPT);
    ac_model_init(&mod.pos, AC_MAX, NULL, MODEL_ADAPT);
    ac_model_init(&mod.symb, 256, NULL, MODEL_ADAPT);
    ac_model_init(&mod.skip, 256, NULL, MODEL_ADAPT);
    // First comes the block size...
    ac_encode_symbol(&enc, &mod.blocksize, bsize_log);
    int is_first = 1;
    int tmp = 0;
    while (len = fread(text, sizeof(char), n, infile)) {
        size_t bpos = 0;
        ds_ssort(text,sa,len);                           // sort suffixes
        compr1[bpos++] = text[len - 1];             // L[0] = Text[len-1]
        for(i=0;i<len;i++) 
            if(sa[i]!=0)
                compr1[bpos++] = text[sa[i]-1];      // write char preceeding sa[i]
            else
                eof_pos = i;                      // store position of EOF
        if (len < n) { // the last block
            ac_encode_symbol(&enc, &mod.blockstart, 1);
            ac_encode_symbol_extralarge(&enc, &mod.pos, len);
        } else {
            ac_encode_symbol(&enc, &mod.blockstart, 0);
        }
        int outsz = 0;
        //fwrite(compr1, bpos, sizeof(char), fopen("check1", "w"));
        dc_encode(compr1, &enc, len, &outsz);
        ac_encode_symbol_extralarge(&enc, &mod.pos, eof_pos);
        is_first = 0;
    }
    ac_encoder_done(&enc);
    free(text);
    free(sa);
}

FILE *ensure_files()
{
    /* ------ open input and output files ------ */
    if(infile_name==NULL)
        fatal_error("Please provide the input file name (open_files)\n");
    else {
        FILE *infile = fopen( infile_name, "rb"); // b is for binary: required by DOS
        if(infile == NULL) {
            perror(infile_name);
            exit(1);
        }
        // ---- store input file length in Infile_size
        if(fseek(infile,0,SEEK_END)!=0) {
            perror(infile_name), exit(1);
        }
        infile_size=ftell(infile);
        if (infile_size==-1) {
            perror(infile_name), exit(1);
        } 
        if (infile_size==0) fatal_error("Input file empty (open_files)\n");
        rewind(infile); 
        // --- open output file
        if(outfile_name==NULL) {
            /* if outfile was not given add ".nsa" to infile_name */
            outfile_name = (char *) malloc(strlen(infile_name)+5);
            outfile_name = strcpy(outfile_name, infile_name);
            outfile_name = strcat(outfile_name, ".nsa");
        }
        FILE *outfile = fopen( outfile_name, "wb"); // b is for binary: required by DOS
        if(outfile==NULL) {
            perror(outfile_name);
            exit(1);
        }
        fclose(outfile);
        return infile;
    }
}

int main(int argc, char *argv[])
{  
    double getTime ( void );
    double end, start;
    int c;
    extern char *optarg;
    extern int optind, opterr, optopt;

    if(argc<2) {
        fprintf(stderr, "Usage:\n\t%s infile ",argv[0]);
        fprintf(stderr,"[-v][-o outfile] \n");
        fprintf(stderr,"\t-v           verbose output\n");
        fprintf(stderr,"\t-o outfile   output file\n");
        fprintf(stderr,"If no output file name is given default is ");
        fprintf(stderr,"{infile}.nsa\n\n");
        exit(0);
    }

    /* ------------- read command line options ----------- */
    Verbose=0;                       // be quiet by default 
    infile_name=outfile_name=NULL;
    opterr = 0;
    while ((c=getopt(argc, argv, "do:n:v")) != -1) {
        switch (c)
        {
            case 'o':
                outfile_name = optarg; break;
            case 'v':
                Verbose++; break;
            case '?':
                fprintf(stderr,"Unknown option: %c -main-\n", optopt);
                exit(1);
        }
    }
    if(optind<argc)
        infile_name=argv[optind];

    // ----- check input parameters--------------
    if(infile_name==NULL) fatal_error("The input file name is required\n");

    if(Verbose>2) {
        fprintf(stderr,"\n*****************************************************");
        fprintf(stderr,"\n             nsa  Ver 0.1\n");
        fprintf(stderr,"Created on %s at %s from %s\n",__DATE__,__TIME__,__FILE__);
        fprintf(stderr,"*****************************************************\n");
    }
    if(Verbose>1) {
        fprintf(stderr,"Command line: ");
        for(c=0;c<argc;c++)
            fprintf(stderr,"%s ",argv[c]);
        fprintf(stderr,"\n");
    }

    // ---- do the work ---------------------------
    start = getTime();
    FILE *infile = ensure_files();
    size_t bsize_log = 27; // 2 ^ 16 = 256 ^ 2
    encode(infile, bsize_log);
    end = getTime();
    if(Verbose)
        fprintf(stderr,"Elapsed time: %f seconds.\n", end-start);
    return 0;
}

void fatal_error(char *s)
{
    fprintf(stderr,"%s",s);
    exit(1);
}

void out_of_mem(char *f)
{
    fprintf(stderr, "Out of memory in function %s!\n", f);
    exit(1);
}

double getTime ( void )
{
#if UNIX
    double usertime,systime;
    struct rusage usage;

    getrusage ( RUSAGE_SELF, &usage );

    usertime = (double)usage.ru_utime.tv_sec +
        (double)usage.ru_utime.tv_usec / 1000000.0;

    systime = (double)usage.ru_stime.tv_sec +
        (double)usage.ru_stime.tv_usec / 1000000.0;

    return(usertime+systime);
#endif
    return 0;
}
