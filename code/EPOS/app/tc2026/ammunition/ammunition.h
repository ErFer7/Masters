#pragma once

namespace Ammunition {

class Ammunition {
   public:
    Ammunition() { ammunition_result = 0; }

    ~Ammunition() = default;

    int run();

   private:
    void ammunition_reset_str_bits(char *str, char *s);
    void ammunition_reset_str_arithm(char *str, char *s, char *d, char *e, char *g);
    int ammunition_bits_test();
    int ammunition_arithm_test();

   private:
    int ammunition_result;
};
}  // namespace Ammunition
