#ifndef __QUICKSORT_H
#define __QUICKSORT_H

namespace Quicksort {

struct quicksort_3DVertexStruct {
    unsigned int x, y, z;
    double distance;
};

class Quicksort {
   public:
    Quicksort();

    ~Quicksort() = default;

    int run();

   private:
    inline void reset();

    void quicksort_str(char *, unsigned long, unsigned long);
    void quicksort_vec(char *, unsigned long, unsigned long);

   private:
    static const char *quicksort_input_string[681];
    char quicksort_strings[681][20];

    unsigned int quicksort_input_vector[1000 * 3];
    static const unsigned int quicksort_input_vector_data[1000 * 3];
    quicksort_3DVertexStruct quicksort_vectors[1000];

    volatile int quicksort_const_prop_border_i = 0;
    volatile char quicksort_const_prop_border_c = 0;
};

}  // namespace Quicksort

#endif
