#include "tc2026.h"

using namespace EPOS;

int main() {
    BenchmarkRunner::init();
    BenchmarkRunner::run();
    BenchmarkRunner::print_thread_timing();
    BenchmarkRunner::free();

    return 0;
}
