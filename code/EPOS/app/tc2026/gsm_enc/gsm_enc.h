#ifndef PRIVATE_H
#define PRIVATE_H

namespace GsmEnc {

typedef short gsm_word;   /* 16 bit signed int  */
typedef int longgsm_word; /* 32 bit signed int  */

typedef unsigned short ugsm_word;   /* unsigned gsm_word  */
typedef unsigned int ulonggsm_word; /* unsigned longgsm_word  */

struct gsm_state {
    gsm_word dp0[280];

    gsm_word z1;       /* preprocessing.c, Offset_com. */
    longgsm_word L_z2; /*                  Offset_com. */
    int mp;            /*                  Preemphasis */

    gsm_word u[8];        /* short_term_aly_filter.c  */
    gsm_word LARpp[2][8]; /*                              */
    gsm_word j;           /*                              */

    gsm_word nrp; /* 40 */ /* long_term.c, synthesis */
    gsm_word v[9];         /* short_term.c, synthesis  */
    gsm_word msr;          /* decoder.c, Postprocessing  */

    char verbose; /* only used if !NDEBUG   */
    char fast;    /* only used if FAST    */
};

#define MIN_WORD ((-32767) - 1)
#define MAX_WORD (32767)

#define MIN_LONGWORD ((-2147483647) - 1)
#define MAX_LONGWORD (2147483647)

#define SASR(x, by) ((x) >> (by))

typedef struct gsm_state *gsm;
typedef short gsm_signal; /* signed 16 bit */
typedef unsigned char gsm_byte;
typedef gsm_byte gsm_frame[33]; /* 33 * 8 bits   */

#define GSM_MAGIC 0xD /* 13 kbit/s RPE-LTP */

#define GSM_PATCHLEVEL 6
#define GSM_MINOR 0
#define GSM_MAJOR 1
#define SAMPLES 20

class GsmEnc {
   public:
    GsmEnc();

    ~GsmEnc();

    int run();

   private:
    inline void reset() { gsm_enc_state_ptr = gsm_enc_create(); }

    gsm_word gsm_enc_div(gsm_word num, gsm_word denum);

    gsm_word gsm_enc_sub(gsm_word a, gsm_word b);

    gsm_word gsm_enc_norm(longgsm_word a);

    gsm_word gsm_enc_asl(gsm_word a, int n);

    gsm_word gsm_enc_asr(gsm_word a, int n);

    void gsm_enc_Gsm_Coder(struct gsm_state *S,
                           gsm_word *s,     /* [ 0..159 ] samples   IN  */
                           gsm_word *LARc,  /* [ 0..7 ] LAR coefficients  OUT */
                           gsm_word *Nc,    /* [ 0..3 ] LTP lag   OUT   */
                           gsm_word *bc,    /* [ 0..3 ] coded LTP gain  OUT   */
                           gsm_word *Mc,    /* [ 0..3 ] RPE grid selection  OUT     */
                           gsm_word *xmaxc, /* [ 0..3 ] Coded maximum amplitude OUT */
                           gsm_word *xMc /* [ 13*4 ] normalized RPE samples OUT  */);

    void gsm_enc_Gsm_Long_Term_Predictor(               /* 4x for 160 samples */
                                         gsm_word *d,   /* [ 0..39 ]   residual signal  IN  */
                                         gsm_word *dp,  /* [ -120..-1 ] d'    IN  */
                                         gsm_word *e,   /* [ 0..40 ]      OUT */
                                         gsm_word *dpp, /* [ 0..40 ]      OUT */
                                         gsm_word *Nc,  /* correlation lag    OUT */
                                         gsm_word *bc /* gain factor      OUT */);

    void gsm_enc_Gsm_LPC_Analysis(gsm_word *s,     /* 0..159 signals  IN/OUT  */
                                  gsm_word *LARc); /* 0..7   LARc's  OUT */

    void gsm_enc_Gsm_Preprocess(struct gsm_state *S, gsm_word *s, gsm_word *so);

    void gsm_enc_Gsm_Short_Term_Analysis_Filter(struct gsm_state *S,
                                                gsm_word *LARc, /* coded log area ratio [ 0..7 ]  IN  */
                                                gsm_word *d /* st res. signal [ 0..159 ]  IN/OUT  */);

    void gsm_enc_Gsm_RPE_Encoding(gsm_word *e,     /* -5..-1 ][ 0..39 ][ 40..44     IN/OUT  */
                                  gsm_word *xmaxc, /*                              OUT */
                                  gsm_word *Mc,    /*                              OUT */
                                  gsm_word *xMc);  /* [ 0..12 ]                      OUT */

    void gsm_enc_encode(gsm, gsm_signal *, gsm_byte *);

    int gsm_enc_explode(gsm, gsm_byte *, gsm_signal *);
    void gsm_enc_implode(gsm, gsm_signal *, gsm_byte *);

    void gsm_enc_Weighting_filter(gsm_word *e, /* signal [ -5..0.39.44 ] IN  */
                                  gsm_word *x  /* signal [ 0..39 ] OUT */
    );

    void gsm_enc_RPE_grid_selection(gsm_word *x,     /* [ 0..39 ]    IN  */
                                    gsm_word *xM,    /* [ 0..12 ]    OUT */
                                    gsm_word *Mc_out /*      OUT */
    );

    void gsm_enc_APCM_quantization_xmaxc_to_exp_mant(gsm_word xmaxc,      /* IN   */
                                                     gsm_word *exp_out,   /* OUT  */
                                                     gsm_word *mant_out); /* OUT  */

    void gsm_enc_APCM_quantization(gsm_word *xM, /* [ 0..12 ]    IN  */

                                   gsm_word *xMc,      /* [ 0..12 ]    OUT */
                                   gsm_word *mant_out, /*      OUT */
                                   gsm_word *exp_out,  /*      OUT */
                                   gsm_word *xmaxc_out /*      OUT */
    );

    void gsm_enc_APCM_inverse_quantization(gsm_word *xMc, /* [ 0..12 ]      IN  */
                                           gsm_word mant,
                                           gsm_word exp,
                                           gsm_word *xMp); /* [ 0..12 ]      OUT   */

    void gsm_enc_RPE_grid_positioning(gsm_word Mc,   /* grid position  IN  */
                                      gsm_word *xMp, /* [ 0..12 ]    IN  */
                                      gsm_word *ep   /* [ 0..39 ]    OUT */
    );

    void gsm_enc_Calculation_of_the_LTP_parameters(gsm_word *d,      /* [ 0..39 ]  IN  */
                                                   gsm_word *dp,     /* [ -120..-1 ] IN  */
                                                   gsm_word *bc_out, /*    OUT */
                                                   gsm_word *Nc_out  /*    OUT */
    );

    void gsm_enc_Long_term_analysis_filtering(gsm_word bc,   /*          IN  */
                                              gsm_word Nc,   /*          IN  */
                                              gsm_word *dp,  /* previous d [ -120..-1 ]    IN  */
                                              gsm_word *d,   /* d    [ 0..39 ]     IN  */
                                              gsm_word *dpp, /* estimate [ 0..39 ]     OUT */
                                              gsm_word *e    /* long term res. signal [ 0..39 ]  OUT */
    );

    void gsm_enc_Decoding_of_the_coded_Log_Area_Ratios(gsm_word *LARc,   /* coded log area ratio [ 0..7 ]  IN  */
                                                       gsm_word *LARpp); /* out: decoded ..      */

    void gsm_enc_Coefficients_0_12(gsm_word *LARpp_j_1, gsm_word *LARpp_j, gsm_word *LARp);

    void gsm_enc_Coefficients_13_26(gsm_word *LARpp_j_1, gsm_word *LARpp_j, gsm_word *LARp);

    void gsm_enc_Coefficients_27_39(gsm_word *LARpp_j_1, gsm_word *LARpp_j, gsm_word *LARp);

    void gsm_enc_Coefficients_40_159(gsm_word *LARpp_j, gsm_word *LARp);

    void gsm_enc_LARp_to_rp(gsm_word *LARp); /* [ 0..7 ] IN/OUT  */

    void gsm_enc_Short_term_analysis_filtering(struct gsm_state *S,
                                               gsm_word *rp, /* [ 0..7 ] IN  */
                                               int k_n,      /*   k_end - k_start  */
                                               gsm_word *s   /* [ 0..n-1 ] IN/OUT  */
    );

    void gsm_enc_Autocorrelation(gsm_word *s,          /* [ 0..159 ] IN/OUT  */
                                 longgsm_word *L_ACF); /* [ 0..8 ] OUT     */

    void gsm_enc_Reflection_coefficients(longgsm_word *L_ACF, /* 0...8  IN  */
                                         gsm_word *r          /* 0...7  OUT   */
    );

    void gsm_enc_Transformation_to_Log_Area_Ratios(gsm_word *r /* 0..7    IN/OUT */
    );

    void gsm_enc_Quantization_and_coding(gsm_word *LAR /* [ 0..7 ] IN/OUT  */
    );

    gsm gsm_enc_create(void);

   private:
    gsm_state gsm_enc_state;
    gsm gsm_enc_state_ptr;
    volatile int gsm_enc_result;

    /*   Table 4.3a  Decision level of the LTP gain quantizer
     */
    /*  bc          0         1   2      3      */
    inline static const gsm_word gsm_enc_DLB[4] = {6554, 16384, 26214, 32767};

    /*   Table 4.5   Normalized inverse mantissa used to compute xM/xmax
     */
    /* i      0        1    2      3      4      5     6      7   */
    inline static const gsm_word gsm_enc_NRFAC[8] = {29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384};

    /*   Table 4.6   Normalized direct mantissa used to compute xM/xmax
     */
    /* i                  0      1       2      3      4      5      6      7   */
    inline static const gsm_word gsm_enc_FAC[8] = {18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767};

    static const unsigned char gsm_enc_bitoff[256];

    gsm_signal *gsm_enc_pcmdata;

    static const gsm_signal gsm_enc_pcmdata_const[];

    static const size_t gsm_enc_pcmdata_const_size;

    gsm_byte *gsm_enc_gsmdata;

    static const gsm_byte gsm_enc_gsmdata_const[];

    static const size_t gsm_enc_gsmdata_const_size;
};
}  // namespace GsmEnc

#endif /* PRIVATE_H */
