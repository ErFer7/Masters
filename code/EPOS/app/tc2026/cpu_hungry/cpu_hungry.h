#pragma once

namespace CpuHungry {

class CpuHungry {
   public:
    CpuHungry() = default;

    ~CpuHungry() = default;

    inline int run() { return static_cast<int>(1.33 * iterative_fib()); }

   private:
    // method that configures iterative fibonacci execution
    int iterative_fib() {
        int fib = 1;
        int temp = 1;
        int prev = 1;
        fib = 1;
        prev = 1;
        for (int j = 1; j < 10000; j++) {
            temp = prev + fib;
            prev = fib;
            fib = temp;
        }
        return fib;
    }
};

}  // namespace CpuHungry
