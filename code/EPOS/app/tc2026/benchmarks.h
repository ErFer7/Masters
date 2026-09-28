#pragma once

#include "adpcm_enc/adpcm_enc.h"
#include "ammunition/ammunition.h"
#include "anagram/anagram.h"
#include "audiobeam/audiobeam.h"
#include "bandwidth/bandwidth.h"
#include "bitcount/bitcount.h"
#include "cjpeg_transupp/cjpeg_transupp.h"
#include "cjpeg_wrbmp/cjpeg_wrbmp.h"
#include "cosf/cosf.h"
#include "cpu_hungry/cpu_hungry.h"
#include "deg2rad/deg2rad.h"
#include "dijkstra/dijkstra.h"
#include "disparity/disparity.h"
#include "fac/fac.h"
#include "fft/fft.h"
#include "filterbank/filterbank.h"
#include "fmref/fmref.h"
#include "g723_enc/g723_enc.h"
#include "gsm_enc/gsm_enc.h"
#include "h264_dec/h264_dec.h"
#include "huff_enc/huff_enc.h"
#include "iir/iir.h"
#include "kalman/kalman.h"
#include "lms/lms.h"
#include "ludcmp/ludcmp.h"
#include "matrix1/matrix1.h"
#include "md5/md5.h"
#include "minver/minver.h"
#include "mpeg2/mpeg2.h"
#include "ndes/ndes.h"
#include "petrinet/petrinet.h"
#include "pointer_chase/pointer_chase.h"
#include "prime/prime.h"
#include "quicksort/quicksort.h"
#include "recursion/recursion.h"
#include "rijndael_enc/rijndael_enc.h"
#include "sha/sha.h"
#include "statemate/statemate.h"
#include "susan/susan.h"

// TODO: Check them on the VisionFive2
// TODO: Add an "industrial" benchmark (probably SDAV)
// TODO: Check if the benchmarks are actually runninng correctly, some of them might require a proper reset for each run
enum BenchmarkType {
    // IsolBench like
    BANDWIDTH_L1,
    BANDWIDTH_L2,
    POINTER_CHASE_L1,
    POINTER_CHASE_L2,

    // TACLeBench
    RIJNDAEL_ENC,
    H264_DEC,
    MPEG2,
    SUSAN,
    CJPEG_TRANSUPP,
    CJPEG_WRBMP,
    AUDIOBEAM,  // WARN: There are possible memory corruptions here, but OK
    ANAGRAM,
    PETRINET,
    FAC,    // WARN: OK, but way too fast
    PRIME,  // WARN: OK, but way too fast
    BITCOUNT,
    COSF,
    DEG2RAD,
    MD5,
    SHA,
    FFT,
    IIR,
    LMS,
    FILTERBANK,
    MINVER,
    LUDCMP,
    MATRIX1,
    QUICKSORT,
    RECURSION,
    DIJKSTRA,
    HUFF_ENC,
    ADPCM_ENC,  // WARN: OK, but it's way too fast for now, maybe there are some weird optimizations going on
    GSM_ENC,    // WARN: OK, but can have some dangerous optmization
    G723_ENC,
    STATEMATE,
    NDES,
    AMMUNITION,
    FMREF,

    // SD-VBS
    DISPARITY,

    // Custom
    KALMAN,
    CPU_HUNGRY
};

static const char *benchmark_name(BenchmarkType benchmark_type) {
    switch (benchmark_type) {
        case BANDWIDTH_L1:
            return "BANDWIDTH_L1";
        case BANDWIDTH_L2:
            return "BANDWIDTH_L2";
        case POINTER_CHASE_L1:
            return "POINTER_CHASE_L1";
        case POINTER_CHASE_L2:
            return "POINTER_CHASE_L2";
        case RIJNDAEL_ENC:
            return "RIJNDAEL_ENC";
        case H264_DEC:
            return "H264DEC";
        case MPEG2:
            return "MPEG2";
        case SUSAN:
            return "SUSAN";
        case CJPEG_TRANSUPP:
            return "CJPEG_TRANSUPP";
        case CJPEG_WRBMP:
            return "CJPEG_WRBMP";
        case AUDIOBEAM:
            return "AUDIOBEAM";
        case ANAGRAM:
            return "ANAGRAM";
        case PETRINET:
            return "PETRINET";
        case FAC:
            return "FAC";
        case PRIME:
            return "PRIME";
        case BITCOUNT:
            return "BITCOUNT";
        case COSF:
            return "COSF";
        case DEG2RAD:
            return "DEG2RAD";
        case MD5:
            return "MD5";
        case SHA:
            return "SHA";
        case FFT:
            return "FFT";
        case IIR:
            return "IIR";
        case LMS:
            return "LMS";
        case FILTERBANK:
            return "FILTERBANK";
        case MINVER:
            return "MINVER";
        case LUDCMP:
            return "LUDCMP";
        case MATRIX1:
            return "MATRIX1";
        case QUICKSORT:
            return "QUICKSORT";
        case RECURSION:
            return "RECURSION";
        case DIJKSTRA:
            return "DIJKSTRA";
        case HUFF_ENC:
            return "HUFF_ENC";
        case ADPCM_ENC:
            return "ADPCM_ENC";
        case GSM_ENC:
            return "GSM_ENC";
        case G723_ENC:
            return "G723_ENC";
        case STATEMATE:
            return "STATEMATE";
        case NDES:
            return "NDES";
        case AMMUNITION:
            return "AMMUNITION";
        case FMREF:
            return "FMREF";
        case DISPARITY:
            return "DISPARITY";
        case KALMAN:
            return "KALMAN";
        case CPU_HUNGRY:
            return "CPU_HUNGRY";
        default:
            return "INVALID BENCHMARK!";
    };
}

template <int TaskEnum>
struct BenchmarkTraits;

template <>
struct BenchmarkTraits<RIJNDAEL_ENC> {
    using Type = RijndaelEnc::RijndaelEnc;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<KALMAN> {
    using Type = Kalman::Kalman;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<BANDWIDTH_L1> {
    using Type = Bandwidth::Bandwidth;
    static Type *create() { return new Type(Bandwidth::Bandwidth::L1_CACHE_SIZE); }
};

template <>
struct BenchmarkTraits<BANDWIDTH_L2> {
    using Type = Bandwidth::Bandwidth;
    static Type *create() { return new Type(Bandwidth::Bandwidth::L2_CACHE_SIZE); }
};

template <>
struct BenchmarkTraits<H264_DEC> {
    using Type = H264Dec::H264Dec;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<MPEG2> {
    using Type = Mpeg2::Mpeg2;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<SUSAN> {
    using Type = Susan::Susan;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<CJPEG_TRANSUPP> {
    using Type = CJpegTransupp::CJpegTransupp;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<CJPEG_WRBMP> {
    using Type = CJpegWRBMP::CJpegWRBMP;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<AUDIOBEAM> {
    using Type = Audiobeam::Audiobeam;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<ANAGRAM> {
    using Type = Anagram::Anagram;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<PETRINET> {
    using Type = Petrinet::Petrinet;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<FAC> {
    using Type = Fac::Fac;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<PRIME> {
    using Type = Prime::Prime;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<BITCOUNT> {
    using Type = BitCount::BitCount;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<COSF> {
    using Type = Cosf::Cosf;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<DEG2RAD> {
    using Type = Deg2Rad::Deg2Rad;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<MD5> {
    using Type = Md5::Md5;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<SHA> {
    using Type = Sha::Sha;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<FFT> {
    using Type = Fft::Fft;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<IIR> {
    using Type = Iir::Iir;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<LMS> {
    using Type = Lms::Lms;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<FILTERBANK> {
    using Type = Filterbank::Filterbank;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<MINVER> {
    using Type = Minver::Minver;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<LUDCMP> {
    using Type = LudCmp::LudCmp;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<MATRIX1> {
    using Type = Matrix1::Matrix1;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<QUICKSORT> {
    using Type = Quicksort::Quicksort;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<RECURSION> {
    using Type = Recursion::Recursion;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<DIJKSTRA> {
    using Type = Dijkstra::Dijkstra;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<HUFF_ENC> {
    using Type = HuffEnc::HuffEnc;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<ADPCM_ENC> {
    using Type = AdpcmEnc::AdpcmEnc;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<GSM_ENC> {
    using Type = GsmEnc::GsmEnc;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<G723_ENC> {
    using Type = G723Enc::G723Enc;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<STATEMATE> {
    using Type = Statemate::Statemate;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<NDES> {
    using Type = Ndes::Ndes;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<AMMUNITION> {
    using Type = Ammunition::Ammunition;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<FMREF> {
    using Type = Fmref::Fmref;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<POINTER_CHASE_L1> {
    using Type = PointerChase::PointerChase;
    static Type *create() { return new Type(PointerChase::PointerChase::L1_CACHE_SIZE); }
};

template <>
struct BenchmarkTraits<POINTER_CHASE_L2> {
    using Type = PointerChase::PointerChase;
    static Type *create() { return new Type(PointerChase::PointerChase::L2_CACHE_SIZE); }
};

template <>
struct BenchmarkTraits<DISPARITY> {
    using Type = Disparity::Disparity;
    static Type *create() { return new Type(); }
};

template <>
struct BenchmarkTraits<CPU_HUNGRY> {
    using Type = CpuHungry::CpuHungry;
    static Type *create() { return new Type(); }
};
