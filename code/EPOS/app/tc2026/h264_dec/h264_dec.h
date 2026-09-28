#pragma once

/*
  This program is part of the TACLeBench benchmark suite.
  Version V 2.0

  Name: h264_dec_ldecode_macroblock.c

  Author: Inge Lille-Langoy et al.

  Function: H.264 decoder

  Source: MediaBench II
          http://euler.slu.edu/~fritts/mediabench (mirror)

  Original name: h264_dec_ldecode_macroblock.c

  Changes: no functional changes

  License: see copyright.txt

*/

/*
 * This program was further ported and adapted to EPOS on C++
 * */

namespace H264Dec {

class H264Dec {
   private:
    enum h264_dec_SliceType { P_SLICE = 0, B_SLICE, I_SLICE, SP_SLICE, SI_SLICE };

   public:
    H264Dec() {
        unsigned int i;
        unsigned char *dst;
        const unsigned char *src;
        volatile signed char bitmask = 0;

        /*
          Apply volatile XOR-bitmask to entire input array.
        */
        dst = (unsigned char *)&h264_dec_mv_array[0];
        src = (const unsigned char *)&h264_dec_mv_array_const[0];
        for (i = 0; i < sizeof(h264_dec_mv_array); ++i, ++dst, ++src) {
            *dst = *src ^ bitmask;
        }

        dst = (unsigned char *)&h264_dec_list_imgUV[0];
        src = (const unsigned char *)&h264_dec_list_imgUV_const[0];
        for (i = 0; i < sizeof(h264_dec_list_imgUV); ++i, ++dst, ++src) {
            *dst = *src ^ bitmask;
        }

        dst = (unsigned char *)&h264_dec_img_m7[0];
        src = (const unsigned char *)&h264_dec_img_m7_const[0];
        for (i = 0; i < sizeof(h264_dec_img_m7); ++i, ++dst, ++src) {
            *dst = *src ^ bitmask;
        }

        mb_cr_size_x = 8;
        mb_cr_size_y = 8;
        num_blk8x8_uv = 2;
        pix_c_x = 256;
        pix_c_y = 256;
        width_cr = 352;
        apply_weights = 0;
        direct_spatial_mv_pred_flag = 1;
        type = 1;
        wp_round_chroma = 0;
        chroma_log2_weight_denom = 0;
    }

    ~H264Dec() {}

    inline int run() {
        h264_dec_decode_one_macroblock();
        return h264_dec_return();
    }

   private:
    int h264_dec_return() { return (h264_dec_img_mpr[0][0] + h264_dec_dec_picture_imgUV[0][0][0] + 128 != 0); }

    void h264_dec_decode_one_macroblock() {
        int i = 0, j = 0, ii = 0, jj = 0, i1 = 0, j1 = 0, j4 = 0, i4 = 0;
        int uv;
        int ioff, joff;
        int bw_pred = 0, fw_pred = 0, ifx;
        int ii0, jj0, ii1, jj1, if1, jf1, if0, jf0;
        int f1_x, f1_y, f2_x, f2_y, f3, f4;

        short fw_refframe = -1, bw_refframe = -1;
        int mv_mode, pred_dir, intra_prediction;  // = currMB->ref_frame;
        short fw_ref_idx = -1, bw_ref_idx = -1;

        int mb_nr = 0;
        short dec_picture_ref_idx = 0;

        short active_sps_chroma_format_idc = 1;
        short active_pps_weighted_pred_flag = 0;
        short active_pps_weighted_bipred_idc = 0;

        int smb = 0;
        int max_y_cr = 287;

        int jf;

        int direct_pdir = -1;

        int curr_mb_field = 0;

        int b8, b4;

        int residue_transform_flag = 0;

        if (1) {
            f1_x = 64 / mb_cr_size_x;
            f2_x = f1_x - 1;

            f1_y = 64 / mb_cr_size_y;
            f2_y = f1_y - 1;

            f3 = f1_x * f1_y;
            f4 = f3 >> 1;

            for (uv = 0; uv < 2; uv++) {
                intra_prediction = 0;

                for (b8 = 0; b8 < (num_blk8x8_uv / 2); b8++) {
                    for (b4 = 0; b4 < 4; b4++) {
                        joff = 0;
                        j4 = pix_c_y + joff;
                        ioff = 0;
                        i4 = pix_c_x + ioff;

                        mv_mode = 1;
                        pred_dir = -1;

                        if (!intra_prediction) {
                            if (pred_dir != 2) {
                                for (jj = 0; jj < 4; jj++) {
                                    jf = ((j4 + jj) / (mb_cr_size_y / 4)) % 64;
                                    for (ii = 0; ii < 4; ii++) {
                                        ifx = ((i4 + ii) / (mb_cr_size_x / 4)) % 64;
                                        i1 = (i4 + ii) * f1_x + h264_dec_mv_array[jf][ifx][0];

                                        if (!curr_mb_field)
                                            j1 = (j4 + jj) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                        else {
                                            if (mb_nr % 2 == 0) {
                                                j1 = ((pix_c_y / 2) + jj + joff) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                            } else {
                                                j1 = ((pix_c_y - mb_cr_size_y) / 2 + jj + joff) * f1_y +
                                                     h264_dec_mv_array[jf][ifx][1];
                                            }
                                            ++mb_nr;
                                        }

                                        if (active_sps_chroma_format_idc == 1) j1 += 0;

                                        ii0 = (((0 < ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x))
                                                    ? ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x)
                                                    : 0)) %
                                              45;
                                        jj0 = (((0 < ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y))
                                                    ? ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y)
                                                    : 0)) %
                                              45;
                                        ii1 = (((0 < (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                          : (i1 + f2_x) / f1_x))
                                                    ? (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                           : (i1 + f2_x) / f1_x)
                                                    : 0)) %
                                              45;
                                        jj1 = (((0 < (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr : (j1 + f2_y) / f1_y))
                                                    ? (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr : (j1 + f2_y) / f1_y)
                                                    : 0)) %
                                              45;

                                        if1 = (i1 & f2_x);
                                        jf1 = (j1 & f2_y);
                                        if0 = f1_x - if1;
                                        jf0 = f1_y - jf1;

                                        if (apply_weights) {
                                        } else {
                                            h264_dec_img_mpr[ii + ioff][jj + joff] =
                                                (if0 * jf0 * h264_dec_list_imgUV[uv][jj0][ii0] +
                                                 if1 * jf0 * h264_dec_list_imgUV[uv][jj0][ii1] +
                                                 if0 * jf1 * h264_dec_list_imgUV[uv][jj1][ii0] +
                                                 if1 * jf1 * h264_dec_list_imgUV[uv][jj1][ii1] + f4) /
                                                f3;
                                        }
                                    }
                                }
                            } else {
                                for (jj = 0; jj < 4; jj++) {
                                    jf = (j4 + jj) / (mb_cr_size_y / 4);
                                    for (ii = 0; ii < 4; ii++) {
                                        ifx = (i4 + ii) / (mb_cr_size_x / 4);
                                        direct_pdir = 2;

                                        if (mv_mode == 0 && direct_spatial_mv_pred_flag) {
                                            if (dec_picture_ref_idx != -1) {
                                                fw_refframe = 0;
                                                fw_ref_idx = fw_refframe;
                                            }
                                            if (dec_picture_ref_idx != -1) {
                                                bw_refframe = 0;
                                                bw_ref_idx = bw_refframe;
                                            }

                                            if (dec_picture_ref_idx == -1)
                                                direct_pdir = 0;
                                            else if (dec_picture_ref_idx == -1)
                                                direct_pdir = 1;

                                            if (direct_pdir == 0 || direct_pdir == 2) {
                                                i1 = (pix_c_x + ii + ioff) * f1_x + h264_dec_mv_array[jf][ifx][0];

                                                if (!curr_mb_field) {
                                                    j1 = (pix_c_y + jj + joff) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                                } else {
                                                    if (mb_nr % 2 == 0) {
                                                        j1 = ((pix_c_y) / 2 + jj + joff) * f1_y +
                                                             h264_dec_mv_array[jf][ifx][1];
                                                    } else {
                                                        j1 = ((pix_c_y - mb_cr_size_y) / 2 + jj + joff) * f1_y +
                                                             h264_dec_mv_array[jf][ifx][1];
                                                    }
                                                }

                                                if (active_sps_chroma_format_idc == 1) j1 += 0;

                                                ii0 = (((0 < ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x))
                                                            ? ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x)
                                                            : 0)) %
                                                      45;
                                                jj0 = (((0 < ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y))
                                                            ? ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y)
                                                            : 0)) %
                                                      45;
                                                ii1 = (((0 < (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                                  : (i1 + f2_x) / f1_x))
                                                            ? (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                                   : (i1 + f2_x) / f1_x)
                                                            : 0)) %
                                                      45;
                                                jj1 = (((0 < (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                              : (j1 + f2_y) / f1_y))
                                                            ? (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                               : (j1 + f2_y) / f1_y)
                                                            : 0)) %
                                                      45;

                                                if1 = (i1 & f2_x);
                                                jf1 = (j1 & f2_y);
                                                if0 = f1_x - if1;
                                                jf0 = f1_y - jf1;

                                                fw_pred = (if0 * jf0 * h264_dec_list_imgUV[uv][jj0][ii0] +
                                                           if1 * jf0 * h264_dec_list_imgUV[uv][jj0][ii1] +
                                                           if0 * jf1 * h264_dec_list_imgUV[uv][jj1][ii0] +
                                                           if1 * jf1 * h264_dec_list_imgUV[uv][jj1][ii1] + f4) /
                                                          f3;
                                            }
                                            if (direct_pdir == 1 || direct_pdir == 2) {
                                                i1 = (pix_c_x + ii + ioff) * f1_x + h264_dec_mv_array[jf][ifx][0];

                                                if (!curr_mb_field) {
                                                    j1 = (pix_c_y + jj + joff) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                                } else {
                                                    if (mb_nr % 2 == 0) {
                                                        j1 = ((pix_c_y) / 2 + jj + joff) * f1_y +
                                                             h264_dec_mv_array[jf][ifx][1];
                                                    } else {
                                                        j1 = ((pix_c_y - mb_cr_size_y) / 2 + jj + joff) * f1_y +
                                                             h264_dec_mv_array[jf][ifx][1];
                                                    }
                                                }
                                                if (active_sps_chroma_format_idc == 1) j1 += 0;

                                                ii0 = (((0 < ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x))
                                                            ? ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x)
                                                            : 0)) %
                                                      45;
                                                jj0 = (((0 < ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y))
                                                            ? ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y)
                                                            : 0)) %
                                                      45;
                                                ii1 = (((0 < (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                                  : (i1 + f2_x) / f1_x))
                                                            ? (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                                   : (i1 + f2_x) / f1_x)
                                                            : 0)) %
                                                      45;
                                                jj1 = (((0 < (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                              : (j1 + f2_y) / f1_y))
                                                            ? (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                               : (j1 + f2_y) / f1_y)
                                                            : 0)) %
                                                      45;

                                                if1 = (i1 & f2_x);
                                                jf1 = (j1 & f2_y);
                                                if0 = f1_x - if1;
                                                jf0 = f1_y - jf1;

                                                bw_pred = (if0 * jf0 * h264_dec_list_imgUV[uv][jj0][ii0] +
                                                           if1 * jf0 * h264_dec_list_imgUV[uv][jj0][ii1] +
                                                           if0 * jf1 * h264_dec_list_imgUV[uv][jj1][ii0] +
                                                           if1 * jf1 * h264_dec_list_imgUV[uv][jj1][ii1] + f4) /
                                                          f3;
                                            }

                                        } else {
                                            fw_refframe = 0;
                                            bw_refframe = 0;

                                            fw_ref_idx = fw_refframe;
                                            bw_ref_idx = bw_refframe;

                                            i1 = (pix_c_x + ii + ioff) * f1_x + h264_dec_mv_array[jf][ifx][0];

                                            if (!curr_mb_field) {
                                                j1 = (pix_c_y + jj + joff) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                            } else {
                                                if (mb_nr % 2 == 0) {
                                                    j1 = ((pix_c_y) / 2 + jj + joff) * f1_y +
                                                         h264_dec_mv_array[jf][ifx][1];
                                                } else {
                                                    j1 = ((pix_c_y - mb_cr_size_y) / 2 + jj + joff) * f1_y +
                                                         h264_dec_mv_array[jf][ifx][1];
                                                }
                                            }

                                            if (active_sps_chroma_format_idc == 1) j1 += 0;

                                            ii0 = (((0 < ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x))
                                                        ? ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x)
                                                        : 0)) %
                                                  45;
                                            jj0 = (((0 < ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y))
                                                        ? ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y)
                                                        : 0)) %
                                                  45;
                                            ii1 = (((0 < (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                              : (i1 + f2_x) / f1_x))
                                                        ? (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                               : (i1 + f2_x) / f1_x)
                                                        : 0)) %
                                                  45;
                                            jj1 = (((0 <
                                                     (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr : (j1 + f2_y) / f1_y))
                                                        ? (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                           : (j1 + f2_y) / f1_y)
                                                        : 0)) %
                                                  45;

                                            if1 = (i1 & f2_x);
                                            jf1 = (j1 & f2_y);
                                            if0 = f1_x - if1;
                                            jf0 = f1_y - jf1;

                                            fw_pred = (if0 * jf0 * h264_dec_list_imgUV[uv][jj0][ii0] +
                                                       if1 * jf0 * h264_dec_list_imgUV[uv][jj0][ii1] +
                                                       if0 * jf1 * h264_dec_list_imgUV[uv][jj1][ii0] +
                                                       if1 * jf1 * h264_dec_list_imgUV[uv][jj1][ii1] + f4) /
                                                      f3;

                                            i1 = (pix_c_x + ii + ioff) * f1_x + h264_dec_mv_array[jf][ifx][0];

                                            if (!curr_mb_field) {
                                                j1 = (pix_c_y + jj + joff) * f1_y + h264_dec_mv_array[jf][ifx][1];
                                            } else {
                                                if (mb_nr % 2 == 0) {
                                                    j1 = ((pix_c_y) / 2 + jj + joff) * f1_y +
                                                         h264_dec_mv_array[jf][ifx][1];
                                                } else {
                                                    j1 = ((pix_c_y - mb_cr_size_y) / 2 + jj + joff) * f1_y +
                                                         h264_dec_mv_array[jf][ifx][1];
                                                }
                                            }

                                            if (active_sps_chroma_format_idc == 1) j1 += 0;

                                            ii0 = (((0 < ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x))
                                                        ? ((i1 / f1_x > width_cr - 1) ? width_cr - 1 : i1 / f1_x)
                                                        : 0)) %
                                                  45;
                                            jj0 = (((0 < ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y))
                                                        ? ((j1 / f1_y > max_y_cr) ? max_y_cr : j1 / f1_y)
                                                        : 0)) %
                                                  45;
                                            ii1 = (((0 < (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                              : (i1 + f2_x) / f1_x))
                                                        ? (((i1 + f2_x) / f1_x > width_cr - 1) ? width_cr - 1
                                                                                               : (i1 + f2_x) / f1_x)
                                                        : 0)) %
                                                  45;
                                            jj1 = (((0 <
                                                     (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr : (j1 + f2_y) / f1_y))
                                                        ? (((j1 + f2_y) / f1_y > max_y_cr) ? max_y_cr
                                                                                           : (j1 + f2_y) / f1_y)
                                                        : 0)) %
                                                  45;

                                            if1 = (i1 & f2_x);
                                            jf1 = (j1 & f2_y);
                                            if0 = f1_x - if1;
                                            jf0 = f1_y - jf1;

                                            bw_pred = (if0 * jf0 * h264_dec_list_imgUV[uv][jj0][ii0] +
                                                       if1 * jf0 * h264_dec_list_imgUV[uv][jj0][ii1] +
                                                       if0 * jf1 * h264_dec_list_imgUV[uv][jj1][ii0] +
                                                       if1 * jf1 * h264_dec_list_imgUV[uv][jj1][ii1] + f4) /
                                                      f3;
                                        }

                                        if (apply_weights) {
                                            if (((active_pps_weighted_pred_flag &&
                                                  (type == P_SLICE || type == SP_SLICE)) ||
                                                 (active_pps_weighted_bipred_idc == 1 && (type == B_SLICE))) &&
                                                curr_mb_field) {
                                                fw_ref_idx >>= 1;
                                                bw_ref_idx >>= 1;
                                            }

                                            if (direct_spatial_mv_pred_flag && direct_pdir == 1) {
                                                mpr[ii + ioff][jj + joff] =
                                                    (((wp_round_chroma) >> chroma_log2_weight_denom) < 0
                                                         ? 0
                                                         : ((wp_round_chroma) >> chroma_log2_weight_denom)) +
                                                    0;
                                            } else if (direct_spatial_mv_pred_flag && direct_pdir == 0) {
                                                mpr[ii + ioff][jj + joff] =
                                                    ((((wp_round_chroma) >> chroma_log2_weight_denom)) < 0
                                                         ? 0
                                                         : (((wp_round_chroma) >> chroma_log2_weight_denom)));
                                            } else {
                                                int alpha_fw = 0;
                                                int alpha_bw = 0;

                                                mpr[ii + ioff][jj + joff] =
                                                    (((alpha_fw * fw_pred + alpha_bw * bw_pred +
                                                       (1 << chroma_log2_weight_denom)) >>
                                                      (chroma_log2_weight_denom + 1)) < 0
                                                         ? 0
                                                         : ((alpha_fw * fw_pred + alpha_bw * bw_pred +
                                                             (1 << chroma_log2_weight_denom)) >>
                                                            (chroma_log2_weight_denom + 1)));
                                            }
                                        } else {
                                            if (direct_spatial_mv_pred_flag && direct_pdir == 1)
                                                mpr[ii + ioff][jj + joff] = bw_pred;
                                            else if (direct_spatial_mv_pred_flag && direct_pdir == 0)
                                                mpr[ii + ioff][jj + joff] = fw_pred;
                                            else {
                                                mpr[ii + ioff][jj + joff] = (fw_pred + bw_pred + 1) / 2;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (!smb) {
                            for (ii = 0; ii < 4; ii++) {
                                jj = 0;
                                for (; jj < 4; jj++) {
                                    if (!residue_transform_flag) {
                                        h264_dec_dec_picture_imgUV[uv][(j4 + jj) % 64][(i4 + ii) % 54] =
                                            h264_dec_img_m7[ii][jj];
                                    }
                                }
                            }
                        }
                    }
                }

                if (smb) {
                    for (j = 4; j < 6; j++) {
                        joff = (j - 4) * 4;
                        j4 = pix_c_y + joff;
                        for (i = 0; i < 2; i++) {
                            ioff = i * 4;
                            i4 = pix_c_x + ioff;

                            for (ii = 0; ii < 4; ii++)
                                for (jj = 0; jj < 4; jj++) {
                                    h264_dec_dec_picture_imgUV[uv][(j4 + jj) % 64][(i4 + ii) % 54] =
                                        h264_dec_img_m7[ii][jj];
                                }
                        }
                    }
                }
            }
        }
    }

   private:
    int direct_spatial_mv_pred_flag;
    int type;
    int width_cr;
    int pix_c_y;
    int pix_c_x;
    unsigned short mpr[16][16];
    unsigned int chroma_log2_weight_denom;
    int wp_round_chroma;
    unsigned int apply_weights;
    int num_blk8x8_uv;
    int mb_cr_size_x;
    int mb_cr_size_y;
    signed char h264_dec_img_mpr[7][7];
    signed char h264_dec_dec_picture_imgUV[2][64][54];
    // Moving this to the heap would just be annoying
    signed char h264_dec_mv_array[65][65][2];
    short h264_dec_list_imgUV[2][45][45];
    int h264_dec_img_m7[16][16];
    static const signed char h264_dec_mv_array_const[65][65][2];
    static const short h264_dec_list_imgUV_const[2][45][45];
    static const int h264_dec_img_m7_const[16][16];
};
}  // namespace H264Dec
