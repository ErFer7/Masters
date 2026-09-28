#pragma once

#include <utility/ostream.h>
#include "disparity_image.h"

using namespace EPOS;

extern OStream cout;

//#include "sdvbs_common.h"
//comment the following line in order to compile disparity without mser
///*
typedef struct
{
    int width;
    int height;
    int data[];
}I2D;

typedef struct
{
    int width;
    int height;
    unsigned int data[];
}UI2D;

typedef struct
{
    int width;
    int height;
    float data[];
}F2D;

typedef long long int64_t;
typedef unsigned long long uint64_t;

#define subsref(a,i,j) a->data[(i) * a->width + (j)]
#define asubsref(a,i) a->data[i]
#define arrayref(a,i) a[i]

/** Image read and write **/
I2D* readImage(const char* pathName);;
F2D* readFile(unsigned char* fileName);

/** Memory allocation functions **/
I2D* iMallocHandle(int rows, int cols);
F2D* fMallocHandle(int rows, int cols);
UI2D* uiMallocHandle(int rows, int cols);

void iFreeHandle(I2D* out);
void fFreeHandle(F2D* out);
void uiFreeHandle(UI2D* out);

/** Memory copy/set function **/
I2D* iSetArray(int rows, int cols, int val);
F2D* fSetArray(int rows, int cols, float val);
UI2D* uiSetArray(int rows, int cols, int val);
I2D* iDeepCopy(I2D* in);
F2D* fDeepCopy(F2D* in);
I2D* iDeepCopyRange(I2D* in, int startRow, int numberRows, int startCol, int numberCols);
F2D* fDeepCopyRange(F2D* in, int startRow, int numberRows, int startCol, int numberCols);
F2D* fiDeepCopy(I2D* in);
I2D* ifDeepCopy(F2D* in);

/** Matrix operations - concatenation, reshape **/
F2D* ffVertcat(F2D* matrix1, F2D* matrix2);
I2D* iVertcat(I2D* matrix1, I2D* matrix2);
F2D* fHorzcat(F2D* a, F2D* b);
I2D* iHorzcat(I2D* a, I2D* b);
F2D* horzcat(F2D* a, F2D* b, F2D* c);
F2D* fTranspose(F2D* a);
I2D* iTranspose(I2D* a);
F2D* fReshape(F2D* in, int rows, int cols);
I2D* iReshape(I2D* in, int rows, int cols);

/** Binary Operations **/
F2D* fDivide(F2D* a, float b);
F2D* fMdivide(F2D* a, F2D* b);
F2D* ffDivide(F2D* a, F2D* b);
F2D* ffTimes(F2D* a, float b);
F2D* fTimes(F2D* a, F2D* b);
I2D* iTimes(I2D* a, I2D* b);
F2D* fMtimes(F2D* a, F2D* b);
F2D* ifMtimes(I2D* a, F2D* b);
F2D* fMinus(F2D* a, F2D* b);
I2D* iMinus(I2D* a, I2D* b);
I2D* isMinus(I2D* a, int b);
F2D* fPlus(F2D* a, F2D* b);
I2D* isPlus(I2D* a, int b);

/** Filtering operations **/
F2D* calcSobel_dX(F2D* imageIn);
F2D* calcSobel_dY(F2D* imageIn);
F2D* ffConv2(F2D* a, F2D* b);
F2D* fiConv2(I2D* a, F2D* b);
F2D* ffConv2_dY(F2D* a, F2D* b);
F2D* ffiConv2(F2D* a, I2D* b);
I2D* iiConv2(I2D* a, I2D* b);

/** Image Transformations - resize, integration etc **/
F2D* imageResize(F2D* imageIn);
F2D* imageBlur(I2D* imageIn);

/** Support functions **/
F2D* fFind3(F2D* in);
F2D* fSum2(F2D* inMat, int dir);
F2D* fSum(F2D* inMat);
I2D* iSort(I2D* in, int dim);
F2D* fSort(F2D* in, int dim);
I2D* iSortIndices(I2D* in, int dim);
I2D* fSortIndices(F2D* input, int dim);
F2D* randnWrapper(int m, int n);
F2D* randWrapper(int m, int n);

/** Checking functions **/
int selfCheck(I2D* in1, char* path, int tol);
int fSelfCheck(F2D* in1, char* path, float tol);
void writeMatrix(I2D* input, char* inpath);
void fWriteMatrix(F2D* input, char* inpath);

/** Timing functions **/
unsigned int* photonEndTiming(void);
unsigned int* photonStartTiming(void);
unsigned int* photonReportTiming(unsigned int* startCycles,unsigned int* endCycles);
void photonPrintTiming(unsigned int * elapsed);

typedef unsigned int u32;
static unsigned int temp;
void computeSAD(I2D *Ileft, I2D* Iright_moved, F2D* SAD);
I2D* getDisparity(I2D* Ileft, I2D* Iright, int win_sz, int max_shift);
void finalSAD(F2D* integralImg, int win_sz, F2D* retSAD);
void findDisparity(F2D* retSAD, F2D* minSAD, I2D* retDisp, int level, int nr, int nc);
void integralImage2D2D(F2D* SAD, F2D* integralImg);
void correlateSAD_2D(I2D* Ileft, I2D* Iright, I2D* Iright_moved, int win_sz, int disparity, F2D* SAD, F2D* integralImg, F2D* retSAD);
I2D* padarray2(I2D* inMat, I2D* borderMat);
void padarray4(I2D* inMat, I2D* borderMat, int dir, I2D* paddedArray);

struct DisparityAlloc {
    unsigned char sminSAD[8 * 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sretDisp[8 + 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char shalfWin[8 + 4 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sSAD[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sintergalImg[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sIright_moved[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
    unsigned char sretSAD[8 + 8 * IMG_HEIGHT * IMG_WIDTH];
};

unsigned char spaddedArray[2 * 8 * IMG_WIDTH * IMG_HEIGHT];

I2D* getDisparity(I2D* Ileft, I2D* Iright, int win_sz, int max_shift, DisparityAlloc *data)
{
    I2D* retDisp;
    int nr, nc, k;
    I2D *halfWin;
    int half_win_sz, rows, cols;
    F2D *retSAD, *minSAD, *SAD, *integralImg;
    I2D* IrightPadded, *IleftPadded, *Iright_moved, *Iout;

    int i, j;
    F2D *fout;

    nr = Ileft->height;
    nc = Ileft->width;
    half_win_sz=win_sz/2;

    fout = (F2D *)data->sminSAD;
    fout->height = nr;
    fout->width = nc;
    for(i=0; i<nr; i++) {
        for(j=0; j<nc; j++) {
            subsref(fout,i,j) = 255.0*255.0;
        }
    }

    minSAD = fout;

    Iout = (I2D *)data->sretDisp; //iMallocHandle(rows, cols);

    Iout->height = nr;
    Iout->width = nc;

    for(i=0; i<nr; i++) {
        for(j=0; j<nc; j++) {
            subsref(Iout,i,j) = max_shift;
        }
    }

    retDisp = Iout;

    Iout = (I2D *)data->shalfWin; //iMallocHandle(rows, cols);

    Iout->height = nr;
    Iout->width = nc;

    for(i=0; i<nr; i++) {
        for(j=0; j<nc; j++) {
            subsref(Iout,i,j) = half_win_sz;
        }
    }

    halfWin = Iout;

    if(win_sz > 1)
    {
        IleftPadded = padarray2(Ileft, halfWin);
        IrightPadded = padarray2(Iright, halfWin);
    }
    else
    {
        IleftPadded = Ileft;
        IrightPadded = Iright;
    }

    rows = IleftPadded->height;
    cols = IleftPadded->width;

    fout = (F2D *)data->sSAD;
    fout->height = rows;
    fout->width = cols;
    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            subsref(fout,i,j) = 255;
        }
    }

    SAD = fout;

    //integralImg = fSetArray(rows, cols,0);

    fout = (F2D *)data->sintergalImg;
    fout->height = rows;
    fout->width = cols;
    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            subsref(fout,i,j) = 0;
        }
    }

    integralImg = fout;

    //retSAD = fMallocHandle(rows-win_sz, cols-win_sz);
    retSAD =  (F2D *)data->sretSAD;

    retSAD->height = rows-win_sz;
    retSAD->width  = cols-win_sz;

    //Iright_moved = iSetArray(rows, cols, 0);

    Iout = (I2D *)data->sIright_moved; //iMallocHandle(rows, cols);

    Iout->height = rows;
    Iout->width = cols;

    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            subsref(Iout,i,j) = 0;
        }
    }

    Iright_moved = Iout;


    for( k=0; k<max_shift; k++)
    {
        correlateSAD_2D(IleftPadded, IrightPadded, Iright_moved, win_sz, k, SAD, integralImg, retSAD); // Removed Mallocs here
        findDisparity(retSAD, minSAD, retDisp, k, nr, nc); // No malloc here
    }

    return retDisp;
}

void computeSAD(I2D *Ileft, I2D* Iright_moved, F2D* SAD)
{
    int rows, cols, i, j, diff;

    rows = Ileft->height;
    cols = Ileft->width;

    for(i=0; i<rows; i++)
    {
        for(j=0; j<cols; j++)
        {
            diff = subsref(Ileft,i,j) - subsref(Iright_moved,i,j);
            subsref(SAD,i,j) = diff * diff;
        }
    }

    return;
}

void correlateSAD_2D(I2D* Ileft, I2D* Iright, I2D* Iright_moved, int win_sz, int disparity, F2D* SAD, F2D* integralImg, F2D* retSAD)
{
    int rows, cols;
    int i;
    I2D *range;
    unsigned char sRange[8 + sizeof(int)*1*2];

    //range = iMallocHandle(1,2);
    range = (I2D*) sRange;
    subsref(range,0,0) = 0;
    subsref(range,0,1) = disparity; // no malloc

    rows = Iright_moved->height;
    cols = Iright_moved->width;

    //printf("rows = %d col = %d \n", rows,cols);

    for(i=0; i<rows*cols; i++)
        asubsref(Iright_moved,i) = 0; // No Malloc



    padarray4(Iright, range, -1, Iright_moved); // No Malloc here as well good



    computeSAD(Ileft, Iright_moved, SAD);
    integralImage2D2D(SAD, integralImg);
    finalSAD(integralImg, win_sz, retSAD);


    return;
}

void finalSAD(F2D* integralImg, int win_sz, F2D* retSAD)
{
    int endR, endC;
    int i, j;

    endR = integralImg->height;
    endC = integralImg->width;

    for(j=0; j<(endC-win_sz); j++)
    {
        for(i=0; i<(endR-win_sz); i++)
        {
            subsref(retSAD,i,j) = subsref(integralImg,(win_sz+i),(j+win_sz)) + subsref(integralImg,(i+1) ,(j+1)) - subsref(integralImg,(i+1),(j+win_sz)) - subsref(integralImg,(win_sz+i),(j+1));
        }
    }

    return;
}

void findDisparity(F2D* retSAD, F2D* minSAD, I2D* retDisp, int level, int nr, int nc)
{
    // cout << "retSAD: " << retSAD << ", minSAD: " << minSAD << ", retDisp: " << retDisp << ", level: " << level << ", nr: " << nr << ", nc: " << nc << endl;
    for (volatile int i = 0; i < 100; i++);

    int i, j, a, b;

    for(i=0; i<nr; i++)
    {
        for(j=0; j<nc; j++)
        {
            a = subsref(retSAD,i,j);
            b = subsref(minSAD,i,j);
            if(a<b)
            {
                subsref(minSAD,i,j) = a;
                subsref(retDisp,i,j) = level;
            }
        }
    }
    return;
}

void integralImage2D2D(F2D* SAD, F2D* integralImg)
{
    int nr, nc, i, j;

    nr = SAD->height;
    nc = SAD->width;

    for(i=0; i<nc; i++)
        subsref(integralImg,0,i) = subsref(SAD,0,i);

    for(i=1; i<nr; i++)
        for(j=0; j<nc; j++)
        {
            subsref(integralImg,i,j) = subsref(integralImg, (i-1), j) + subsref(SAD,i,j);
        }

    for(i=0; i<nr; i++)
        for(j=1; j<nc; j++)
            subsref(integralImg,i,j) = subsref(integralImg, i, (j-1)) + subsref(integralImg,i,j);

    return;

}

I2D* padarray2(I2D* inMat, I2D* borderMat)
{
    int rows, cols, bRows, bCols, newRows, newCols;
    I2D *paddedArray, *Iout;
    int i, j;

    rows = inMat->height;
    cols = inMat->width;

    bRows = borderMat->data[0];
    bCols = borderMat->data[1];

    newRows = rows + bRows*2;
    newCols = cols + bCols*2;

    //paddedArray = iSetArray(newRows, newCols, 0);

    Iout = (I2D*) (spaddedArray + temp * 8 * IMG_WIDTH * IMG_HEIGHT); //iMallocHandle(rows, cols);

    Iout->height = newRows;
    Iout->width  = newCols;

    for(i=0; i<newRows; i++) {
        for(j=0; j<newCols; j++) {
            subsref(Iout,i,j) = 0;
        }
    }

    paddedArray = Iout;

    temp = temp + 1;



    if(temp > 1)
        temp =0;

    //printf("temp %d \n", temp);

    for(i=0; i<rows; i++)
        for(j=0; j<cols; j++)
            subsref(paddedArray, (bRows+i), (bCols+j)) = subsref(inMat, i, j);

    return paddedArray;
}

void padarray4(I2D* inMat, I2D* borderMat, int dir, I2D* paddedArray)
{
    int rows, cols, bRows, bCols;
    int i, j;

    rows = inMat->height;
    cols = inMat->width;

    bRows = borderMat->data[0];
    bCols = borderMat->data[1];

    if(dir ==1)
    {
        for(i=0; i<rows; i++)
            for(j=0; j<cols; j++)
                subsref(paddedArray, i, j) = subsref(inMat,i,j);
    }
    else
    {
        for(i=0; i<rows-bRows; i++)
            for(j=0; j<cols-bCols; j++)
                subsref(paddedArray, (bRows+i), (bCols+j)) = subsref(inMat,i,j);
    }

    // printf("padarray4 \n");

    return;
}

inline int disparity()
{
//    int rows = 32;
//    int cols = 32;
    I2D *imleft, *imright, *retDisparity;

    int WIN_SZ=8, SHIFT=64;

    imleft  = (I2D *) img1;
    imright = (I2D *) img2;

//    rows = imleft->height;
//    cols = imleft->width;

    (void)SHIFT;
    (void)WIN_SZ;

    retDisparity = getDisparity(imleft, imright, WIN_SZ, SHIFT);

    return retDisparity->height;
}

