#pragma once

/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: fac

  Author: unknown

  Function: fac is a program to calculate factorials.
    This program computes the sum of the factorials
    from zero to five.

  Source: MRTC
          http://www.mrtc.mdh.se/projects/wcet/wcet_bench/fac/fac.c

  Changes: CS 2006/05/19: Changed loop bound from constant to variable.

  License: public domain

*/

namespace Fac {

class Fac {
   public:
    Fac() { fac_n = 5; }

    ~Fac() = default;

    inline int run() {
        reset();

        int i;

        for (i = 0; i <= fac_n; i++) {
            fac_s += fac_fac(i);
        }

        int expected_result = 154;
        return fac_s - expected_result;
    }

   private:
    inline void reset() { fac_s = 0; }

    int fac_fac(int n) {
        if (n == 0)
            return 1;
        else
            return (n * fac_fac(n - 1));
    }

   private:
    int fac_s;
    volatile int fac_n;
};
}  // namespace Fac
