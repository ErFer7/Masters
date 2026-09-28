#pragma once

#include "disparity_data.h"

namespace Disparity {

typedef struct {
    int width;
    int height;
    int data[];
} I2D;

typedef struct {
    int width;
    int height;
    unsigned int data[];
} UI2D;

typedef struct {
    int width;
    int height;
    float data[];
} F2D;

typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned int u32;

#define subsref(a, i, j) a->data[(i) * a->width + (j)]
#define asubsref(a, i) a->data[i]
#define arrayref(a, i) a[i]

class Disparity {
   public:
    Disparity() {
        t_img1 = new signed char[sizeof(img1)];
        t_img2 = new signed char[sizeof(img2)];

        for (unsigned int i = 0; i < sizeof(img1); i++) {
            t_img1[i] = img1[i];
        }

        for (unsigned int i = 0; i < sizeof(img2); i++) {
            t_img2[i] = img2[i];
        }

        temp = 0;

        for (unsigned int i = 0; i < sizeof(sminSAD); i++) {
            sminSAD[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(sretDisp); i++) {
            sretDisp[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(shalfWin); i++) {
            shalfWin[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(sSAD); i++) {
            sSAD[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(sintergalImg); i++) {
            sintergalImg[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(sIright_moved); i++) {
            sIright_moved[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(sretSAD); i++) {
            sretSAD[i] = 0;
        }

        for (unsigned int i = 0; i < sizeof(spaddedArray); i++) {
            spaddedArray[i] = 0;
        }
    }

    ~Disparity() {
        delete[] t_img1;
        delete[] t_img2;
    }

    int run() {
        I2D *imleft, *imright, *retDisparity;

        int WIN_SZ = 8, SHIFT = 64;

        imleft = (I2D *)t_img1;
        imright = (I2D *)t_img2;

        // We don't need to delete the returned data because it was allocated in the stack before (disparity_allocs)
        retDisparity = getDisparity(imleft, imright, WIN_SZ, SHIFT);
        int height = retDisparity->height;

        return height;
    }

   private:
    I2D *getDisparity(I2D *Ileft, I2D *Iright, int win_sz, int max_shift) {
        I2D *retDisp;
        int nr, nc, k;
        I2D *halfWin;
        int half_win_sz, rows, cols;
        F2D *retSAD, *minSAD, *SAD, *integralImg;
        I2D *IrightPadded, *IleftPadded, *Iright_moved, *Iout;

        int i, j;
        F2D *fout;

        nr = Ileft->height;
        nc = Ileft->width;
        half_win_sz = win_sz / 2;

        fout = (F2D *)sminSAD;
        fout->height = nr;
        fout->width = nc;
        for (i = 0; i < nr; i++) {
            for (j = 0; j < nc; j++) {
                subsref(fout, i, j) = 255.0 * 255.0;
            }
        }

        minSAD = fout;

        Iout = (I2D *)sretDisp;  // iMallocHandle(rows, cols);

        Iout->height = nr;
        Iout->width = nc;

        for (i = 0; i < nr; i++) {
            for (j = 0; j < nc; j++) {
                subsref(Iout, i, j) = max_shift;
            }
        }

        retDisp = Iout;

        Iout = (I2D *)shalfWin;  // iMallocHandle(rows, cols);

        Iout->height = nr;
        Iout->width = nc;

        for (i = 0; i < nr; i++) {
            for (j = 0; j < nc; j++) {
                subsref(Iout, i, j) = half_win_sz;
            }
        }

        halfWin = Iout;

        if (win_sz > 1) {
            IleftPadded = padarray2(Ileft, halfWin);
            IrightPadded = padarray2(Iright, halfWin);
        } else {
            IleftPadded = Ileft;
            IrightPadded = Iright;
        }

        rows = IleftPadded->height;
        cols = IleftPadded->width;

        fout = (F2D *)sSAD;
        fout->height = rows;
        fout->width = cols;
        for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
                subsref(fout, i, j) = 255;
            }
        }

        SAD = fout;

        // integralImg = fSetArray(rows, cols,0);

        fout = (F2D *)sintergalImg;
        fout->height = rows;
        fout->width = cols;
        for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
                subsref(fout, i, j) = 0;
            }
        }

        integralImg = fout;

        // retSAD = fMallocHandle(rows-win_sz, cols-win_sz);
        retSAD = (F2D *)sretSAD;

        retSAD->height = rows - win_sz;
        retSAD->width = cols - win_sz;

        // Iright_moved = iSetArray(rows, cols, 0);

        Iout = (I2D *)sIright_moved;  // iMallocHandle(rows, cols);

        Iout->height = rows;
        Iout->width = cols;

        for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
                subsref(Iout, i, j) = 0;
            }
        }

        Iright_moved = Iout;

        for (k = 0; k < max_shift; k++) {
            correlateSAD_2D(
                IleftPadded, IrightPadded, Iright_moved, win_sz, k, SAD, integralImg, retSAD);  // Removed Mallocs here
            findDisparity(retSAD, minSAD, retDisp, k, nr, nc);                                  // No malloc here
        }

        return retDisp;
    }

    void computeSAD(I2D *Ileft, I2D *Iright_moved, F2D *SAD) {
        int rows, cols, i, j, diff;

        rows = Ileft->height;
        cols = Ileft->width;

        for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
                diff = subsref(Ileft, i, j) - subsref(Iright_moved, i, j);
                subsref(SAD, i, j) = diff * diff;
            }
        }

        return;
    }

    void correlateSAD_2D(I2D *Ileft,
                         I2D *Iright,
                         I2D *Iright_moved,
                         int win_sz,
                         int disparity,
                         F2D *SAD,
                         F2D *integralImg,
                         F2D *retSAD) {
        int rows, cols;
        int i;
        I2D *range;
        unsigned char sRange[8 + sizeof(int) * 1 * 2];

        // range = iMallocHandle(1,2);
        range = (I2D *)sRange;
        subsref(range, 0, 0) = 0;
        subsref(range, 0, 1) = disparity;  // no malloc

        rows = Iright_moved->height;
        cols = Iright_moved->width;

        // printf("rows = %d col = %d \n", rows,cols);

        for (i = 0; i < rows * cols; i++) asubsref(Iright_moved, i) = 0;  // No Malloc

        padarray4(Iright, range, -1, Iright_moved);  // No Malloc here as well good

        computeSAD(Ileft, Iright_moved, SAD);
        integralImage2D2D(SAD, integralImg);
        finalSAD(integralImg, win_sz, retSAD);

        return;
    }

    void finalSAD(F2D *integralImg, int win_sz, F2D *retSAD) {
        int endR, endC;
        int i, j;

        endR = integralImg->height;
        endC = integralImg->width;

        for (j = 0; j < (endC - win_sz); j++) {
            for (i = 0; i < (endR - win_sz); i++) {
                subsref(retSAD, i, j) =
                    subsref(integralImg, (win_sz + i), (j + win_sz)) + subsref(integralImg, (i + 1), (j + 1)) -
                    subsref(integralImg, (i + 1), (j + win_sz)) - subsref(integralImg, (win_sz + i), (j + 1));
            }
        }

        return;
    }

    void findDisparity(F2D *retSAD, F2D *minSAD, I2D *retDisp, int level, int nr, int nc) {
        // cout << "retSAD: " << retSAD << ", minSAD: " << minSAD << ", retDisp: " << retDisp << ", level: " << level <<
        // ", nr: " << nr << ", nc: " << nc << endl;
        for (volatile int i = 0; i < 100; i++);

        int i, j, a, b;

        for (i = 0; i < nr; i++) {
            for (j = 0; j < nc; j++) {
                a = subsref(retSAD, i, j);
                b = subsref(minSAD, i, j);
                if (a < b) {
                    subsref(minSAD, i, j) = a;
                    subsref(retDisp, i, j) = level;
                }
            }
        }
        return;
    }

    void integralImage2D2D(F2D *SAD, F2D *integralImg) {
        int nr, nc, i, j;

        nr = SAD->height;
        nc = SAD->width;

        for (i = 0; i < nc; i++) subsref(integralImg, 0, i) = subsref(SAD, 0, i);

        for (i = 1; i < nr; i++)
            for (j = 0; j < nc; j++) {
                subsref(integralImg, i, j) = subsref(integralImg, (i - 1), j) + subsref(SAD, i, j);
            }

        for (i = 0; i < nr; i++)
            for (j = 1; j < nc; j++)
                subsref(integralImg, i, j) = subsref(integralImg, i, (j - 1)) + subsref(integralImg, i, j);

        return;
    }

    I2D *padarray2(I2D *inMat, I2D *borderMat) {
        int rows, cols, bRows, bCols, newRows, newCols;
        I2D *paddedArray, *Iout;
        int i, j;

        rows = inMat->height;
        cols = inMat->width;

        bRows = borderMat->data[0];
        bCols = borderMat->data[1];

        newRows = rows + bRows * 2;
        newCols = cols + bCols * 2;

        // paddedArray = iSetArray(newRows, newCols, 0);

        Iout = (I2D *)(spaddedArray + temp * 8 * IMG_WIDTH * IMG_HEIGHT);  // iMallocHandle(rows, cols);

        Iout->height = newRows;
        Iout->width = newCols;

        for (i = 0; i < newRows; i++) {
            for (j = 0; j < newCols; j++) {
                subsref(Iout, i, j) = 0;
            }
        }

        paddedArray = Iout;

        temp = temp + 1;

        if (temp > 1) temp = 0;

        // printf("temp %d \n", temp);

        for (i = 0; i < rows; i++)
            for (j = 0; j < cols; j++) subsref(paddedArray, (bRows + i), (bCols + j)) = subsref(inMat, i, j);

        return paddedArray;
    }

    void padarray4(I2D *inMat, I2D *borderMat, int dir, I2D *paddedArray) {
        int rows, cols, bRows, bCols;
        int i, j;

        rows = inMat->height;
        cols = inMat->width;

        bRows = borderMat->data[0];
        bCols = borderMat->data[1];

        if (dir == 1) {
            for (i = 0; i < rows; i++)
                for (j = 0; j < cols; j++) subsref(paddedArray, i, j) = subsref(inMat, i, j);
        } else {
            for (i = 0; i < rows - bRows; i++)
                for (j = 0; j < cols - bCols; j++)
                    subsref(paddedArray, (bRows + i), (bCols + j)) = subsref(inMat, i, j);
        }

        // printf("padarray4 \n");

        return;
    }

   private:
    signed char *t_img1;
    signed char *t_img2;
    unsigned int temp;
    unsigned char sminSAD[8 * 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sretDisp[8 + 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char shalfWin[8 + 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sSAD[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sintergalImg[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sIright_moved[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sretSAD[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char spaddedArray[2 * 8 * IMG_WIDTH * IMG_HEIGHT];
};

}  // namespace Disparity
