/*
  This program is part of the TACLeBench benchmark suite.
  Version V 2.0

  Name: quicksort

  Author: Matthew R. Guthaus

  Function: quicksort applies a recursive quicksort algorithm to two different
    input sets.

  Source: MiBench
          http://wwweb.eecs.umich.edu/mibench

  Original name: qsort

  Changes: No major functional changes.

  License: GPL

*/

/*
  Include section
*/

#include "quicksort.h"

#include "quicksortlibm.h"
#include "quicksortstdlib.h"

namespace Quicksort {
/*
  Initialization- and return-value-related functions
*/

Quicksort::Quicksort() {
    unsigned int j;
    unsigned int x, y, z;
    unsigned int read_counter = 0;

    for (unsigned int i = 0; i < 3000; i++) {
        quicksort_input_vector[i] = quicksort_input_vector_data[i];
    }

    /* constant propagation border */
    for (unsigned int i = 0; i < 3000; i++) quicksort_input_vector[i] += quicksort_const_prop_border_i;

    /* Init arrays */
    for (unsigned int i = 0; i < 681; i++) {
        for (j = 0; j < 20 - 1; j++) {
            quicksort_strings[i][j] = quicksort_input_string[i][j];
            quicksort_strings[i][j] += quicksort_const_prop_border_c;

            if (quicksort_input_string[i][j] == '\0') break;
        }

        /* Terminate with '\0' anyways. */
        quicksort_strings[i][20 - 1] = '\0';
    }

    for (unsigned int i = 0; i < 1000; i++) {
        x = quicksort_vectors[i].x = quicksort_input_vector[read_counter++];
        y = quicksort_vectors[i].y = quicksort_input_vector[read_counter++];
        z = quicksort_vectors[i].z = quicksort_input_vector[read_counter++];

        quicksort_vectors[i].distance = quicksort_sqrt(quicksort_pow(x, 2) + quicksort_pow(y, 2) + quicksort_pow(z, 2));
    }
}

int Quicksort::run() {
    reset();

    quicksort_str(*quicksort_strings, 681, sizeof(char[20]));

    quicksort_vec((char *)quicksort_vectors, 1000, sizeof(struct quicksort_3DVertexStruct));

    int checksum = 0;

    checksum += quicksort_strings[42][1] + quicksort_vectors[42].x + quicksort_vectors[42].y + quicksort_vectors[42].z;

    return checksum - 1527923179 != 0;
}

void Quicksort::reset() {
    unsigned int j;
    unsigned int x, y, z;
    unsigned int read_counter = 0;

    for (unsigned int i = 0; i < 3000; i++) quicksort_input_vector[i] = quicksort_input_vector_data[i];
    for (unsigned int i = 0; i < 3000; i++) quicksort_input_vector[i] += quicksort_const_prop_border_i;

    for (unsigned int i = 0; i < 681; i++) {
        for (j = 0; j < 20 - 1; j++) {
            quicksort_strings[i][j] = quicksort_input_string[i][j];
            quicksort_strings[i][j] += quicksort_const_prop_border_c;
            if (quicksort_input_string[i][j] == '\0') break;
        }
        quicksort_strings[i][20 - 1] = '\0';
    }

    for (unsigned int i = 0; i < 1000; i++) {
        x = quicksort_vectors[i].x = quicksort_input_vector[read_counter++];
        y = quicksort_vectors[i].y = quicksort_input_vector[read_counter++];
        z = quicksort_vectors[i].z = quicksort_input_vector[read_counter++];
        quicksort_vectors[i].distance = quicksort_sqrt(quicksort_pow(x, 2) + quicksort_pow(y, 2) + quicksort_pow(z, 2));
    }
}

/*
  Algorithm core functions
*/

void Quicksort::quicksort_str(char *a, unsigned long n, unsigned long es) {
    unsigned long j;
    char *pi, *pj, *pn;

    while (n > 1) {
        if (n > 10)
            pi = quicksort_pivot_strings(a, n, es);
        else
            pi = a + (n >> 1) * es;

        quicksort_swapi(a, pi, es);
        pi = a;
        pn = a + n * es;
        pj = pn;

        while (1) {
            do pi += es;
            while ((pi < pn) && (quicksort_compare_strings(pi, a) < 0));

            do pj -= es;
            while ((pj > a) && (quicksort_compare_strings(pj, a) > 0));

            if (pj < pi) break;
            quicksort_swapi(pi, pj, es);
        }
        quicksort_swapi(a, pj, es);
        j = (pj - a) / es;
        n = n - j - 1;

        if (j >= n) {
            quicksort_str(a, j, es);
            a += (j + 1) * es;
        } else {
            quicksort_str(a + (j + 1) * es, n, es);
            n = j;
        }
    }
}

void Quicksort::quicksort_vec(char *a, unsigned long n, unsigned long es) {
    unsigned long j;
    char *pi, *pj, *pn;

    while (n > 1) {
        if (n > 10)
            pi = quicksort_pivot_vectors(a, n, es);
        else
            pi = a + (n >> 1) * es;

        quicksort_swapi(a, pi, es);
        pi = a;
        pn = a + n * es;
        pj = pn;

        while (1) {
            do pi += es;
            while ((pi < pn) && (quicksort_compare_vectors(pi, a) < 0));

            do pj -= es;
            while ((pj > a) && (quicksort_compare_vectors(pj, a) > 0));

            if (pj < pi) break;

            quicksort_swapi(pi, pj, es);
        }

        quicksort_swapi(a, pj, es);
        j = (pj - a) / es;
        n = n - j - 1;

        if (j >= n) {
            quicksort_vec(a, j, es);
            a += (j + 1) * es;
        } else {
            quicksort_vec(a + (j + 1) * es, n, es);
            n = j;
        }
    }
}
}  // namespace Quicksort
