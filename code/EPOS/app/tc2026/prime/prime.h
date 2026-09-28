#pragma once

/*

  This program is part of the TACLeBench benchmark suite.
  Version V 2.0

  Name: prime

  Author: unknown

  Function: prime calculates whether numbers are prime.

  Source: MRTC
          http://www.mrtc.mdh.se/projects/wcet/wcet_bench/prime/prime.c

  Changes: no major functional changes

  License: may be used, modified, and re-distributed freely

*/

namespace Prime {

class Prime {
   public:
    Prime() {
        prime_initSeed();

        prime_x = prime_randomInteger();
        prime_y = prime_randomInteger();
    }

    ~Prime() = default;

    inline int run() {
        prime_swap(&prime_x, &prime_y);

        prime_result = !(!prime_prime(prime_x) && !prime_prime(prime_y));

        return prime_result;
    }

   private:
    /*
      Initialization- and return-value-related functions
    */

    void prime_initSeed() { prime_seed = 0; }

    unsigned int prime_randomInteger() {
        prime_seed = ((prime_seed * 133) + 81) % 8095;
        return (prime_seed);
    }

    /*
      Algorithm core functions
    */

    unsigned char prime_divides(unsigned int n, unsigned int m) { return (m % n == 0); }

    unsigned char prime_even(unsigned int n) { return (prime_divides(2, n)); }

    unsigned char prime_prime(unsigned int n) {
        unsigned int i;
        if (prime_even(n)) return (n == 2);
        for (i = 3; i * i <= n; i += 2) {
            if (prime_divides(i, n)) /* ai: loop here min 0 max 357 end; */
                return 0;
        }
        return (n > 1);
    }

    void prime_swap(unsigned int *a, unsigned int *b) {
        unsigned int tmp = *a;
        *a = *b;
        *b = tmp;
    }

   private:
    unsigned int prime_x;
    unsigned int prime_y;
    int prime_result;
    volatile int prime_seed;
};
}  // namespace Prime
