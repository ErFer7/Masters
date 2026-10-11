#pragma once

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

// fibonacci recursive method
int fib(int pos) {
    if (pos == 1 || pos == 0) {
        return 1;
    } else {
        return (fib(pos - 1) + fib(pos - 2));
    }
}

inline int cpu_hungry() { return (int)(1.33 * iterative_fib()); }
