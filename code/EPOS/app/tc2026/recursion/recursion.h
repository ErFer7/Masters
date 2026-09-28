#pragma once

/*
  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: recursion

  Author: unknown

  Function: recursion is a recursion program.
    This program computes the Fibonacci number recursively.

  Source: MRTC
          http://www.mrtc.mdh.se/projects/wcet/wcet_bench/recursion/recursion.c

  Changes: no major functional changes

  License: May be used, modified, and re-distributed freely.

*/

namespace Recursion {

class Recursion {
   public:
    Recursion() {
        int volatile temp_input = 10;
        recursion_input = temp_input;
    }

    ~Recursion() = default;

    inline int run() {
        recursion_result = recursion_fib(recursion_input);
        return (recursion_result + (-89)) != 0;
    }

   private:
    int recursion_fib(int i) {
        if (i == 0) return 1;
        if (i == 1) return 1;

        return recursion_fib(i - 1) + recursion_fib(i - 2);
    }

   private:
    int recursion_result;
    int recursion_input;
};
}  // namespace Recursion
