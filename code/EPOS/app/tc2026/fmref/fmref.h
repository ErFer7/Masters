#pragma once

/*
  fmref.c: C reference implementation of FM Radio
  David Maze <dmaze@cag.lcs.mit.edu>
  $Id: fmref.c,v 1.2 2010-10-04 21:21:26 garus Exp $
*/

#include "math_private.h"

namespace Fmref {
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

// Defines
#define SAMPLING_RATE 250000000
#define CUTOFF_FREQUENCY 108000000
#define NUM_TAPS 64
#define MAX_AMPLITUDE 27000.0
#define BANDWIDTH 10000
#define DECIMATION 4
/* Must be at least NUM_TAPS+1: */
#define IN_BUFFER_LEN 200
#define EQUALIZER_BANDS 10

// Type declarations
typedef struct FloatBuffer {
    float buff[IN_BUFFER_LEN];
    int rpos, rlen;
} FloatBuffer;
/* Low pass filter: */
typedef struct LPFData {
    float coeff[NUM_TAPS];
    float freq;
    int taps, decimation;
} LPFData;
typedef struct EqualizerData {
    LPFData lpf[EQUALIZER_BANDS + 1];
    FloatBuffer fb[EQUALIZER_BANDS + 1];
    float gain[EQUALIZER_BANDS];
} EqualizerData;

class Fmref {
   public:
    Fmref() = default;

    ~Fmref() = default;

    inline int run() {
        reset();

        int i;
        EqualizerData eq_data;

        fmref_fb1.rpos = fmref_fb1.rlen = 0;
        fmref_fb2.rpos = fmref_fb2.rlen = 0;
        fmref_fb3.rpos = fmref_fb3.rlen = 0;
        fmref_fb4.rpos = fmref_fb4.rlen = 0;

        fmref_init_lpf_data(&fmref_lpf_data, CUTOFF_FREQUENCY, NUM_TAPS, DECIMATION);
        fmref_init_equalizer(&eq_data);

        /* Startup: */
        fmref_get_floats(&fmref_fb1);
        /* LPF needs at least NUM_TAPS+1 inputs; fmref_get_floats is fine. */
        fmref_run_lpf(&fmref_fb1, &fmref_fb2, &fmref_lpf_data);
        /* run_demod needs 1 input, OK here. */
        /* run_equalizer needs 51 inputs (same reason as for LPF).  This means
           running the pipeline up to demod 50 times in advance: */
        for (i = 0; i < 64; i++) {
            if (fmref_fb1.rlen - fmref_fb1.rpos < NUM_TAPS + 1) fmref_get_floats(&fmref_fb1);
            fmref_run_lpf(&fmref_fb1, &fmref_fb2, &fmref_lpf_data);
            fmref_run_demod(&fmref_fb2, &fmref_fb3);
        }

        /* Main loop: */
        while (fmref_numiters-- > 0) {
            /* The low-pass filter will need NUM_TAPS+1 items; read them if we
               need to. */
            if (fmref_fb1.rlen - fmref_fb1.rpos < NUM_TAPS + 1) fmref_get_floats(&fmref_fb1);
            fmref_run_lpf(&fmref_fb1, &fmref_fb2, &fmref_lpf_data);
            fmref_run_demod(&fmref_fb2, &fmref_fb3);
            fmref_run_equalizer(&fmref_fb3, &fmref_fb4, &eq_data);
        }

        return 0;
    }

   private:
    inline void reset() {
        x = 0;
        fmref_numiters = 2;
    }

    void fmref_fb_compact(FloatBuffer *fb) {
        int i;
        char *source;
        char *target;
        target = (char *)(fb->buff);
        source = (char *)(fb->buff + fb->rpos);
        for (i = 0; i < fb->rlen - fb->rpos; i++) target[i] = source[i];
        fb->rlen -= fb->rpos;
        fb->rpos = 0;
    }

    int fmref_fb_ensure_writable(FloatBuffer *fb, int amount) {
        int available = IN_BUFFER_LEN - fb->rlen;
        if (available >= amount) return 1;

        /* Nope, not enough room, move current contents back to the beginning. */
        fmref_fb_compact(fb);

        available = IN_BUFFER_LEN - fb->rlen;
        if (available >= amount) return 1;

        return 0;
    }

    void fmref_get_floats(FloatBuffer *fb) {
        fmref_fb_compact(fb);

        /* Fill the remaining space in fb with 1.0. */
        while (fb->rlen < IN_BUFFER_LEN) {
            fb->buff[fb->rlen++] = (float)x;
            x++;
        }
    }

    void fmref_init_lpf_data(LPFData *data, float freq, int taps, int decimation) {
        /* Assume that CUTOFF_FREQUENCY is non-zero.  See comments in
           StreamIt LowPassFilter.java for origin. */
        float w = 2 * M_PI * freq / SAMPLING_RATE;
        int i;
        float m = taps - 1.0f;

        data->freq = freq;
        data->taps = taps;
        data->decimation = decimation;

        for (i = 0; i < taps; i++) {
            if (i - m / 2 == 0.0f)
                data->coeff[i] = w / M_PI;
            else
                data->coeff[i] = sin(w * (i - m / 2)) / M_PI / (i - m / 2) * (0.54f - 0.46f * cos(2 * M_PI * i / m));
        }
    }

    void fmref_run_lpf(FloatBuffer *fbin, FloatBuffer *fbout, LPFData *data) {
        float sum = 0.0f;
        int i = 0;

        for (i = 0; i < data->taps; i++) sum += fbin->buff[fbin->rpos + i] * data->coeff[i];

        fbin->rpos += data->decimation + 1;

        /* Check that there's room in the output buffer; move data if necessary. */
        fmref_fb_ensure_writable(fbout, 1);
        fbout->buff[fbout->rlen++] = sum;
    }

    void fmref_run_demod(FloatBuffer *fbin, FloatBuffer *fbout) {
        float temp, gain;
        gain = MAX_AMPLITUDE * SAMPLING_RATE / (BANDWIDTH * M_PI);
        temp = fbin->buff[fbin->rpos] * fbin->buff[fbin->rpos + 1];
        temp = gain * atan(temp);
        fbin->rpos++;
        fmref_fb_ensure_writable(fbout, 1);
        fbout->buff[fbout->rlen++] = temp;
    }

    void fmref_init_equalizer(EqualizerData *data) {
        int i;

        /* Equalizer structure: there are ten band-pass filters, with
           cutoffs as shown below.  The outputs of these filters get added
           together.  Each band-pass filter is LPF(high)-LPF(low). */
        for (i = 0; i < EQUALIZER_BANDS + 1; i++) fmref_init_lpf_data(&data->lpf[i], fmref_eq_cutoffs[i], 64, 0);

        /* Also initialize member buffers. */
        for (i = 0; i < EQUALIZER_BANDS + 1; i++) data->fb[i].rpos = data->fb[i].rlen = 0;

        for (i = 0; i < EQUALIZER_BANDS; i++) {
            // the gain amplifies the middle bands the most
            float val = (((float)i) - (((float)(EQUALIZER_BANDS - 1)) / 2.0f)) / 5.0f;
            data->gain[i] = val > 0 ? 2.0f - val : 2.0f + val;
        }
    }

    void fmref_run_equalizer(FloatBuffer *fbin, FloatBuffer *fbout, EqualizerData *data) {
        int i, rpos;
        float lpf_out[EQUALIZER_BANDS + 1];
        float sum = 0.0;

        /* Save the input read location; we can reuse the same input data on all
           of the LPFs. */
        rpos = fbin->rpos;

        /* Run the child filters. */
        for (i = 0; i < EQUALIZER_BANDS + 1; i++) {
            fbin->rpos = rpos;
            fmref_run_lpf(fbin, &data->fb[i], &data->lpf[i]);
            lpf_out[i] = data->fb[i].buff[data->fb[i].rpos++];
        }

        /* Now process the results of the filters.  Remember that each band is
           output(hi)-output(lo). */
        for (i = 0; i < EQUALIZER_BANDS; i++) sum += (lpf_out[i + 1] - lpf_out[i]) * data->gain[i];

        /* Write that result.  */
        fmref_fb_ensure_writable(fbout, 1);
        fbout->buff[fbout->rlen++] = sum;
    }

// Math
#ifdef __STDC__
    float cos(float x)
#else
    float cos(x) float x;
#endif
    {
        float y[2], z = 0.0f;
        int n, ix;

        GET_FLOAT_WORD(ix, x);

        /* |x| ~< pi/4 */
        ix &= 0x7fffffff;
        if (ix <= 0x3f490fd8) return fmref___kernel_cosf(x, z);

        /* cos(Inf or NaN) is NaN */
        else if (ix >= 0x7f800000)
            return x - x;

        /* argument reduction needed */
        else {
            n = fmref___ieee754_rem_pio2f(x, y);
            switch (n & 3) {
                case 0:
                    return fmref___kernel_cosf(y[0], y[1]);
                case 1:
                    return -fmref___kernel_sinf(y[0], y[1], 1);
                case 2:
                    return -fmref___kernel_cosf(y[0], y[1]);
                default:
                    return fmref___kernel_sinf(y[0], y[1], 1);
            }
        }
    }

#ifdef __STDC__
    float sin(float x)
#else
    float sin(x) float x;
#endif
    {
        float y[2], z = 0.0;
        int n, ix;

        GET_FLOAT_WORD(ix, x);

        /* |x| ~< pi/4 */
        ix &= 0x7fffffff;
        if (ix <= 0x3f490fd8) return fmref___kernel_sinf(x, z, 0);

        /* sin(Inf or NaN) is NaN */
        else if (ix >= 0x7f800000)
            return x - x;

        /* argument reduction needed */
        else {
            n = fmref___ieee754_rem_pio2f(x, y);
            switch (n & 3) {
                case 0:
                    return fmref___kernel_sinf(y[0], y[1], 1);
                case 1:
                    return fmref___kernel_cosf(y[0], y[1]);
                case 2:
                    return -fmref___kernel_sinf(y[0], y[1], 1);
                default:
                    return -fmref___kernel_cosf(y[0], y[1]);
            }
        }
    }

#ifdef __STDC__
    float atan(float x)
#else
    float atan(x) float x;
#endif
    {
        float w, s1, s2, z;
        int ix, hx, id;

        GET_FLOAT_WORD(hx, x);
        ix = hx & 0x7fffffff;
        if (ix >= 0x50800000) {                /* if |x| >= 2^34 */
            if (ix > 0x7f800000) return x + x; /* NaN */
            if (hx > 0)
                return fmref_atanhi[3] + fmref_atanlo[3];
            else
                return -fmref_atanhi[3] - fmref_atanlo[3];
        }
        if (ix < 0x3ee00000) {                            /* |x| < 0.4375 */
            if (ix < 0x31000000) {                        /* |x| < 2^-29 */
                if (fmref_huge + x > fmref_one) return x; /* raise inexact */
            }
            id = -1;
        } else {
            x = fabsf(x);
            if (ix < 0x3f980000) {     /* |x| < 1.1875 */
                if (ix < 0x3f300000) { /* 7/16 <=|x|<11/16 */
                    id = 0;
                    x = ((float)2.0f * x - fmref_one) / ((float)2.0f + x);
                } else { /* 11/16<=|x|< 19/16 */
                    id = 1;
                    x = (x - fmref_one) / (x + fmref_one);
                }
            } else {
                if (ix < 0x401c0000) { /* |x| < 2.4375 */
                    id = 2;
                    x = (x - (float)1.5f) / (fmref_one + (float)1.5f * x);
                } else { /* 2.4375 <= |x| < 2^66 */
                    id = 3;
                    x = -(float)1.0f / x;
                }
            }
        }
        /* end of argument reduction */
        z = x * x;
        w = z * z;
        /* break sum from i=0 to 10 aT[ i ]z**(i+1) into odd and even poly */
        s1 = z * (fmref_aT[0] +
                  w * (fmref_aT[2] + w * (fmref_aT[4] + w * (fmref_aT[6] + w * (fmref_aT[8] + w * fmref_aT[10])))));
        s2 = w * (fmref_aT[1] + w * (fmref_aT[3] + w * (fmref_aT[5] + w * (fmref_aT[7] + w * fmref_aT[9]))));
        if (id < 0)
            return x - x * (s1 + s2);
        else {
            z = fmref_atanhi[id] - ((x * (s1 + s2) - fmref_atanlo[id]) - x);
            return (hx < 0) ? -z : z;
        }
    }

#ifdef __STDC__
    float fmref___kernel_cosf(float x, float y)
#else
    float fmref___kernel_cosf(x, y) float x, y;
#endif
    {
        float a, hz, z, r, qx;
        int ix;
        GET_FLOAT_WORD(ix, x);
        ix &= 0x7fffffff;                        /* ix = |x|'s high word*/
        if (ix < 0x32000000) {                   /* if x < 2**27 */
            if (((int)x) == 0) return fmref_one; /* generate inexact */
        }
        z = x * x;
        r = z * (fmref_C1 + z * (fmref_C2 + z * (fmref_C3 + z * (fmref_C4 + z * (fmref_C5 + z * fmref_C6)))));
        if (ix < 0x3e99999a) /* if |x| < 0.3 */
            return fmref_one - ((float)0.5f * z - (z * r - x * y));
        else {
            if (ix > 0x3f480000) /* x > 0.78125 */
                qx = (float)0.28125f;

            else {
                SET_FLOAT_WORD(qx, ix - 0x01000000); /* x/4 */
            }
            hz = (float)0.5f * z - qx;
            a = fmref_one - qx;
            return a - (hz - (z * r - x * y));
        }
    }

#ifdef __STDC__
    int fmref___ieee754_rem_pio2f(float x, float *y)
#else
    int fmref___ieee754_rem_pio2f(x, y) float x, y[];
#endif
    {
        float z, w, t, r, fn;
        int i, j, n, ix, hx;

        GET_FLOAT_WORD(hx, x);
        ix = hx & 0x7fffffff;
        if (ix <= 0x3f490fd8) { /* |x| ~<= pi/4 , no need for reduction */
            y[0] = x;
            y[1] = 0;
            return 0;
        }
        if (ix < 0x4016cbe4) { /* |x| < 3pi/4, special case with n=+-1 */
            if (hx > 0) {
                z = x - fmref_pio2_1;
                if ((ix & 0xfffffff0) != 0x3fc90fd0) { /* 24+24 bit pi OK */
                    y[0] = z - fmref_pio2_1t;
                    y[1] = (z - y[0]) - fmref_pio2_1t;
                } else { /* near pi/2, use 24+24+24 bit pi */
                    z -= fmref_pio2_2;
                    y[0] = z - fmref_pio2_2t;
                    y[1] = (z - y[0]) - fmref_pio2_2t;
                }
                return 1;
            } else { /* negative x */
                z = x + fmref_pio2_1;
                if ((ix & 0xfffffff0) != 0x3fc90fd0) { /* 24+24 bit pi OK */
                    y[0] = z + fmref_pio2_1t;
                    y[1] = (z - y[0]) + fmref_pio2_1t;
                } else { /* near pi/2, use 24+24+24 bit pi */
                    z += fmref_pio2_2;
                    y[0] = z + fmref_pio2_2t;
                    y[1] = (z - y[0]) + fmref_pio2_2t;
                }
                return -1;
            }
        }
        if (ix <= 0x43490f80) { /* |x| ~<= 2^7*(pi/2), medium size */
            t = fabsf(x);
            n = (int)(t * fmref_invpio2 + fmref_half);
            fn = (float)n;
            r = t - fn * fmref_pio2_1;
            w = fn * fmref_pio2_1t; /* 1st round good to 40 bit */
            if (n < 32 && (int)(ix & 0xffffff00) != fmref_npio2_hw[n - 1]) {
                y[0] = r - w; /* quick check no cancellation */
            } else {
                unsigned int high;
                j = ix >> 23;
                y[0] = r - w;
                GET_FLOAT_WORD(high, y[0]);
                i = j - ((high >> 23) & 0xff);
                if (i > 8) { /* 2nd iteration needed, good to 57 */
                    t = r;
                    w = fn * fmref_pio2_2;
                    r = t - w;
                    w = fn * fmref_pio2_2t - ((t - r) - w);
                    y[0] = r - w;
                    GET_FLOAT_WORD(high, y[0]);
                    i = j - ((high >> 23) & 0xff);
                    if (i > 25) { /* 3rd iteration need, 74 bits acc */
                        t = r;    /* will cover all possible cases */
                        w = fn * fmref_pio2_3;
                        r = t - w;
                        w = fn * fmref_pio2_3t - ((t - r) - w);
                        y[0] = r - w;
                    }
                }
            }
            y[1] = (r - y[0]) - w;
            if (hx < 0) {
                y[0] = -y[0];
                y[1] = -y[1];
                return -n;
            } else
                return n;
        }
        /*
           all other (large) arguments
        */
        if (ix >= 0x7f800000) { /* x is inf or NaN */
            y[0] = y[1] = x - x;
            return 0;
        }

        y[0] = y[1] = x - x; /* dummy initialization */
        return 0;            /* doesn't happen for our input */
    }

#ifdef __STDC__
    float fmref___kernel_sinf(float x, float y, int iy)
#else
    float fmref___kernel_sinf(x, y, iy) float x, y;
    int iy; /* iy=0 if y is zero */
#endif
    {
        float z, r, v;
        int ix;
        GET_FLOAT_WORD(ix, x);
        ix &= 0x7fffffff;              /* high word of x */
        if (ix < 0x32000000) {         /* |x| < 2**-27 */
            if ((int)x == 0) return x; /* generate inexact */
        }
        z = x * x;
        v = z * x;
        r = fmref_S2 + z * (fmref_S3 + z * (fmref_S4 + z * (fmref_S5 + z * fmref_S6)));
        if (iy == 0)
            return x + v * (fmref_S1 + z * r);
        else
            return x - ((z * (fmref_half * y - v * r) - y) - v * fmref_S1);
    }

    /*
       fabsf(x) returns the absolute value of x.
    */

#ifdef __STDC__
    float fabsf(float x)
#else
    float fabsf(x) float x;
#endif
    {
        unsigned int ix;
        GET_FLOAT_WORD(ix, x);
        SET_FLOAT_WORD(x, ix & 0x7fffffff);
        return x;
    }

   private:
    int x;
    float fmref_lpf_coeff[NUM_TAPS];
    float fmref_eq_cutoffs[EQUALIZER_BANDS + 1] = {55.000004f,
                                                   77.78174f,
                                                   110.00001f,
                                                   155.56354f,
                                                   220.00002f,
                                                   311.12695f,
                                                   440.00003f,
                                                   622.25415f,
                                                   880.00006f,
                                                   1244.5078f,
                                                   1760.0001f};
    int fmref_numiters = 2;
    FloatBuffer fmref_fb1, fmref_fb2, fmref_fb3, fmref_fb4;
    LPFData fmref_lpf_data;

#ifdef __STDC__
    inline static const float fmref_atanhi[] = {
#else
    inline static float fmref_atanhi[] = {
#endif
        4.6364760399e-01f, /* atan(0.5)hi 0x3eed6338 */
        7.8539812565e-01f, /* atan(1.0)hi 0x3f490fda */
        9.8279368877e-01f, /* atan(1.5)hi 0x3f7b985e */
        1.5707962513e+00f, /* atan(inf)hi 0x3fc90fda */
    };

#ifdef __STDC__
    inline static const float fmref_atanlo[] = {
#else
    inline static float fmref_atanlo[] = {
#endif
        5.0121582440e-09f, /* atan(0.5)lo 0x31ac3769 */
        3.7748947079e-08f, /* atan(1.0)lo 0x33222168 */
        3.4473217170e-08f, /* atan(1.5)lo 0x33140fb4 */
        7.5497894159e-08f, /* atan(inf)lo 0x33a22168 */
    };

#ifdef __STDC__
    inline static const float fmref_aT[] = {
#else
    inline static float fmref_aT[] = {
#endif
        3.3333334327e-01f,  /* 0x3eaaaaaa */
        -2.0000000298e-01f, /* 0xbe4ccccd */
        1.4285714924e-01f,  /* 0x3e124925 */
        -1.1111110449e-01f, /* 0xbde38e38 */
        9.0908870101e-02f,  /* 0x3dba2e6e */
        -7.6918758452e-02f, /* 0xbd9d8795 */
        6.6610731184e-02f,  /* 0x3d886b35 */
        -5.8335702866e-02f, /* 0xbd6ef16b */
        4.9768779427e-02f,  /* 0x3d4bda59 */
        -3.6531571299e-02f, /* 0xbd15a221 */
        1.6285819933e-02f,  /* 0x3c8569d7 */
    };

#ifdef __STDC__
    inline static const float
#else
    inline static float
#endif
        /* one =  1.0000000000e+00, /\* 0x3f800000 *\/ */
        fmref_C1 = 4.1666667908e-02f,  /* 0x3d2aaaab */
        fmref_C2 = -1.3888889225e-03f, /* 0xbab60b61 */
        fmref_C3 = 2.4801587642e-05f,  /* 0x37d00d01 */
        fmref_C4 = -2.7557314297e-07f, /* 0xb493f27c */
        fmref_C5 = 2.0875723372e-09f,  /* 0x310f74f6 */
        fmref_C6 = -1.1359647598e-11f; /* 0xad47d74e */

#ifdef __STDC__
    inline static const float
#else
    inline static float
#endif
        /* zero =  0.0000000000e+00f, /\* 0x00000000 *\/ */
        /* half =  5.0000000000e-01f, /\* 0x3f000000 *\/ */
        /* two8 =  2.5600000000e+02f, /\* 0x43800000 *\/ */
        fmref_invpio2 = 6.3661980629e-01f, /* 0x3f22f984 */
        fmref_pio2_1 = 1.5707855225e+00f,  /* 0x3fc90f80 */
        fmref_pio2_1t = 1.0804334124e-05f, /* 0x37354443 */
        fmref_pio2_2 = 1.0804273188e-05f,  /* 0x37354400 */
        fmref_pio2_2t = 6.0770999344e-11f, /* 0x2e85a308 */
        fmref_pio2_3 = 6.0770943833e-11f,  /* 0x2e85a300 */
        fmref_pio2_3t = 6.1232342629e-17f; /* 0x248d3132 */

#ifdef __STDC__
    inline static const float
#else
    inline static float
#endif
        /* half =  5.0000000000e-01f,/\* 0x3f000000 *\/ */
        fmref_S1 = -1.6666667163e-01f, /* 0xbe2aaaab */
        fmref_S2 = 8.3333337680e-03f,  /* 0x3c088889 */
        fmref_S3 = -1.9841270114e-04f, /* 0xb9500d01 */
        fmref_S4 = 2.7557314297e-06f,  /* 0x3638ef1b */
        fmref_S5 = -2.5050759689e-08f, /* 0xb2d72f34 */
        fmref_S6 = 1.5896910177e-10f;  /* 0x2f2ec9d3 */

/* This array is like the one in e_rem_pio2.c, but the numbers are
   single precision and the last 8 bits are forced to 0.  */
#ifdef __STDC__
    inline static const int fmref_npio2_hw[] = {
#else
    inline static int fmref_npio2_hw[] = {
#endif
        0x3fc90f00, 0x40490f00, 0x4096cb00, 0x40c90f00, 0x40fb5300, 0x4116cb00, 0x412fed00, 0x41490f00,
        0x41623100, 0x417b5300, 0x418a3a00, 0x4196cb00, 0x41a35c00, 0x41afed00, 0x41bc7e00, 0x41c90f00,
        0x41d5a000, 0x41e23100, 0x41eec200, 0x41fb5300, 0x4203f200, 0x420a3a00, 0x42108300, 0x4216cb00,
        0x421d1400, 0x42235c00, 0x4229a500, 0x422fed00, 0x42363600, 0x423c7e00, 0x4242c700, 0x42490f00};

    // Often used variables/consts
#ifdef __STDC__
    inline static const float
#else
    inline static float
#endif
        fmref_one = 1.0f,
        fmref_half = 5.0000000000e-01f,                                         /* 0x3f000000 */
        fmref_zero = 0.0f, fmref_huge = 1.0e30, fmref_two8 = 2.5600000000e+02f, /* 0x43800000 */
        fmref_twon8 = 3.9062500000e-03f;                                        /* 0x3b800000 */
};
}  // namespace Fmref
