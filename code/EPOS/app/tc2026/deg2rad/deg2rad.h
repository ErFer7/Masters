#pragma once

/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.9

  Name: deg2rad

  Author: unknown

  Function: deg2rad performs conversion of degree to radiant

  Source: MiBench
          http://wwweb.eecs.umich.edu/mibench

  Original name: basicmath_small

  Changes: no major functional changes

  License: this code is FREE with no restrictions

*/

namespace Deg2Rad {

#define deg2rad(d) ((d) * 3.14 / 180)

class Deg2Rad {
   public:
    Deg2Rad() {
        deg2rad_X = 0;
    }

    ~Deg2Rad() = default;

    inline int run() {
        reset();

        for (deg2rad_X = 0.0f; deg2rad_X <= 360.0f; deg2rad_X += 1.0f) deg2rad_Y += deg2rad(deg2rad_X);

        int temp = deg2rad_Y;

        if (temp == 1133)
            return 0;
        else
            return -1;
    }

   private:
    inline void reset() { deg2rad_Y = 0; }

   private:
    float deg2rad_X;
    float deg2rad_Y;
};
}  // namespace Deg2Rad
