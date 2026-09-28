#pragma once

/*
  This program is part of the TACLeBench benchmark suite.
  Version 2.0

  Name: filterbank

  Author: unknown

  Function: Creates a filter bank to perform multirate signal processing.
            The coefficients for the sets of filters are created in the
            top-level init function, and passed down through the init
            functions to FIR filter objects.
            On each branch, a delay, filter, and downsample is performed,
            followed by an upsample, delay, and filter.

  Source: StreamIt
          (http://groups.csail.mit.edu/cag/streamit/shtml/benchmarks.shtml)

  Original name: filterbank

  Changes: See ChangeLog.txt

  License: MIT License
*/

namespace Filterbank {

class Filterbank {
   public:
    Filterbank() { filterbank_numiters = 2; }

    ~Filterbank() = default;

    inline int run() {
        reset();

        float r[256];
        float y[256];
        float H[8][32];
        float F[8][32];

        int i, j;

        for (i = 0; i < 256; i++) r[i] = i + 1;

        for (i = 0; i < 32; i++) {
            for (j = 0; j < 8; j++) {
                H[j][i] = i * 32 + j * 8 + j + i + j + 1;
                F[j][i] = i * j + j * j + j + i;
            }
        }

        while (filterbank_numiters-- > 0) filterbank_core(r, y, H, F);

        filterbank_return_value = (int)(y[0]) - 9408;

        return filterbank_return_value;
    }

   private:
    inline void reset() { filterbank_numiters = 2; }

    void filterbank_core(float r[256], float y[256], float H[8][32], float F[8][32]) {
        int i, j, k;

        for (i = 0; i < 256; i++) y[i] = 0;

        for (i = 0; i < 8; i++) {
            float Vect_H[256];           /* (output of the H) */
            float Vect_Dn[(int)256 / 8]; /* output of the down sampler; */
            float Vect_Up[256];          /* output of the up sampler; */
            float Vect_F[256];           /* this is the output of the */

            /* convolving H */
            for (j = 0; j < 256; j++) {
                Vect_H[j] = 0;
                for (k = 0; ((k < 32) & ((j - k) >= 0)); k++) Vect_H[j] += H[i][k] * r[j - k];
            }

            /* Down Sampling */
            for (j = 0; j < 256 / 8; j++) Vect_Dn[j] = Vect_H[j * 8];

            /* Up Sampling */
            for (j = 0; j < 256; j++) Vect_Up[j] = 0;
            for (j = 0; j < 256 / 8; j++) Vect_Up[j * 8] = Vect_Dn[j];

            /* convolving F */
            for (j = 0; j < 256; j++) {
                Vect_F[j] = 0;
                for (k = 0; ((k < 32) & ((j - k) >= 0)); k++) Vect_F[j] += F[i][k] * Vect_Up[j - k];
            }

            /* adding the results to the y matrix */

            for (j = 0; j < 256; j++) y[j] += Vect_F[j];
        }
    }

   private:
    int filterbank_return_value;
    int filterbank_numiters;
};
}  // namespace Filterbank
