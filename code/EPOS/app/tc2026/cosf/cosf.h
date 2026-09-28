#pragma once

/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.9

  Name: cosf

  Author: Dustin Green

  Function: cosf performs calculations of the cosinus function

  Source:

  Original name:

  Changes:

  License: this code is FREE with no restrictions

*/

#include "wcclibm.h"

namespace Cosf {

class Cosf {
   public:
    Cosf() = default;

    ~Cosf() = default;

    inline int run() {
        reset();

        float i;
        for (i = 0.0f; i < 10; i += 0.1f) cosf_solutions += basicmath___cosf(i);

        int temp = cosf_solutions;

        if (temp == -4)
            return 0;
        else
            return -1;
    }

   private:
    inline void reset() { cosf_solutions = 0.0f; }

   private:
    float cosf_solutions;
};
}  // namespace Cosf
