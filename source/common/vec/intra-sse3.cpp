/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
 *          Steve Borho <steve@borho.org>
 *          ShinYee Chung <shinyee@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#if defined(_MSC_VER)
#define ALWAYSINLINE  __forceinline
#endif

#define INSTRSET 3
#include "vectorclass.h"

#include "primitives.h"
#include "TLibCommon/TComRom.h"
#include <assert.h>

using namespace x265;

extern unsigned char IntraFilterType[][35];

#define PRED_INTRA_ANG4_START   \
    Vec8s row11, row12, row21, row22, row31, row32, row41, row42;   \
    Vec16uc tmp16_1, tmp16_2;   \
    Vec2uq tmp2uq;  \
    Vec8s v_deltaFract, v_deltaPos(0), thirty2(32), thirty1(31), v_ipAngle(0);  \
    bool modeHor = (dirMode < 18);

#define PRED_INTRA_ANG4_END \
    v_deltaFract = v_deltaPos & thirty1;    \
    row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;  \
    /*row2*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;  \
    /*row3*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;  \
    /*row4*/    \
    v_deltaPos += v_ipAngle;    \
    v_deltaFract = v_deltaPos & thirty1;    \
    row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;  \
    /* Flip the block */    \
    if (modeHor)    \
    {   \
        Vec8s tmp1, tmp2, tmp3, tmp4;   \
        tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31); \
        tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41); \
        tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);   \
        tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2); \
        tmp16_1 = compress_unsafe(tmp3, tmp3);  \
        store_partial(const_int(4), dst, tmp16_1); \
        tmp2uq = reinterpret_i(tmp16_1);    \
        tmp2uq >>= 32;  \
        store_partial(const_int(4), dst + dstStride, tmp2uq);  \
        tmp16_1 = compress_unsafe(tmp4, tmp4);  \
        store_partial(const_int(4), dst + (2 * dstStride), tmp16_1);   \
        tmp2uq = reinterpret_i(tmp16_1);    \
        tmp2uq >>= 32;  \
        store_partial(const_int(4), dst + (3 * dstStride), tmp2uq);    \
    }   \
    else    \
    {   \
        store_partial(const_int(4), dst, compress_unsafe(row11, row11));   \
        store_partial(const_int(4), dst + (dstStride), compress_unsafe(row21, row21)); \
        store_partial(const_int(4), dst + (2 * dstStride), compress_unsafe(row31, row31)); \
        store_partial(const_int(4), dst + (3 * dstStride), compress_unsafe(row41, row41)); \
    }

#define PRED_INTRA_ANG8_START   \
    /* Map the mode index to main prediction direction and angle*/    \
    bool modeHor       = (dirMode < 18);    \
    bool modeVer       = !modeHor;  \
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;    \
    int absAng         = abs(intraPredAngle);   \
    int signAng        = intraPredAngle < 0 ? -1 : 1;   \
    /* Set bitshifts and scale the angle parameter to block size*/  \
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };   \
    absAng             = angTable[absAng];  \
    intraPredAngle     = signAng * absAng;  \
    if (modeHor)         /* Near horizontal modes*/   \
    { \
        Vec16uc tmp;    \
        Vec8s row11, row12; \
        Vec16uc row1, row2, row3, row4, tmp16_1, tmp16_2;   \
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;    \
        Vec8s tmp1, tmp2;   \
        v_deltaPos = 0; \
        v_ipAngle = intraPredAngle; \

#define PRED_INTRA_ANG8_MIDDLE   \
    /* Flip the block */    \
    tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);    \
    tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);  \
    row1 = tmp16_1; \
    row2 = tmp16_2; \
    tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);    \
    tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);  \
    row3 = tmp16_1; \
    row4 = tmp16_2; \
    tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row1, row2);    \
    tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row1, row2);  \
    row1 = tmp16_1; \
    row2 = tmp16_2; \
    tmp16_1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(row3, row4);    \
    tmp16_2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(row3, row4);  \
    row3 = tmp16_1; \
    row4 = tmp16_2; \
    tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row1, (Vec4i)row3);    \
    tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row1, (Vec4i)row3);    \
    row1 = tmp16_1; \
    row3 = tmp16_2; \
    tmp16_1 = blend4i<0, 4, 1, 5>((Vec4i)row2, (Vec4i)row4);    \
    tmp16_2 = blend4i<2, 6, 3, 7>((Vec4i)row2, (Vec4i)row4);    \
    row2 = tmp16_1; \
    row4 = tmp16_2; \
    store_partial(const_int(8), dst, row1);       /*row1*/   \
    store_partial(const_int(8), dst + (2 * dstStride), row3);       /*row3*/   \
    store_partial(const_int(8), dst + (4 * dstStride), row2);       /*row5*/   \
    store_partial(const_int(8), dst + (6 * dstStride), row4);       /*row7*/   \
    row1 = blend2q<1, 3>((Vec2q)row1, (Vec2q)row1); \
    store_partial(const_int(8), dst + (1 * dstStride), row1);       /*row2*/   \
    row1 = blend2q<1, 3>((Vec2q)row3, (Vec2q)row3); \
    store_partial(const_int(8), dst + (3 * dstStride), row1);       /*row4*/   \
    row1 = blend2q<1, 3>((Vec2q)row2, (Vec2q)row2);    \
    store_partial(const_int(8), dst + (5 * dstStride), row1);       /*row6*/   \
    row1 = blend2q<1, 3>((Vec2q)row4, (Vec2q)row4); \
    store_partial(const_int(8), dst + (7 * dstStride), row1);       /*row8*/   \
    }   \
    else                         /* Vertical modes*/    \
    { \
        Vec8s row11, row12; \
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;    \
        Vec16uc tmp;    \
        Vec8s tmp1, tmp2;   \
        v_deltaPos = 0; \
        v_ipAngle = intraPredAngle; \


namespace {
const int angAP[17][64] =
{
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
    },
    {
        0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29, 30, 30, 31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52
    },
    {
        0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 40, 40, 41, 42
    },
    {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 34
    },
    {
        0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24, 25, 25, 26
    },
    {
        0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18
    },
    {
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
    },
    { // 0th virtual index; never used; just to help indexing
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4
    },
    {
        -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -4, -4, -4, -4, -4, -4, -5, -5, -5, -5, -5, -5, -5, -6, -6, -6, -6, -6, -6, -7, -7, -7, -7, -7, -7, -8, -8, -8, -8, -8, -8, -8, -9, -9, -9, -9, -9, -9, -10, -10, -10, -10, -10, -10, -10
    },
    {
        -1, -1, -1, -2, -2, -2, -2, -3, -3, -3, -4, -4, -4, -4, -5, -5, -5, -6, -6, -6, -6, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, -9, -10, -10, -10, -11, -11, -11, -11, -12, -12, -12, -13, -13, -13, -13, -14, -14, -14, -15, -15, -15, -15, -16, -16, -16, -17, -17, -17, -17, -18, -18, -18, -18
    },
    {
        -1, -1, -2, -2, -3, -3, -3, -4, -4, -5, -5, -5, -6, -6, -7, -7, -7, -8, -8, -9, -9, -9, -10, -10, -11, -11, -11, -12, -12, -13, -13, -13, -14, -14, -15, -15, -16, -16, -16, -17, -17, -18, -18, -18, -19, -19, -20, -20, -20, -21, -21, -22, -22, -22, -23, -23, -24, -24, -24, -25, -25, -26, -26, -26
    },
    {
        -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -10, -10, -11, -11, -12, -12, -13, -13, -14, -14, -15, -15, -16, -16, -17, -17, -18, -19, -19, -20, -20, -21, -21, -22, -22, -23, -23, -24, -24, -25, -25, -26, -27, -27, -28, -28, -29, -29, -30, -30, -31, -31, -32, -32, -33, -33, -34, -34
    },
    {
        -1, -2, -2, -3, -4, -4, -5, -6, -6, -7, -8, -8, -9, -10, -10, -11, -12, -12, -13, -14, -14, -15, -16, -16, -17, -18, -18, -19, -20, -20, -21, -21, -22, -23, -23, -24, -25, -25, -26, -27, -27, -28, -29, -29, -30, -31, -31, -32, -33, -33, -34, -35, -35, -36, -37, -37, -38, -39, -39, -40, -41, -41, -42, -42
    },
    {
        -1, -2, -3, -4, -5, -5, -6, -7, -8, -9, -9, -10, -11, -12, -13, -13, -14, -15, -16, -17, -18, -18, -19, -20, -21, -22, -22, -23, -24, -25, -26, -26, -27, -28, -29, -30, -31, -31, -32, -33, -34, -35, -35, -36, -37, -38, -39, -39, -40, -41, -42, -43, -44, -44, -45, -46, -47, -48, -48, -49, -50, -51, -52, -52
    },
    {
        -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -17, -18, -19, -20, -21, -22, -23, -24, -25, -26, -27, -28, -29, -30, -31, -32, -33, -34, -35, -36, -37, -38, -39, -40, -41, -42, -43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56, -57, -58, -59, -60, -61, -62, -63, -64
    }
};

#define GETAP(X, Y) angAP[8 - (X)][(Y)]

static inline
void predDCFiltering(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int /*height*/)
{
    int y;
    pixel pixDC = *dst;
    int pixDCx3 = pixDC * 3 + 2;

    // boundary pixels processing
    dst[0] = (pixel)((above[0] + left[0] + 2 * pixDC + 2) >> 2);

    Vec8us im1(pixDCx3);
    Vec8us im2, im3;
#if HIGH_BIT_DEPTH
    switch (width)
    {
    case 4:
        im2 = load_partial(const_int(8), &above[1]);
        im2 = (im1 + im2) >> const_int(2);
        store_partial(const_int(8), &dst[1], im2);
        break;

    case 8:
        im2.load(&above[1]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);
        break;

    case 16:
        im2.load(&above[1]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);

        im2.load(&above[1 + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 8]);
        break;

    case 32:
        im2.load(&above[1]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);

        im2.load(&above[1 + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 8]);

        im2.load(&above[1 + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 16]);

        im2.load(&above[1 + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 24]);
        break;

    //case 64:
    default:
        im2.load(&above[1]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1]);

        im2.load(&above[1 + 8]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 8]);

        im2.load(&above[1 + 16]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 16]);

        im2.load(&above[1 + 24]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 24]);

        im2.load(&above[1 + 32]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 32]);

        im2.load(&above[1 + 40]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 40]);

        im2.load(&above[1 + 48]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 48]);

        im2.load(&above[1 + 56]);
        im2 = (im1 + im2) >> const_int(2);
        im2.store(&dst[1 + 56]);
        break;
    }

#else /* if HIGH_BIT_DEPTH */
    Vec16uc pix;
    switch (width)
    {
    case 4:
        pix = load_partial(const_int(4), &above[1]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(4), &dst[1], pix);
        break;

    case 8:
        pix = load_partial(const_int(8), &above[1]);
        im2 = extend_low(pix);
        im2 = (im1 + im2) >> const_int(2);
        pix = compress(im2, im2);
        store_partial(const_int(8), &dst[1], pix);
        break;

    case 16:
        pix.load(&above[1]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);
        break;

    case 32:
        pix.load(&above[1]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);

        pix.load(&above[1 + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 16]);
        break;

    //case 64:
    default:
        pix.load(&above[1]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1]);

        pix.load(&above[1 + 16]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 16]);

        pix.load(&above[1 + 32]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 32]);

        pix.load(&above[1 + 48]);
        im2 = extend_low(pix);
        im3 = extend_high(pix);
        im2 = (im1 + im2) >> const_int(2);
        im3 = (im1 + im3) >> const_int(2);
        pix = compress(im2, im3);
        pix.store(&dst[1 + 48]);
        break;
    }

#endif /* if HIGH_BIT_DEPTH */

    for (y = 1; y < width; y++)
    {
        dst[dstStride] = (pixel)((left[y] + pixDCx3) >> 2);
        dst += dstStride;
    }
}

void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int filter)
{
    int sum;
    int logSize = g_convertToBit[width] + 2;

#if HIGH_BIT_DEPTH
    Vec8s sumLeft(0);
    Vec8s sumAbove(0);
    Vec8s m0;

    switch (width)
    {
    case 4:
        sumLeft  = load_partial(const_int(8), left);
        sumAbove = load_partial(const_int(8), above);
        break;
    case 8:
        m0.load(left);
        sumLeft = m0;
        m0.load(above);
        sumAbove = m0;
        break;
    case 16:
        m0.load(left);
        sumLeft  = m0;
        m0.load(left + 8);
        sumLeft += m0;

        m0.load(above);
        sumAbove  = m0;
        m0.load(above + 8);
        sumAbove += m0;
        break;
    case 32:
        m0.load(left);
        sumLeft  = m0;
        m0.load(left + 8);
        sumLeft += m0;
        m0.load(left + 16);
        sumLeft += m0;
        m0.load(left + 24);
        sumLeft += m0;

        m0.load(above);
        sumAbove  = m0;
        m0.load(above + 8);
        sumAbove += m0;
        m0.load(above + 16);
        sumAbove += m0;
        m0.load(above + 24);
        sumAbove += m0;
        break;
    //case 64:
    default:
        // CHECK_ME: the max support bit_depth is 13-bits
        m0.load(left);
        sumLeft  = m0;
        m0.load(left + 8);
        sumLeft += m0;
        m0.load(left + 16);
        sumLeft += m0;
        m0.load(left + 24);
        sumLeft += m0;
        m0.load(left + 32);
        sumLeft += m0;
        m0.load(left + 40);
        sumLeft += m0;
        m0.load(left + 48);
        sumLeft += m0;
        m0.load(left + 56);
        sumLeft += m0;

        m0.load(above);
        sumAbove  = m0;
        m0.load(above + 8);
        sumAbove += m0;
        m0.load(above + 16);
        sumAbove += m0;
        m0.load(above + 24);
        sumAbove += m0;
        m0.load(above + 32);
        sumAbove += m0;
        m0.load(above + 40);
        sumAbove += m0;
        m0.load(above + 48);
        sumAbove += m0;
        m0.load(above + 56);
        sumAbove += m0;
        break;
    }

    sum = horizontal_add_x(sumAbove) + horizontal_add_x(sumLeft);

    logSize += 1;
    pixel dcVal = (sum + (1 << (logSize - 1))) >> logSize;
    Vec8us dcValN(dcVal);
    int k;

    pixel *dst1 = dst;
    switch (width)
    {
    case 4:
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        break;

    case 8:
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        dcValN.store(dst1);
        dst1 += dstStride;
        break;

    case 16:
        for (k = 0; k < 16; k += 2)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 + 8);
            dst1 += dstStride;
            dcValN.store(dst1);
            dcValN.store(dst1 + 8);
            dst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k++)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 +  8);
            dcValN.store(dst1 + 16);
            dcValN.store(dst1 + 24);
            dst1 += dstStride;
        }

        break;

    //case 64:
    default:
        for (k = 0; k < 64; k++)
        {
            dcValN.store(dst1);
            dcValN.store(dst1 +  8);
            dcValN.store(dst1 + 16);
            dcValN.store(dst1 + 24);
            dcValN.store(dst1 + 32);
            dcValN.store(dst1 + 40);
            dcValN.store(dst1 + 48);
            dcValN.store(dst1 + 56);
            dst1 += dstStride;
        }

        break;
    }

    if (filter)
    {
        predDCFiltering(above, left, dst, dstStride, width, width);
    }
#else // if !HIGH_BIT_DEPTH

    {
        Vec16uc pixL, pixT;
        Vec8us  im;
        Vec4ui  im1, im2;

        switch (width)
        {
        case 4:
            pixL.fromUint32(*(uint32_t*)left);
            pixT.fromUint32(*(uint32_t*)above);
            sum  = horizontal_add(extend_low(pixL));
            sum += horizontal_add(extend_low(pixT));
            break;
        case 8:
#if X86_64
            pixL.fromUint64(*(uint64_t*)left);
            pixT.fromUint64(*(uint64_t*)above);
#else
            pixL.load_partial(8, left);
            pixT.load_partial(8, above);
#endif
            sum  = horizontal_add(extend_low(pixL));
            sum += horizontal_add(extend_low(pixT));
            break;
        case 16:
            pixL.load(left);
            pixT.load(above);
            sum  = horizontal_add_x(pixL);
            sum += horizontal_add_x(pixT);
            break;
        case 32:
            pixL.load(left);
            im1  = (Vec4ui)(pixL.sad(_mm_setzero_si128()));
            pixL.load(left + 16);
            im1 += (Vec4ui)(pixL.sad(_mm_setzero_si128()));

            pixT.load(above);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            pixT.load(above + 16);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            sum  = toInt32(im1);
            break;
        //case 64:
        default:
            pixL.load(left);
            im1  = (Vec4ui)(pixL.sad(_mm_setzero_si128()));
            pixL.load(left + 16);
            im1 += (Vec4ui)(pixL.sad(_mm_setzero_si128()));
            pixL.load(left + 32);
            im1 += (Vec4ui)(pixL.sad(_mm_setzero_si128()));
            pixL.load(left + 48);
            im1 += (Vec4ui)(pixL.sad(_mm_setzero_si128()));

            pixT.load(above);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            pixT.load(above + 16);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            pixT.load(above + 32);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            pixT.load(above + 48);
            im1 += (Vec4ui)(pixT.sad(_mm_setzero_si128()));
            im1 += (Vec4ui)((Vec128b)im1 >> const_int(64));
            sum  = toInt32(im1);
            break;
        }
    }

    logSize += 1;
    pixel dcVal = (sum + (1 << (logSize - 1))) >> logSize;
    Vec16uc dcValN(dcVal);
    int k;

    pixel *dst1 = dst;
    switch (width)
    {
    case 4:
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(4), dst1, dcValN);
        break;

    case 8:
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        dst1 += dstStride;
        store_partial(const_int(8), dst1, dcValN);
        break;

    case 16:
        for (k = 0; k < 16; k += 4)
        {
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1, dcValN);
            dst1 += dstStride;
        }

        break;

    case 32:
        for (k = 0; k < 32; k += 2)
        {
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            dst1 += dstStride;
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            dst1 += dstStride;
        }

        break;

    case 64:
        for (k = 0; k < 64; k++)
        {
            store_partial(const_int(16), dst1,      dcValN);
            store_partial(const_int(16), dst1 + 16, dcValN);
            store_partial(const_int(16), dst1 + 32, dcValN);
            store_partial(const_int(16), dst1 + 48, dcValN);
            dst1 += dstStride;
        }

        break;
    }

    if (filter)
    {
        predDCFiltering(above, left, dst, dstStride, width, width);
    }
#endif // if HIGH_BIT_DEPTH
}

#define BROADCAST16(a, d, x) { \
    const int dL = (d) & 3; \
    const int dH = ((d)-4) & 3; \
    if (d>=4) { \
        (x) = _mm_shufflehi_epi16((a), dH * 0x55); \
        (x) = _mm_unpackhi_epi64((x), (x)); \
    } \
    else { \
        (x) = _mm_shufflelo_epi16((a), dL * 0x55); \
        (x) = _mm_unpacklo_epi64((x), (x)); \
    } \
}


#if HIGH_BIT_DEPTH
// CHECK_ME: I am not sure the v_rightColumnN will be overflow when input is 12bpp
void intra_pred_planar4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    int bottomLeft, topRight;
    // NOTE: I use 16-bits is enough here, because we have least than 13-bits as input, and shift left by 2, it is 15-bits

    // Get left and above reference column and row
    Vec8s v_topRow = (Vec8s)load_partial(const_int(8), above); // topRow

    Vec8s v_leftColumn = (Vec8s)load_partial(const_int(8), left);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[4];
    topRight   = above[4];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    v_topRow = v_topRow << const_int(2);
    v_leftColumn = v_leftColumn << const_int(2);

    // Generate prediction signal
    Vec8s v_horPred4 = v_leftColumn + Vec8s(4);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    // line0
    v_horPred = broadcast(const_int(0), v_horPred4);
    v_rightColumnN = broadcast(const_int(0), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    // CHECK_ME: the HM don't clip the pixel, so I assume there is biggest 12+3=15(bits)
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[0 * dstStride], v_im4);

    // line1
    v_horPred = broadcast(const_int(1), v_horPred4);
    v_rightColumnN = broadcast(const_int(1), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[1 * dstStride], v_im4);

    // line2
    v_horPred = broadcast(const_int(2), v_horPred4);
    v_rightColumnN = broadcast(const_int(2), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[2 * dstStride], v_im4);

    // line3
    v_horPred = broadcast(const_int(3), v_horPred4);
    v_rightColumnN = broadcast(const_int(3), v_rightColumn) * v_multi;
    v_horPred = v_horPred + v_rightColumnN;
    v_topRow = v_topRow + v_bottomRow;
    v_im4 = (Vec8s)(v_horPred + v_topRow) >> const_int(3);
    store_partial(const_int(8), &dst[3 * dstStride], v_im4);
}

#else /* if HIGH_BIT_DEPTH */
void intra_pred_planar4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i im0 = _mm_cvtsi32_si128(*(int*)above); // topRow
    __m128i v_topRow = _mm_unpacklo_epi8(im0, _mm_setzero_si128());

    __m128i v_leftColumn = _mm_cvtsi32_si128(*(int*)left);  // leftColumn
    v_leftColumn = _mm_unpacklo_epi8(v_leftColumn, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[4];
    topRight   = above[4];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight = _mm_set1_epi16(topRight);

    __m128i v_bottomRow = _mm_sub_epi16(v_bottomLeft, v_topRow);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumn);

    v_topRow = _mm_slli_epi16(v_topRow, 2);
    v_leftColumn = _mm_slli_epi16(v_leftColumn, 2);

    __m128i v_horPred4 = _mm_add_epi16(v_leftColumn, _mm_set1_epi16(4));
    const __m128i v_multi = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;
    __m128i v_im5;

#define COMP_PRED_PLANAR4_ROW(X) { \
        BROADCAST16(v_horPred4, (X), v_horPred); \
        BROADCAST16(v_rightColumn, (X), v_rightColumnN); \
        v_rightColumnN = _mm_mullo_epi16(v_rightColumnN, v_multi); \
        v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
        v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
        v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), 3); \
        v_im5 = _mm_packus_epi16(v_im4, v_im4); \
        *(int*)&dst[(X)*dstStride] = _mm_cvtsi128_si32(v_im5); \
}

    COMP_PRED_PLANAR4_ROW(0)
    COMP_PRED_PLANAR4_ROW(1)
    COMP_PRED_PLANAR4_ROW(2)
    COMP_PRED_PLANAR4_ROW(3)

#undef COMP_PRED_PLANAR4_ROW
}
#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = _mm_add_epi16(v_horPred, v_rightColumnN); \
        v_topRow = _mm_add_epi16(v_topRow, v_bottomRow); \
        v_im4 = _mm_srai_epi16(_mm_add_epi16(v_horPred, v_topRow), (3 + 1)); \
        _mm_storeu_si128((__m128i*)&dst[X * dstStride], v_im4); \
}

void intra_pred_planar8(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    int bottomLeft, topRight;

    // Get left and above reference column and row
    __m128i v_topRow = _mm_loadu_si128((__m128i*)above); // topRow
    __m128i v_leftColumn = _mm_loadu_si128((__m128i*)left); // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[8];
    topRight   = above[8];

    __m128i v_bottomLeft = _mm_set1_epi16(bottomLeft);
    __m128i v_topRight = _mm_set1_epi16(topRight);

    __m128i v_bottomRow = _mm_sub_epi16(v_bottomLeft, v_topRow);
    __m128i v_rightColumn = _mm_sub_epi16(v_topRight, v_leftColumn);

    v_topRow = _mm_slli_epi16(v_topRow, (2 + 1));
    v_leftColumn = _mm_slli_epi16(v_leftColumn, (2 + 1));

    // Generate prediction signal
    __m128i v_horPred4 = _mm_add_epi16(v_leftColumn, _mm_set1_epi16(8));
    const __m128i v_multi = _mm_setr_epi16(1, 2, 3, 4, 5, 6, 7, 8);
    __m128i v_horPred, v_rightColumnN;
    __m128i v_im4;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW
#else /* if HIGH_BIT_DEPTH */

#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_rightColumnN = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn) * v_multi; \
        v_horPred = v_horPred + v_rightColumnN; \
        v_topRow = v_topRow + v_bottomRow; \
        v_im4 = (Vec8s)(v_horPred + v_topRow) >> (3 + shift); \
        v_im5 = compress(v_im4, v_im4); \
        store_partial(const_int(8), &dst[X * dstStride], v_im5); \
}

void intra_pred_planar8(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(8), (void*)above); // topRow
    Vec8s v_topRow = extend_low(im0);

    Vec8s v_leftColumn = _mm_loadl_epi64((__m128i*)left);   // leftColumn
    v_leftColumn = _mm_unpacklo_epi8(v_leftColumn, _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[8];
    topRight   = above[8];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow = v_bottomLeft - v_topRow;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_convertToBit[8];         // Using value corresponding to width = 8
    v_topRow = v_topRow << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(8);
    const Vec8s v_multi(1, 2, 3, 4, 5, 6, 7, 8);
    Vec8s v_horPred, v_rightColumnN;
    Vec8s v_im4;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7
}

#undef COMP_PRED_PLANAR_ROW
#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im4_lo.store(&dst[X * dstStride]); \
        v_im4_hi.store(&dst[X * dstStride + 8]); \
}

void intra_pred_planar16(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    Vec8s v_topRow_lo, v_topRow_hi;

    v_topRow_lo.load(&above[0]);
    v_topRow_hi.load(&above[8]);

    Vec8s v_leftColumn;
    v_leftColumn.load(left);   // leftColumn

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[16];
    topRight   = above[16];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_convertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    v_leftColumn.load(left + 8);   // leftColumn lower 8 rows
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}

#undef COMP_PRED_PLANAR_ROW

#else /* if HIGH_BIT_DEPTH */
#define COMP_PRED_PLANAR_ROW(X) { \
        v_horPred_lo = permute8s<X, X, X, X, X, X, X, X>(v_horPred4); \
        v_horPred_hi = v_horPred_lo; \
        v_rightColumnN_lo = permute8s<X, X, X, X, X, X, X, X>(v_rightColumn); \
        v_rightColumnN_hi = v_rightColumnN_lo; \
        v_rightColumnN_lo *= v_multi_lo; \
        v_rightColumnN_hi *= v_multi_hi; \
        v_horPred_lo = v_horPred_lo + v_rightColumnN_lo; \
        v_horPred_hi = v_horPred_hi + v_rightColumnN_hi; \
        v_topRow_lo = v_topRow_lo + v_bottomRow_lo; \
        v_topRow_hi = v_topRow_hi + v_bottomRow_hi; \
        v_im4_lo = (Vec8s)(v_horPred_lo + v_topRow_lo) >> (3 + shift); \
        v_im4_hi = (Vec8s)(v_horPred_hi + v_topRow_hi) >> (3 + shift); \
        v_im5 = compress(v_im4_lo, v_im4_hi); \
        store_partial(const_int(16), &dst[X * dstStride], v_im5); \
}

void intra_pred_planar16(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
{
    pixel bottomLeft, topRight;

    // Get left and above reference column and row
    Vec16uc im0 = (Vec16uc)load_partial(const_int(16), above); // topRow
    Vec8s v_topRow_lo = extend_low(im0);
    Vec8s v_topRow_hi = extend_high(im0);

    Vec8s v_leftColumn = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)left), _mm_setzero_si128());

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[16];
    topRight   = above[16];

    Vec8s v_bottomLeft(bottomLeft);
    Vec8s v_topRight(topRight);

    Vec8s v_bottomRow_lo = v_bottomLeft - v_topRow_lo;
    Vec8s v_bottomRow_hi = v_bottomLeft - v_topRow_hi;
    Vec8s v_rightColumn = v_topRight - v_leftColumn;

    int shift = g_convertToBit[16];         // Using value corresponding to width = 8
    v_topRow_lo = v_topRow_lo << (2 + shift);
    v_topRow_hi = v_topRow_hi << (2 + shift);
    v_leftColumn = v_leftColumn << (2 + shift);

    Vec8s v_horPred4 = v_leftColumn + Vec8s(16);
    const Vec8s v_multi_lo(1, 2, 3, 4, 5, 6, 7, 8);
    const Vec8s v_multi_hi(9, 10, 11, 12, 13, 14, 15, 16);
    Vec8s v_horPred_lo, v_horPred_hi, v_rightColumnN_lo, v_rightColumnN_hi;
    Vec8s v_im4_lo, v_im4_hi;
    Vec16uc v_im5;

    COMP_PRED_PLANAR_ROW(0);     // row 0
    COMP_PRED_PLANAR_ROW(1);
    COMP_PRED_PLANAR_ROW(2);
    COMP_PRED_PLANAR_ROW(3);
    COMP_PRED_PLANAR_ROW(4);
    COMP_PRED_PLANAR_ROW(5);
    COMP_PRED_PLANAR_ROW(6);
    COMP_PRED_PLANAR_ROW(7);     // row 7

    // leftColumn lower 8 rows
    v_leftColumn = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(left + 8)), _mm_setzero_si128());
    v_rightColumn = v_topRight - v_leftColumn;
    v_leftColumn = v_leftColumn << (2 + shift);
    v_horPred4 = v_leftColumn + Vec8s(16);

    COMP_PRED_PLANAR_ROW(8);     // row 0
    COMP_PRED_PLANAR_ROW(9);
    COMP_PRED_PLANAR_ROW(10);
    COMP_PRED_PLANAR_ROW(11);
    COMP_PRED_PLANAR_ROW(12);
    COMP_PRED_PLANAR_ROW(13);
    COMP_PRED_PLANAR_ROW(14);
    COMP_PRED_PLANAR_ROW(15);
}

#undef COMP_PRED_PLANAR_ROW

#endif /* if HIGH_BIT_DEPTH */

typedef void intra_pred_planar_t (pixel* above, pixel* left, pixel* dst, intptr_t dstStride);
intra_pred_planar_t *intraPlanarN[] =
{
    intra_pred_planar4,
    intra_pred_planar8,
    intra_pred_planar16,
};

void intra_pred_planar(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width)
{
    int nLog2Size = g_convertToBit[width] + 2;

    int k, l, bottomLeft, topRight;
    int horPred;
    // OPT_ME: when width is 64, the shift1D is 8, then the dynamic range is [-65280, 65280], so we have to use 32 bits here
    int32_t leftColumn[MAX_CU_SIZE], topRow[MAX_CU_SIZE];
    // CHECK_ME: dynamic range is 9 bits or 15 bits(I assume max input bit_depth is 14 bits)
    int16_t bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
    int blkSize = width;
    int offset2D = width;
    int shift1D = nLog2Size;
    int shift2D = shift1D + 1;

    if (width < 32)
    {
        intraPlanarN[nLog2Size - 2](above, left, dst, dstStride);
        return;
    }

    // Get left and above reference column and row
    for (k = 0; k < blkSize; k++)
    {
        topRow[k] = above[k];
        leftColumn[k] = left[k];
    }

    // Prepare intermediate variables used in interpolation
    bottomLeft = left[blkSize];
    topRight   = above[blkSize];
    for (k = 0; k < blkSize; k++)
    {
        bottomRow[k]   = bottomLeft - topRow[k];
        rightColumn[k] = topRight   - leftColumn[k];
        topRow[k]      <<= shift1D;
        leftColumn[k]  <<= shift1D;
    }

    // Generate prediction signal
    for (k = 0; k < blkSize; k++)
    {
        horPred = leftColumn[k] + offset2D;
        for (l = 0; l < blkSize; l++)
        {
            horPred += rightColumn[k];
            topRow[l] += bottomRow[l];
            dst[k * dstStride + l] = ((horPred + topRow[l]) >> shift2D);
        }
    }
}

#if HIGH_BIT_DEPTH
void xPredIntraAng4x4(pixel* dst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 4
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec8s v_temp;
            Vec8s v_side_0; // refSide[0] value in a vector
            v_temp.load((void*)refSide);
            v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);

            Vec8s v_side;
            v_side.load(refSide + 1);

            Vec8s v_main;
            v_main = load_partial(const_int(8), (void*)(refMain + 1));

            Vec8s tmp1, tmp2;
            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(v_main, v_main);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp1);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            Vec8s row0;
            v_side -= v_side_0;
            v_side = v_side >> 1;
            row0 = tmp2 + v_side;
            row0 = min(max(0, row0), (1 << X265_DEPTH) - 1);

            store_partial(const_int(8), dst, row0);                //row0
            store_partial(const_int(8), dst + (2 * dstStride), tmp1); //row2

            tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp2, tmp2);
            tmp1 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp1);

            store_partial(const_int(8), dst + (3 * dstStride), tmp1); //row3
            store_partial(const_int(8), dst + (dstStride), tmp2);    //row1
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), dst, v_main);
            store_partial(const_int(8), dst + dstStride, v_main);
            store_partial(const_int(8), dst + (2 * dstStride), v_main);
            store_partial(const_int(8), dst + (3 * dstStride), v_main);

            for (int k = 0; k < 4; k++)
            {
                dst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << X265_DEPTH) - 1), static_cast<short>((dst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
        store_partial(const_int(8), dst, tmp);
        tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
        store_partial(const_int(8), dst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 2);
        store_partial(const_int(8), dst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain - 3);
        store_partial(const_int(8), dst + 3 * dstStride, tmp);
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
        store_partial(const_int(8), dst, tmp);
        tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
        store_partial(const_int(8), dst + dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 4);
        store_partial(const_int(8), dst + 2 * dstStride, tmp);
        tmp = load_partial(const_int(8), refMain + 5);
        store_partial(const_int(8), dst + 3 * dstStride, tmp);
        return;
    }
    else
    {
        Vec8s row11, row12, row21, row22, row31, row32, row41, row42;
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;

        row11 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0));
        row12 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 0) + 1);

        row21 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1));
        row22 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 1) + 1);

        row31 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2));
        row32 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 2) + 1);

        row41 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3));
        row42 = (Vec8s)load_partial(const_int(8), refMain + 1 + GETAP(lookIdx, 3) + 1);

        v_deltaPos = v_ipAngle = intraPredAngle;

        //row1
        v_deltaFract = v_deltaPos & thirty1;
        row11 = ((thirty2 - v_deltaFract) * row11 + (v_deltaFract * row12) + 16) >> 5;

        //row2
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row21 = ((thirty2 - v_deltaFract) * row21 + (v_deltaFract * row22) + 16) >> 5;

        //row3
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row31 = ((thirty2 - v_deltaFract) * row31 + (v_deltaFract * row32) + 16) >> 5;

        //row4
        v_deltaPos += v_ipAngle;
        v_deltaFract = v_deltaPos & thirty1;
        row41 = ((thirty2 - v_deltaFract) * row41 + (v_deltaFract * row42) + 16) >> 5;

        // Flip the block

        if (modeHor)
        {
            Vec8s tmp1, tmp2, tmp3, tmp4;

            tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row11, row31);
            tmp2 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(row21, row41);

            tmp3 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(tmp1, tmp2);
            tmp4 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(tmp1, tmp2);

            //tmp16_1 = compress(tmp3, tmp3);
            store_partial(const_int(8), dst, tmp3);

            store_partial(const_int(8), dst + (2 * dstStride), tmp4);  //row2

            tmp3 = blend2q<1, 3>((Vec2q)tmp3, (Vec2q)tmp3);
            tmp4 = blend2q<1, 3>((Vec2q)tmp4, (Vec2q)tmp4);

            store_partial(const_int(8), dst + (3 * dstStride), tmp4);   //row3
            store_partial(const_int(8), dst + (dstStride), tmp3);       //row1
        }
        else
        {
            store_partial(const_int(8), dst, row11);
            store_partial(const_int(8), dst + (dstStride), row21);
            store_partial(const_int(8), dst + (2 * dstStride), row31);
            store_partial(const_int(8), dst + (3 * dstStride), row41);
        }
    }
}

#else /* if HIGH_BIT_DEPTH */

void PredIntraAng4_32(pixel* dst, int dstStride, pixel *refMain, int /*dirMode*/)
{
    Vec16uc tmp16_1, tmp16_2;

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 2);
    store_partial(const_int(4), dst, tmp16_1);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 3);
    store_partial(const_int(4), dst + dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 4);
    store_partial(const_int(4), dst + 2 * dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain + 5);
    store_partial(const_int(4), dst + 3 * dstStride, tmp16_2);
}

void PredIntraAng4_26(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    row41 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 32;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(4,5,6,7)

    v_deltaPos = v_ipAngle = 26;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_21(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row21;
    row32 = row22;

    row41 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    v_deltaPos = v_ipAngle = 21;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_17(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row12;

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row31 = row21;
    row32 = row22;

    row41 = row22;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(3,4,5,6)

    v_deltaPos = v_ipAngle = 17;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_13(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)

    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row12;                  //offsets(1,2,3,4)

    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(2,3,4,5)

    row41 = row31;                  //offsets(1,2,3,4)
    row42 = row32;

    v_deltaPos = v_ipAngle = 13;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_9(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row12;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);

    v_deltaPos = v_ipAngle = 9;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_5(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = 5;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_2(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain + 1);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = 2;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_2(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = -2;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_5(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);

    row11 = extend_low(tmp16_1);    //offsets(0,1,2,3)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(1,2,3,4)
    row21 = row11;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row11;
    row32 = row12;
    row41 = row11;
    row42 = row12;

    v_deltaPos = v_ipAngle = -5;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_9(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);

    row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row42;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row42;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row42;
    row32 = row12;

    v_deltaPos = v_ipAngle = -9;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_13(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 1);

    row41 = extend_low(tmp16_1);    //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row42;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row42;                  //offsets(0,1,2,3)
    row22 = row12;
    row31 = row41;
    row32 = row42;

    v_deltaPos = v_ipAngle = -13;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_17(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);

    row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row31 = row42;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row31;                  //offsets(-2,-1,0,1)
    row22 = row32;

    row11 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    v_deltaPos = v_ipAngle = -17;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_21(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 2);

    row41 = extend_low(tmp16_1);    //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row31 = row42;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row21 = row31;                  //offsets(-2,-1,0,1)
    row22 = row32;

    row11 = row32;
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    v_deltaPos = v_ipAngle = -21;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_26(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG4_START

        tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain - 3);

    row41 = extend_low(tmp16_1);    //offsets(-4,-3,-2,-1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 8;
    tmp16_2 = reinterpret_i(tmp2uq);
    row42 = extend_low(tmp16_2);    //offsets(-3,-2,-1,0)

    row31 = row42;                  //offsets(-3,-2,-1,0)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 16;
    tmp16_2 = reinterpret_i(tmp2uq);
    row32 = extend_low(tmp16_2);    //offsets(-2,-1,0,1)

    row21 = row32;                  //offsets(-2,-1,0,1)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 24;
    tmp16_2 = reinterpret_i(tmp2uq);
    row22 = extend_low(tmp16_2);    //offsets(-1,0,1,2)

    row11 = row22;                  //offsets(-1,0,1,2)
    tmp2uq = reinterpret_i(tmp16_1);
    tmp2uq = tmp2uq >> 32;
    tmp16_2 = reinterpret_i(tmp2uq);
    row12 = extend_low(tmp16_2);    //offsets(0,1,2,3)

    v_deltaPos = v_ipAngle = -26;

    PRED_INTRA_ANG4_END
}

void PredIntraAng4_m_32(pixel* dst, int dstStride, pixel *refMain, int /*dirMode*/)
{
    Vec16uc tmp16_1, tmp16_2;

    tmp16_1 = (Vec16uc)load_partial(const_int(8), refMain);    //-1,0,1,2
    store_partial(const_int(4), dst, tmp16_1);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 1); //-2,-1,0,1
    store_partial(const_int(4), dst + dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 2);
    store_partial(const_int(4), dst + 2 * dstStride, tmp16_2);
    tmp16_2 = (Vec16uc)load_partial(const_int(8), refMain - 3);
    store_partial(const_int(4), dst + 3 * dstStride, tmp16_2);
}

typedef void (*PredIntraAng4x4_table)(pixel* dst, int dstStride, pixel *refMain, int dirMode);
PredIntraAng4x4_table PredIntraAng4[] =
{
    /* PredIntraAng4_0 is replaced with PredIntraAng4_2. For PredIntraAng4_0 we are going through default path in the xPredIntraAng4x4 because we cannot afford to pass large number arguments for this function. */
    PredIntraAng4_32,
    PredIntraAng4_26,
    PredIntraAng4_21,
    PredIntraAng4_17,
    PredIntraAng4_13,
    PredIntraAng4_9,
    PredIntraAng4_5,
    PredIntraAng4_2,
    PredIntraAng4_2,    //Intentionally wrong! It should be "PredIntraAng4_0" here.
    PredIntraAng4_m_2,
    PredIntraAng4_m_5,
    PredIntraAng4_m_9,
    PredIntraAng4_m_13,
    PredIntraAng4_m_17,
    PredIntraAng4_m_21,
    PredIntraAng4_m_26,
    PredIntraAng4_m_32,
    PredIntraAng4_m_26,
    PredIntraAng4_m_21,
    PredIntraAng4_m_17,
    PredIntraAng4_m_13,
    PredIntraAng4_m_9,
    PredIntraAng4_m_5,
    PredIntraAng4_m_2,
    PredIntraAng4_2,    //Intentionally wrong! It should be "PredIntraAng4_0" here.
    PredIntraAng4_2,
    PredIntraAng4_5,
    PredIntraAng4_9,
    PredIntraAng4_13,
    PredIntraAng4_17,
    PredIntraAng4_21,
    PredIntraAng4_26,
    PredIntraAng4_32
};
void xPredIntraAng4x4(pixel* dst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    assert(dirMode > 1); //no planar and dc
    static const int mode_to_angle_table[] = { 32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32 };
    static const int mode_to_invAng_table[] = { 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256 };
    int intraPredAngle = mode_to_angle_table[dirMode - 2];
    int invAngle       = mode_to_invAng_table[dirMode - 2];

    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;

    // Do angular predictions
    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        int blkSize = width;
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (int k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for exactly vertical/horizontal modes
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(4), (void*)(refMain + 1));

            Vec16uc tmp16;
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(v_main, v_main);
            tmp16 = blend16c<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(tmp16, tmp16);
            Vec2uq tmp;

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0; // refSide[0] value in a vector
                v_temp = load_partial(const_int(8), (void*)refSide);
                v_side_0 = broadcast(const_int(0), (Vec8s)v_temp);
                v_side_0 = v_side_0 & 0x00FF;

                //shift v_side by 1 element (1 byte)
                tmp = reinterpret_i(v_temp);
                tmp = tmp >> 8;
                v_temp = reinterpret_i(tmp);
                Vec8s v_side = extend_low(v_temp);

                Vec8s row0 = extend_low(tmp16);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 += v_side;
                row0 = min(max(0, row0), 255);
                Vec16uc v_res(compress_unsafe(row0, 0));
                store_partial(const_int(4), dst, v_res);
            }
            else
            {
                store_partial(const_int(4), dst, tmp16);
            }

            tmp = (Vec2uq)tmp16;
            tmp >>= 32;
            store_partial(const_int(4), dst + dstStride, tmp);

            tmp = blend2q<1, 3>(reinterpret_i(tmp16), reinterpret_i(tmp16));
            store_partial(const_int(4), dst + (2 * dstStride), tmp);

            tmp >>= 32;
            store_partial(const_int(4), dst + (3 * dstStride), tmp);
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(4), refMain + 1);
            store_partial(const_int(4), dst, v_main);
            store_partial(const_int(4), dst + dstStride, v_main);
            store_partial(const_int(4), dst + (2 * dstStride), v_main);
            store_partial(const_int(4), dst + (3 * dstStride), v_main);
            if (bFilter)
            {
                for (int k = 0; k < 4; k++)
                {
                    dst[k * dstStride] = (pixel)Clip3((short)0, (short)((1 << 8) - 1), static_cast<short>((dst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
                }
            }
        }
    }
    else
    {
        PredIntraAng4[dirMode - 2](dst, dstStride, refMain, dirMode);
    }
}

#endif /* if HIGH_BIT_DEPTH */

#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(row11, row11, row12); \
        store_partial(const_int(8), dst + (X * dstStride), compress(row11, row11)); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11, GETAP(lookIdx, X)); \
        LOADROW(row12, GETAP(lookIdx, X) + 1); \
        CALCROW(rowx, row11, row12); \
}

// ROW is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROW, X) { \
        tmp = load_partial(const_int(8), refMain + 1 + X); \
        ROW = extend_low(tmp); \
}

#define CALCROW(RES, ROW1, ROW2) { \
        v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1; \
        RES = ((thirty2 - v_deltaFract) * ROW1 + (v_deltaFract * ROW2) + 16) >> 5; \
}

void PredIntraAng8_32(pixel* dst, int dstStride, pixel *refMain, int /*dirMode*/)
{
    Vec8s tmp;

    tmp = load_partial(const_int(8), refMain + 2);        //-1,0,1,2
    store_partial(const_int(8), dst, tmp);
    tmp = load_partial(const_int(8), refMain + 3);     //-2,-1,0,1
    store_partial(const_int(8), dst + dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 4);
    store_partial(const_int(8), dst + 2 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 5);
    store_partial(const_int(8), dst + 3 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 6);
    store_partial(const_int(8), dst + 4 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 7);
    store_partial(const_int(8), dst + 5 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 8);
    store_partial(const_int(8), dst + 6 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain + 9);
    store_partial(const_int(8), dst + 7 * dstStride, tmp);
}

void PredIntraAng8_26(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    // Map the mode index to main prediction direction and angle
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };

    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    if (modeHor)         // Near horizontal modes
    {
        Vec16uc tmp;
        Vec8s row11, row12;
        Vec16uc row1, row2, row3, row4, tmp16_1, tmp16_2;
        Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
        Vec8s tmp1, tmp2;
        v_deltaPos = 0;
        v_ipAngle = intraPredAngle;

        PREDANG_CALCROW_HOR(0, tmp1);
        PREDANG_CALCROW_HOR(1, tmp2);
        row1 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(2, tmp1);
        PREDANG_CALCROW_HOR(3, tmp2);
        row2 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(4, tmp1);
        PREDANG_CALCROW_HOR(5, tmp2);
        row3 = compress(tmp1, tmp2);
        PREDANG_CALCROW_HOR(6, tmp1);
        PREDANG_CALCROW_HOR(7, tmp2);
        row4 = compress(tmp1, tmp2);

        PRED_INTRA_ANG8_MIDDLE PREDANG_CALCROW_VER(0);
        PREDANG_CALCROW_VER(1);
        PREDANG_CALCROW_VER(2);
        PREDANG_CALCROW_VER(3);
        PREDANG_CALCROW_VER(4);
        PREDANG_CALCROW_VER(5);
        PREDANG_CALCROW_VER(6);
        PREDANG_CALCROW_VER(7);
    }
}

void PredIntraAng8_5(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG8_START LOADROW(row11, 0);
    LOADROW(row12, 1);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row1 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row2 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row3 = compress(tmp1, tmp2);
    row11 = row12;
    LOADROW(row12, 2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row4 = compress(tmp1, tmp2);

    PRED_INTRA_ANG8_MIDDLE LOADROW(row11, 0);
    LOADROW(row12, 1);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst, compress(tmp1, tmp1));
    store_partial(const_int(8), dst + dstStride, compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (2 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (3 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (4 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (5 * dstStride), compress(tmp2, tmp2));
    row11 = row12;
    LOADROW(row12, 2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (6 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (7 * dstStride), compress(tmp2, tmp2));
}
}

void PredIntraAng8_2(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG8_START LOADROW(row11, 0);
    LOADROW(row12, 1);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row1 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row2 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row3 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row4 = compress(tmp1, tmp2);

    PRED_INTRA_ANG8_MIDDLE LOADROW(row11, 0);
    LOADROW(row12, 1);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst, compress(tmp1, tmp1));
    store_partial(const_int(8), dst + dstStride, compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (2 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (3 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (4 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (5 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (6 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (7 * dstStride), compress(tmp2, tmp2));
}
}

void PredIntraAng8_m_2(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG8_START LOADROW(row11, -1);
    LOADROW(row12, 0);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row1 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row2 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row3 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row4 = compress(tmp1, tmp2);

    PRED_INTRA_ANG8_MIDDLE LOADROW(row11, -1);
    LOADROW(row12, 0);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst, compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (2 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (3 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (4 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (5 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (6 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (7 * dstStride), compress(tmp2, tmp2));
}
}

void PredIntraAng8_m_5(pixel* dst, int dstStride, pixel *refMain, int dirMode)
{
    PRED_INTRA_ANG8_START LOADROW(row11, -1);
    LOADROW(row12, 0);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row1 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row2 = compress(tmp1, tmp2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row3 = compress(tmp1, tmp2);
    row12 = row11;
    LOADROW(row11, -2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    row4 = compress(tmp1, tmp2);

    PRED_INTRA_ANG8_MIDDLE LOADROW(row11, -1);
    LOADROW(row12, 0);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst, compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (2 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (3 * dstStride), compress(tmp2, tmp2));
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (4 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (5 * dstStride), compress(tmp2, tmp2));
    row12 = row11;
    LOADROW(row11, -2);
    CALCROW(tmp1, row11, row12);
    CALCROW(tmp2, row11, row12);
    store_partial(const_int(8), dst + (6 * dstStride), compress(tmp1, tmp1));
    store_partial(const_int(8), dst + (7 * dstStride), compress(tmp2, tmp2));
}
}

void PredIntraAng8_m_32(pixel* dst, int dstStride, pixel *refMain, int /*dirMode*/)
{
    Vec16uc tmp;

    tmp = load_partial(const_int(8), refMain);        //-1,0,1,2
    store_partial(const_int(8), dst, tmp);
    tmp = load_partial(const_int(8), refMain - 1);     //-2,-1,0,1
    store_partial(const_int(8), dst + dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 2);
    store_partial(const_int(8), dst + 2 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 3);
    store_partial(const_int(8), dst + 3 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 4);
    store_partial(const_int(8), dst + 4 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 5);
    store_partial(const_int(8), dst + 5 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 6);
    store_partial(const_int(8), dst + 6 * dstStride, tmp);
    tmp = load_partial(const_int(8), refMain - 7);
    store_partial(const_int(8), dst + 7 * dstStride, tmp);
}

typedef void (*PredIntraAng8x8_table)(pixel* dst, int dstStride, pixel *refMain, int dirMode);
PredIntraAng8x8_table PredIntraAng8[] =
{
    /*
    PredIntraAng8_0 is replaced with PredIntraAng8_2. For PredIntraAng8_0 we are going through default path in the xPredIntraAng8x8 because we cannot afford to pass large number arguments for this function.
    Path for PredIntraAng8_21, PredIntraAng8_m_21, PredIntraAng8_17, PredIntraAng8_m_17, PredIntraAng8_13, PredIntraAng8_m_13, PredIntraAng8_9, PredIntraAng8_m_9 is same as PredIntraAng8_26.
    */
    PredIntraAng8_32,
    PredIntraAng8_26,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_21" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_9" here.
    PredIntraAng8_5,
    PredIntraAng8_2,
    PredIntraAng8_2,        //Intentionally wrong! It should be "PredIntraAng8_0" here.
    PredIntraAng8_m_2,
    PredIntraAng8_m_5,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_9" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_21" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_26" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_32" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_26" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_21" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_m_9" here.
    PredIntraAng8_m_5,
    PredIntraAng8_m_2,
    PredIntraAng8_2,        //Intentionally wrong! It should be "PredIntraAng8_0" here.
    PredIntraAng8_2,
    PredIntraAng8_5,
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_9" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_13" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_17" here.
    PredIntraAng8_26,       //Intentionally wrong! It should be "PredIntraAng8_21" here.
    PredIntraAng8_26,
    PredIntraAng8_32
};

void xPredIntraAng8x8(pixel* dst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    int k;
    int blkSize= width;

    assert(dirMode > 1); //no planar and dc
    static const int mode_to_angle_table[] = { 32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32 };
    static const int mode_to_invAng_table[] = { 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256 };
    int intraPredAngle = mode_to_angle_table[dirMode - 2];
    int invAngle       = mode_to_invAng_table[dirMode - 2];
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
        {
            invAngleSum += invAngle;
            refMain[k] = refSide[invAngleSum >> 8];
        }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp;
            Vec16uc tmp1;

            v_temp.load(refMain + 1);
            Vec8s v_main;
            v_main = extend_low(v_temp);

            if (bFilter)
            {
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector
                Vec16uc v_temp16;
                v_temp16.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp16);

                Vec8s row0;
                row0 = permute8s<0, 0, 0, 0, 0, 0, 0, 0>(v_main);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = row0 + v_side;
                row0 = min(max(0, row0), (1 << X265_DEPTH) - 1);

                tmp1 = compress(row0, row0);
                store_partial(const_int(8), dst, tmp1);            //row0
            }
            else
            {
                tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
                store_partial(const_int(8), dst, tmp1); //row0
            }
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (1 * dstStride), tmp1); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (2 * dstStride), tmp1); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (3 * dstStride), tmp1); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (4 * dstStride), tmp1); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (5 * dstStride), tmp1); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (6 * dstStride), tmp1); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, -256, -256, -256, -256, -256, -256, -256, -256>(v_temp);
            store_partial(const_int(8), dst + (7 * dstStride), tmp1); //row7
        }
        else
        {
            Vec16uc v_main;
            v_main = load_partial(const_int(8), refMain + 1);
            store_partial(const_int(8), dst, v_main);
            store_partial(const_int(8), dst + dstStride, v_main);
            store_partial(const_int(8), dst + (2 * dstStride), v_main);
            store_partial(const_int(8), dst + (3 * dstStride), v_main);
            store_partial(const_int(8), dst + (4 * dstStride), v_main);
            store_partial(const_int(8), dst + (5 * dstStride), v_main);
            store_partial(const_int(8), dst + (6 * dstStride), v_main);
            store_partial(const_int(8), dst + (7 * dstStride), v_main);

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

                v_temp.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp);

                v_temp.load(refMain + 1);
                Vec8s row0;
                row0 = permute16uc<0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1>(v_temp);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = row0 + v_side;
                row0 = min(max(0, row0), (1 << X265_DEPTH) - 1);

                dst[0 * dstStride] = row0[0];
                dst[1 * dstStride] = row0[1];
                dst[2 * dstStride] = row0[2];
                dst[3 * dstStride] = row0[3];
                dst[4 * dstStride] = row0[4];
                dst[5 * dstStride] = row0[5];
                dst[6 * dstStride] = row0[6];
                dst[7 * dstStride] = row0[7];
            }
        }
    }
    else
    {
        PredIntraAng8[dirMode - 2](dst, dstStride, refMain, dirMode);
    }
}

#undef PREDANG_CALCROW_VER
#undef PREDANG_CALCROW_HOR
#undef LOADROW
#undef CALCROW
#endif /* if HIGH_BIT_DEPTH */

//16x16
#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, X)); \
        LOADROW(row12L, row12H, GETAP(lookIdx, X) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        /*compress(row11L, row11H).store(dst + ((X)*dstStride));*/ \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride)), itmp); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        LOADROW(row11L, row11H, GETAP(lookIdx, (X))); \
        LOADROW(row12L, row12H, GETAP(lookIdx, (X)) + 1); \
        CALCROW(row11L, row11H, row11L, row11H, row12L, row12H); \
        /*rowx = compress(row11L, row11H);*/  \
        rowx = _mm_packus_epi16(row11L, row11H); \
}

// ROWL/H is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROWL, ROWH, X) { \
        /*tmp.load(refMain + 1 + (X)); */ \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (X))); \
        /* ROWL = extend_low(tmp);*/  \
        ROWL = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        /*ROWH = extend_high(tmp);*/  \
        ROWH = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
}

#define CALCROW(RESL, RESH, ROW1L, ROW1H, ROW2L, ROW2H) { \
        /*v_deltaPos += v_ipAngle; \
        v_deltaFract = v_deltaPos & thirty1;*/ \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        /*RESL = ((thirty2 - v_deltaFract) * ROW1L + (v_deltaFract * ROW2L) + 16) >> 5; \
        RESH = ((thirty2 - v_deltaFract) * ROW1H + (v_deltaFract * ROW2H) + 16) >> 5;*/ \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, ROW1L); \
        it3 = _mm_mullo_epi16(v_deltaFract, ROW2L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        RESL = _mm_srai_epi16(it2, 5); \
        \
        it2 = _mm_mullo_epi16(it1, ROW1H); \
        it3 = _mm_mullo_epi16(v_deltaFract, ROW2H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        RESH = _mm_srai_epi16(it2, 5); \
}

#define  BLND2_16(R1, R2) { \
        /*tmp1 = blend16uc<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23>(R1, R2); */ \
        itmp1 = _mm_unpacklo_epi8(R1, R2); \
        /*tmp2 = blend16uc<8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31>(R1, R2);*/ \
        itmp2 = _mm_unpackhi_epi8(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
}

#define MB4(R1, R2, R3, R4) { \
        BLND2_16(R1, R2) \
        BLND2_16(R3, R4) \
        /*tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R1, (Vec8s)R3);*/  \
        itmp1 = _mm_unpacklo_epi16(R1, R3); \
        /* tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R1, (Vec8s)R3);*/ \
        itmp2 = _mm_unpackhi_epi16(R1, R3); \
        R1 = itmp1; \
        R3 = itmp2; \
        /*R1 = tmp1; \
        R3 = tmp2;*/ \
        /*tmp1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>((Vec8s)R2, (Vec8s)R4); \
        tmp2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>((Vec8s)R2, (Vec8s)R4);*/ \
        itmp1 = _mm_unpacklo_epi16(R2, R4); \
        itmp2 = _mm_unpackhi_epi16(R2, R4); \
        R2 = itmp1; \
        R4 = itmp2; \
        /*R2 = tmp1; \
        R4 = tmp2;*/ \
}

#define BLND2_4(R1, R2) { \
        /* tmp1 = blend4i<0, 4, 1, 5>((Vec4i)R1, (Vec4i)R2); \
        tmp2 = blend4i<2, 6, 3, 7>((Vec4i)R1, (Vec4i)R2); */ \
        itmp1 = _mm_unpacklo_epi32(R1, R2); \
        itmp2 = _mm_unpackhi_epi32(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
        /*R1 = tmp1; \
        R2 = tmp2; */\
}

#define BLND2_2(R1, R2) { \
        /*tmp1 = blend2q<0, 2>((Vec2q)R1, (Vec2q)R2); \
        tmp2 = blend2q<1, 3>((Vec2q)R1, (Vec2q)R2);*/ \
        itmp1 = _mm_unpacklo_epi64(R1, R2); \
        itmp2 = _mm_unpackhi_epi64(R1, R2); \
        /*tmp1.store(dst); */ \
        _mm_storeu_si128((__m128i*)dst, itmp1); \
        dst += dstStride; \
        /*tmp2.store(dst);*/ \
        _mm_storeu_si128((__m128i*)dst, itmp2); \
        dst += dstStride; \
}

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) { \
        PREDANG_CALCROW_HOR(0 + X, R1) \
        PREDANG_CALCROW_HOR(1 + X, R2) \
        PREDANG_CALCROW_HOR(2 + X, R3) \
        PREDANG_CALCROW_HOR(3 + X, R4) \
        PREDANG_CALCROW_HOR(4 + X, R5) \
        PREDANG_CALCROW_HOR(5 + X, R6) \
        PREDANG_CALCROW_HOR(6 + X, R7) \
        PREDANG_CALCROW_HOR(7 + X, R8) \
        MB4(R1, R2, R3, R4) \
        MB4(R5, R6, R7, R8) \
        BLND2_4(R1, R5); \
        BLND2_4(R2, R6); \
        BLND2_4(R3, R7); \
        BLND2_4(R4, R8); \
}

void xPredIntraAng16x16(pixel* dst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove, bool bFilter = true)
{
    int k;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        if (intraPredAngle != -32)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp;
            Vec16uc tmp1;
            v_temp.load(refMain + 1);

            if (bFilter)
            {
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector
                Vec16uc v_temp16;
                v_temp16.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp16);

                Vec8s row01, row02, ref(refMain[1]);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row01 = ref + v_side;
                row01 = min(max(0, row01), (1 << X265_DEPTH) - 1);

                v_side = extend_high(v_temp16);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row02 = ref + v_side;
                row02 = min(max(0, row02), (1 << X265_DEPTH) - 1);

                tmp1 = compress_unsafe(row01, row02);
                tmp1.store(dst);            //row0
            }
            else
            {
                tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
                tmp1.store(dst); //row0
            }

            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(dst + (1 * dstStride)); //row1

            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(dst + (2 * dstStride)); //row2

            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(dst + (3 * dstStride)); //row3

            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(dst + (4 * dstStride)); //row4

            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(dst + (5 * dstStride)); //row5

            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(dst + (6 * dstStride)); //row6

            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(dst + (7 * dstStride)); //row7

            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(dst + (8 * dstStride)); //row8

            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(dst + (9 * dstStride)); //row9

            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(dst + (10 * dstStride)); //row10

            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(dst + (11 * dstStride)); //row11

            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(dst + (12 * dstStride)); //row12

            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(dst + (13 * dstStride)); //row13

            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(dst + (14 * dstStride)); //row14

            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(dst + (15 * dstStride)); //row15
        }
        else
        {
            Vec16uc v_main;
//            v_main.load(refMain + 1);
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 1));

            _mm_storeu_si128((__m128i*)dst, v_main);
            _mm_storeu_si128((__m128i*)(dst + dstStride), v_main);
            _mm_storeu_si128((__m128i*)(dst + (2 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (3 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (4 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (5 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (6 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (7 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (8 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (9 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (10 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (11 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (12 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (13 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (14 * dstStride)), v_main);
            _mm_storeu_si128((__m128i*)(dst + (15 * dstStride)), v_main);

            if (bFilter)
            {
                Vec16uc v_temp;
                Vec8s v_side_0(refSide[0]); // refSide[0] value in a vector

                v_temp.load(refSide + 1);
                Vec8s v_side;
                v_side = extend_low(v_temp);

                Vec8s row0, ref(refMain[1]);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = ref + v_side;
                row0 = min(max(0, row0), (1 << X265_DEPTH) - 1);

                dst[0 * dstStride] = row0[0];
                dst[1 * dstStride] = row0[1];
                dst[2 * dstStride] = row0[2];
                dst[3 * dstStride] = row0[3];
                dst[4 * dstStride] = row0[4];
                dst[5 * dstStride] = row0[5];
                dst[6 * dstStride] = row0[6];
                dst[7 * dstStride] = row0[7];

                v_side = extend_high(v_temp);
                v_side -= v_side_0;
                v_side = v_side >> 1;
                row0 = ref + v_side;
                row0 = min(max(0, row0), (1 << X265_DEPTH) - 1);
                dst[8 * dstStride] = row0[0];
                dst[9 * dstStride] = row0[1];
                dst[10 * dstStride] = row0[2];
                dst[11 * dstStride] = row0[3];
                dst[12 * dstStride] = row0[4];
                dst[13 * dstStride] = row0[5];
                dst[14 * dstStride] = row0[6];
                dst[15 * dstStride] = row0[7];
            }
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc v_refSide;
        v_refSide.load(refSide);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        pixel refMain0 = refMain[0];

        v_refSide.store(refMain - 15);
        refMain[0] = refMain0;

        Vec16uc tmp;
        __m128i itmp;
//        tmp.load(refMain);        //-1,0,1,2
//        tmp.store(dst);

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);

        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)--refMain);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);

/*
        tmp.load(--refMain);
        dst += dstStride;
        tmp.store(dst);
        ... 14 times more
*/
        return;
    }
    else if (intraPredAngle == 32)
    {
        Vec8s tmp;
        __m128i itmp;
        refMain += 2;

//        tmp.load(refMain++);
//        tmp.store(dst);

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        _mm_storeu_si128((__m128i*)dst, itmp);

/*
        tmp.load(refMain++);
        dst += dstStride;
        tmp.store(dst);
        ... 14 times more
*/
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);
        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        dst += dstStride;
        _mm_storeu_si128((__m128i*)dst, itmp);

        return;
    }
    else
    {
        if (modeHor)
        {
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec16uc R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;
            Vec16uc tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
            __m128i itmp, itmp1, itmp2, it1, it2, it3, i16;
//            MB16;
            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)
        }
        else
        {
            Vec8s row11L, row12L, row11H, row12H;
            Vec8s v_deltaFract, v_deltaPos, thirty2(32), thirty1(31), v_ipAngle;
            Vec16uc tmp;
            Vec8s tmp1, tmp2;
            v_deltaPos = 0;
            v_ipAngle = intraPredAngle;
            __m128i itmp, it1, it2, it3, i16;

            PREDANG_CALCROW_VER(0);
            PREDANG_CALCROW_VER(1);
            PREDANG_CALCROW_VER(2);
            PREDANG_CALCROW_VER(3);
            PREDANG_CALCROW_VER(4);
            PREDANG_CALCROW_VER(5);
            PREDANG_CALCROW_VER(6);
            PREDANG_CALCROW_VER(7);
            PREDANG_CALCROW_VER(8);
            PREDANG_CALCROW_VER(9);
            PREDANG_CALCROW_VER(10);
            PREDANG_CALCROW_VER(11);
            PREDANG_CALCROW_VER(12);
            PREDANG_CALCROW_VER(13);
            PREDANG_CALCROW_VER(14);
            PREDANG_CALCROW_VER(15);
        }
    }
}

#undef PREDANG_CALCROW_VER
#undef PREDANG_CALCROW_HOR
#undef LOADROW
#undef CALCROW
#undef BLND2_16
#undef BLND2_2
#undef BLND2_4
#undef MB4
#undef CALC_BLND_8ROWS
#endif /* if HIGH_BIT_DEPTH */

//32x32
#if HIGH_BIT_DEPTH
#else
#define PREDANG_CALCROW_VER(X) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)]))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 1))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride)), itmp); \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 16))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][(X)] + 17))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(row11L, row11H); \
        _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride) + 16), itmp); \
}

#define PREDANG_CALCROW_VER_MODE2(X) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11); \
        it3 = _mm_mullo_epi16(v_deltaFract, row21); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row12); \
        it3 = _mm_mullo_epi16(v_deltaFract, row22); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(res1, res2); \
        _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride)), itmp); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row13); \
        it3 = _mm_mullo_epi16(v_deltaFract, row23); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row14); \
        it3 = _mm_mullo_epi16(v_deltaFract, row24); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        itmp = _mm_packus_epi16(res1, res2); \
        _mm_storeu_si128((__m128i*)(dst + ((X)*dstStride) + 16), itmp); \
}

#define PREDANG_CALCROW_HOR(X, rowx) { \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][((X))]))); \
        row11L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row11H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (angAP[8 - (lookIdx)][((X))] + 1))); \
        row12L = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        row12H = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
  \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        row11L = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        row11H = _mm_srai_epi16(it2, 5); \
  \
        rowx = _mm_packus_epi16(row11L, row11H); \
}

#define PREDANG_CALCROW_HOR_MODE2(rowx) { \
        v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle); \
        v_deltaFract = _mm_and_si128(v_deltaPos, thirty1); \
        it1 = _mm_sub_epi16(thirty2, v_deltaFract); \
        it2 = _mm_mullo_epi16(it1, row11L); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12L); \
        it2 = _mm_add_epi16(it2, it3); \
        i16 = _mm_set1_epi16(16); \
        it2 = _mm_add_epi16(it2, i16); \
        res1 = _mm_srai_epi16(it2, 5); \
        it2 = _mm_mullo_epi16(it1, row11H); \
        it3 = _mm_mullo_epi16(v_deltaFract, row12H); \
        it2 = _mm_add_epi16(it2, it3); \
        it2 = _mm_add_epi16(it2, i16); \
        res2 = _mm_srai_epi16(it2, 5); \
  \
        rowx = _mm_packus_epi16(res1, res2); \
}

// ROWL/H is a Vec8s variable, X is the index in of data to be loaded
#define LOADROW(ROWL, ROWH, X) { \
/*        tmp.load(refMain + 1 + (X)); \
        ROWL = extend_low(tmp); \
        ROWH = extend_high(tmp); */\
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + (X))); \
        ROWL = _mm_unpacklo_epi8(itmp, _mm_setzero_si128()); \
        ROWH = _mm_unpackhi_epi8(itmp, _mm_setzero_si128()); \
}

#define BLND2_2(R1, R2) { \
/*        tmp1 = blend2q<0, 2>((Vec2q)R1, (Vec2q)R2); \
        tmp2 = blend2q<1, 3>((Vec2q)R1, (Vec2q)R2); \
        tmp1.store(dst);   dst += dstStride; \
        tmp2.store(dst);   dst += dstStride; */\
        itmp1 = _mm_unpacklo_epi64(R1, R2); \
        itmp2 = _mm_unpackhi_epi64(R1, R2); \
        _mm_storeu_si128((__m128i*)dst, itmp1); \
        dst += dstStride; \
        _mm_storeu_si128((__m128i*)dst, itmp2); \
        dst += dstStride; \
}

#define MB8(R1, R2, R3, R4, R5, R6, R7, R8) { \
        itmp1 = _mm_unpacklo_epi8(R1, R2); \
        itmp2 = _mm_unpackhi_epi8(R1, R2); \
        R1 = itmp1; \
        R2 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R3, R4); \
        itmp2 = _mm_unpackhi_epi8(R3, R4); \
        R3 = itmp1; \
        R4 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R1, R3); \
        itmp2 = _mm_unpackhi_epi16(R1, R3); \
        R1 = itmp1; \
        R3 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R2, R4); \
        itmp2 = _mm_unpackhi_epi16(R2, R4); \
        R2 = itmp1; \
        R4 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R5, R6); \
        itmp2 = _mm_unpackhi_epi8(R5, R6); \
        R5 = itmp1; \
        R6 = itmp2; \
        itmp1 = _mm_unpacklo_epi8(R7, R8); \
        itmp2 = _mm_unpackhi_epi8(R7, R8); \
        R7 = itmp1; \
        R8 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R5, R7); \
        itmp2 = _mm_unpackhi_epi16(R5, R7); \
        R5 = itmp1; \
        R7 = itmp2; \
        itmp1 = _mm_unpacklo_epi16(R6, R8); \
        itmp2 = _mm_unpackhi_epi16(R6, R8); \
        R6 = itmp1; \
        R8 = itmp2; \
        itmp1 = _mm_unpacklo_epi32(R1, R5); \
        itmp2 = _mm_unpackhi_epi32(R1, R5); \
        R1 = itmp1; \
        R5 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R2, R6); \
        itmp2 = _mm_unpackhi_epi32(R2, R6); \
        R2 = itmp1; \
        R6 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R3, R7); \
        itmp2 = _mm_unpackhi_epi32(R3, R7); \
        R3 = itmp1; \
        R7 = itmp2; \
  \
        itmp1 = _mm_unpacklo_epi32(R4, R8); \
        itmp2 = _mm_unpackhi_epi32(R4, R8); \
        R4 = itmp1; \
        R8 = itmp2; \
}

#define CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, X) { \
        PREDANG_CALCROW_HOR(0 + X, R1) \
        PREDANG_CALCROW_HOR(1 + X, R2) \
        PREDANG_CALCROW_HOR(2 + X, R3) \
        PREDANG_CALCROW_HOR(3 + X, R4) \
        PREDANG_CALCROW_HOR(4 + X, R5) \
        PREDANG_CALCROW_HOR(5 + X, R6) \
        PREDANG_CALCROW_HOR(6 + X, R7) \
}

#define CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8) { \
        PREDANG_CALCROW_HOR_MODE2(R1) \
        PREDANG_CALCROW_HOR_MODE2(R2) \
        PREDANG_CALCROW_HOR_MODE2(R3) \
        PREDANG_CALCROW_HOR_MODE2(R4) \
        PREDANG_CALCROW_HOR_MODE2(R5) \
        PREDANG_CALCROW_HOR_MODE2(R6) \
        PREDANG_CALCROW_HOR_MODE2(R7) \
}

void xPredIntraAng32x32(pixel* dst, int dstStride, int width, int dirMode, pixel *refLeft, pixel *refAbove)
{
    int k;
    int blkSize = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int lookIdx = intraPredAngle;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions

    pixel* refMain;
    pixel* refSide;

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
        refMain = (modeVer ? refAbove : refLeft);     // + (blkSize - 1);
        refSide = (modeVer ? refLeft : refAbove);     // + (blkSize - 1);

        // Extend the Main reference to the left.
        int invAngleSum    = 128;     // rounding for (shift by 8)
        if (intraPredAngle != -32)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
    }
    else
    {
        refMain = modeVer ? refAbove : refLeft;
        refSide = modeVer ? refLeft  : refAbove;
    }

    // bfilter will always be true for blocksize 8
    if (intraPredAngle == 0)  // Exactly hotizontal/vertical angles
    {
        if (modeHor)
        {
            Vec16uc v_temp, tmp1;

            v_temp.load(refMain + 1);
            /*BROADSTORE16ROWS;*/
            tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
            tmp1.store(dst + (0 * dstStride));
            tmp1.store(dst + (0 * dstStride) + 16);
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(dst + (1 * dstStride));
            tmp1.store(dst + (1 * dstStride) + 16);
            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(dst + (2 * dstStride));
            tmp1.store(dst + (2 * dstStride) + 16);
            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(dst + (3 * dstStride));
            tmp1.store(dst + (3 * dstStride) + 16);
            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(dst + (4 * dstStride));
            tmp1.store(dst + (4 * dstStride) + 16);
            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(dst + (5 * dstStride));
            tmp1.store(dst + (5 * dstStride) + 16);
            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(dst + (6 * dstStride));
            tmp1.store(dst + (6 * dstStride) + 16);
            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(dst + (7 * dstStride));
            tmp1.store(dst + (7 * dstStride) + 16);
            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(dst + (8 * dstStride));
            tmp1.store(dst + (8 * dstStride) + 16);
            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(dst + (9 * dstStride));
            tmp1.store(dst + (9 * dstStride) + 16);
            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(dst + (10 * dstStride));
            tmp1.store(dst + (10 * dstStride) + 16);
            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(dst + (11 * dstStride));
            tmp1.store(dst + (11 * dstStride) + 16);
            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(dst + (12 * dstStride));
            tmp1.store(dst + (12 * dstStride) + 16);
            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(dst + (13 * dstStride));
            tmp1.store(dst + (13 * dstStride) + 16);
            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(dst + (14 * dstStride));
            tmp1.store(dst + (14 * dstStride) + 16);
            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(dst + (15 * dstStride));
            tmp1.store(dst + (15 * dstStride) + 16);

            dst += 16 * dstStride;
            v_temp.load(refMain + 1 + 16);
            /*BROADSTORE16ROWS;*/
            tmp1 = permute16uc<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>(v_temp);
            tmp1.store(dst + (0 * dstStride));
            tmp1.store(dst + (0 * dstStride) + 16);
            tmp1 = permute16uc<1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(v_temp);
            tmp1.store(dst + (1 * dstStride));
            tmp1.store(dst + (1 * dstStride) + 16);
            tmp1 = permute16uc<2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>(v_temp);
            tmp1.store(dst + (2 * dstStride));
            tmp1.store(dst + (2 * dstStride) + 16);
            tmp1 = permute16uc<3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3>(v_temp);
            tmp1.store(dst + (3 * dstStride));
            tmp1.store(dst + (3 * dstStride) + 16);
            tmp1 = permute16uc<4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4>(v_temp);
            tmp1.store(dst + (4 * dstStride));
            tmp1.store(dst + (4 * dstStride) + 16);
            tmp1 = permute16uc<5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5>(v_temp);
            tmp1.store(dst + (5 * dstStride));
            tmp1.store(dst + (5 * dstStride) + 16);
            tmp1 = permute16uc<6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6>(v_temp);
            tmp1.store(dst + (6 * dstStride));
            tmp1.store(dst + (6 * dstStride) + 16);
            tmp1 = permute16uc<7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7>(v_temp);
            tmp1.store(dst + (7 * dstStride));
            tmp1.store(dst + (7 * dstStride) + 16);
            tmp1 = permute16uc<8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8>(v_temp);
            tmp1.store(dst + (8 * dstStride));
            tmp1.store(dst + (8 * dstStride) + 16);
            tmp1 = permute16uc<9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9>(v_temp);
            tmp1.store(dst + (9 * dstStride));
            tmp1.store(dst + (9 * dstStride) + 16);
            tmp1 = permute16uc<10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10>(v_temp);
            tmp1.store(dst + (10 * dstStride));
            tmp1.store(dst + (10 * dstStride) + 16);
            tmp1 = permute16uc<11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11>(v_temp);
            tmp1.store(dst + (11 * dstStride));
            tmp1.store(dst + (11 * dstStride) + 16);
            tmp1 = permute16uc<12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12>(v_temp);
            tmp1.store(dst + (12 * dstStride));
            tmp1.store(dst + (12 * dstStride) + 16);
            tmp1 = permute16uc<13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13>(v_temp);
            tmp1.store(dst + (13 * dstStride));
            tmp1.store(dst + (13 * dstStride) + 16);
            tmp1 = permute16uc<14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14>(v_temp);
            tmp1.store(dst + (14 * dstStride));
            tmp1.store(dst + (14 * dstStride) + 16);
            tmp1 = permute16uc<15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15>(v_temp);
            tmp1.store(dst + (15 * dstStride));
            tmp1.store(dst + (15 * dstStride) + 16);
        }
        else
        {
            __m128i v_main;
            Pel *dstOriginal = dst;
//            v_main.load(refMain + 1);
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 1));
//            v_main.store(dst);
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);

            dst = dstOriginal + 16;
            v_main = _mm_loadu_si128((__m128i const*)(refMain + 17));
//            v_main.store(dst);

            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
            dst += dstStride;
            _mm_storeu_si128((__m128i*)(dst), v_main);
        }
    }
    else if (intraPredAngle == -32)
    {
        Vec16uc v_refSide;
        pixel refMain0 = refMain[0];

        v_refSide.load(refSide);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        v_refSide.store(refMain - 15);

        v_refSide.load(refSide + 16);
        v_refSide = permute16uc<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>(v_refSide);
        v_refSide.store(refMain - 31);

        refMain[0] = refMain0;

        __m128i itmp;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst + (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 16));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain--;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);

        return;
    }
    else if (intraPredAngle == 32)
    {
        __m128i itmp;
        refMain += 2;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain++);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        _mm_storeu_si128((__m128i*)dst, itmp);
        dst += dstStride;
        refMain++;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        itmp = _mm_loadu_si128((__m128i const*)refMain);
        refMain++;
        _mm_storeu_si128((__m128i*)dst, itmp);
        _mm_storeu_si128((__m128i*)(dst - (16 * dstStride) + 16), itmp);
        itmp = _mm_loadu_si128((__m128i const*)(refMain + 15));
        _mm_storeu_si128((__m128i*)(dst + 16), itmp);
        dst += dstStride;

        return;
    }
    else
    {
        if (modeHor)
        {
            __m128i row11L, row12L, row11H, row12H, res1, res2;
            __m128i v_deltaFract, v_deltaPos, thirty2, thirty1, v_ipAngle;
            __m128i R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16;

            Pel * original_pDst = dst;
            v_deltaPos = _mm_setzero_si128(); //v_deltaPos = 0;
            v_ipAngle = _mm_set1_epi16(intraPredAngle);
            thirty2 = _mm_set1_epi16(32);
            thirty1 = _mm_set1_epi16(31);
            __m128i itmp, itmp1, itmp2, it1, it2, it3, i16;

            switch (intraPredAngle)
            {
            case -2:
                LOADROW(row11L, row11H, -1)
                LOADROW(row12L, row12H,  0)
                R16 = _mm_packus_epi16(row11L, row11H); //R16 = compress(row11L, row11H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row12L = row11L;
                row12H = row11H;
                LOADROW(row11L, row11H, -2)
                R16 = _mm_packus_epi16(row11L, row11H);
                dst = original_pDst + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                dst = original_pDst + (16 * dstStride);
                refMain += 16;

                v_deltaPos = _mm_setzero_si128();
                v_ipAngle = _mm_set1_epi16(intraPredAngle);
                LOADROW(row11L, row11H, -1)
                LOADROW(row12L, row12H,  0)
                R16 = _mm_packus_epi16(row11L, row11H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row12L = row11L;
                row12H = row11H;
                LOADROW(row11L, row11H, -2)
                R16 = _mm_packus_epi16(row11L, row11H);
                dst = original_pDst + (16 * dstStride) + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)
                return;

            case  2:
                LOADROW(row11L, row11H, 0)
                LOADROW(row12L, row12H, 1)
                R16 = _mm_packus_epi16(row12L, row12H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row11L = row12L;
                row11H = row12H;
                LOADROW(row12L, row12H, 2)
                R16 = _mm_packus_epi16(row12L, row12H);
                dst = original_pDst + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                dst = original_pDst + (16 * dstStride);
                refMain += 16;
                v_deltaPos = _mm_setzero_si128();

                v_ipAngle = _mm_set1_epi16(intraPredAngle);
                LOADROW(row11L, row11H, 0)
                LOADROW(row12L, row12H, 1)
                R16 = _mm_packus_epi16(row12L, row12H);

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                row11L = row12L;
                row11H = row12H;
                LOADROW(row12L, row12H, 2)
                R16 = _mm_packus_epi16(row12L, row12H);
                dst = original_pDst + (16 * dstStride) + 16;

                CALC_BLND_8ROWS_MODE2(R1, R2, R3, R4, R5, R6, R7, R8)
                PREDANG_CALCROW_HOR_MODE2(R8)
                MB8(R1, R2, R3, R4, R5, R6, R7, R8)
                CALC_BLND_8ROWS_MODE2(R9, R10, R11, R12, R13, R14, R15, R16)
                MB8(R9, R10, R11, R12, R13, R14, R15, R16)
                BLND2_2(R1, R9)
                BLND2_2(R5, R13)
                BLND2_2(R3, R11)
                BLND2_2(R7, R15)
                BLND2_2(R2, R10)
                BLND2_2(R6, R14)
                BLND2_2(R4, R12)
                BLND2_2(R8, R16)
                return;
            }

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            PREDANG_CALCROW_HOR(7 + 0, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            PREDANG_CALCROW_HOR(7 + 8, R16)
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)

            dst = original_pDst + 16;

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 16)
            PREDANG_CALCROW_HOR(7 + 16, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 24)
            R16 = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)

            dst = original_pDst + (16 * dstStride);
            refMain += 16;
            v_deltaPos = _mm_setzero_si128();
            v_ipAngle = _mm_set1_epi16(intraPredAngle);

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 0)
            PREDANG_CALCROW_HOR(7 + 0, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 8)
            PREDANG_CALCROW_HOR(7 + 8, R16)
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)
            dst = original_pDst + (16 * dstStride) + 16;

            CALC_BLND_8ROWS(R1, R2, R3, R4, R5, R6, R7, R8, 16)
            PREDANG_CALCROW_HOR(7 + 16, R8)
            MB8(R1, R2, R3, R4, R5, R6, R7, R8)
            CALC_BLND_8ROWS(R9, R10, R11, R12, R13, R14, R15, R16, 24)
            R16 = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            MB8(R9, R10, R11, R12, R13, R14, R15, R16)
            BLND2_2(R1, R9)
            BLND2_2(R5, R13)
            BLND2_2(R3, R11)
            BLND2_2(R7, R15)
            BLND2_2(R2, R10)
            BLND2_2(R6, R14)
            BLND2_2(R4, R12)
            BLND2_2(R8, R16)
        }
        else
        {
            __m128i row11L, row12L, row11H, row12H;
            __m128i v_deltaFract, v_deltaPos, thirty2, thirty1, v_ipAngle;
            __m128i row11, row12, row13, row14, row21, row22, row23, row24;
            __m128i res1, res2;

            v_deltaPos = _mm_setzero_si128(); //v_deltaPos = 0;
            v_ipAngle = _mm_set1_epi16(intraPredAngle);
            thirty2 = _mm_set1_epi16(32);
            thirty1 = _mm_set1_epi16(31);
            __m128i itmp, it1, it2, it3, i16;

            switch (intraPredAngle)
            {
            case -2:
                LOADROW(row11, row12, -1)
                LOADROW(row21, row22,  0)
                LOADROW(row13, row14, 15)
                LOADROW(row23, row24, 16)
                for (int i = 0; i <= 14; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                //deltaFract == 0 for 16th row
                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                itmp = _mm_packus_epi16(row11, row12);
                _mm_storeu_si128((__m128i*)(dst + ((15) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row13, row14);
                _mm_storeu_si128((__m128i*)(dst + ((15) * dstStride) + 16), itmp);

                row21 = row11;
                row22 = row12;
                row23 = row13;
                row24 = row14;

                LOADROW(row11, row12, -2)
                LOADROW(row13, row14, 14)
                for (int i = 16; i <= 30; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                itmp = _mm_packus_epi16(row11, row12);
                _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row13, row14);
                _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride) + 16), itmp);

                return;

            case  2:

                LOADROW(row11, row12, 0)
                LOADROW(row21, row22, 1)
                LOADROW(row13, row14, 16)
                LOADROW(row23, row24, 17)
                for (int i = 0; i <= 14; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                //deltaFract == 0 for 16th row

                v_deltaPos = _mm_add_epi16(v_deltaPos, v_ipAngle);
                v_deltaFract = _mm_and_si128(v_deltaPos, thirty1);
                itmp = _mm_packus_epi16(row21, row22);
                _mm_storeu_si128((__m128i*)(dst + ((15) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row23, row24);
                _mm_storeu_si128((__m128i*)(dst + ((15) * dstStride) + 16), itmp);

                row11 = row21;
                row12 = row22;
                row13 = row23;
                row14 = row24;

                LOADROW(row21, row22, 2)
                LOADROW(row23, row24, 18)
                for (int i = 16; i <= 30; i++)
                {
                    PREDANG_CALCROW_VER_MODE2(i);
                }

                itmp = _mm_packus_epi16(row21, row22);
                _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride)), itmp);
                itmp = _mm_packus_epi16(row23, row24);
                _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride) + 16), itmp);

                return;
            }

            for (int i = 0; i <= 30; i++)
            {
                PREDANG_CALCROW_VER(i);
            }

            itmp = _mm_loadu_si128((__m128i const*)(refMain + 1 + GETAP(lookIdx, 31)));
            _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride)), itmp);
            itmp = _mm_loadu_si128((__m128i const*)(refMain + 17 + GETAP(lookIdx, 31)));
            _mm_storeu_si128((__m128i*)(dst + ((31) * dstStride) + 16), itmp);
        }
    }
}

#endif /* if HIGH_BIT_DEPTH */

void intra_pred_ang(pixel* dst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove)
{
#if HIGH_BIT_DEPTH
#else
    switch (width)
    {
    case 4:
        xPredIntraAng4x4(dst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 8:
        xPredIntraAng8x8(dst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 16:
        xPredIntraAng16x16(dst, dstStride, width, dirMode, refLeft, refAbove, bFilter);
        return;
    case 32:
        xPredIntraAng32x32(dst, dstStride, width, dirMode, refLeft, refAbove);
        return;
    }

#endif /* if HIGH_BIT_DEPTH */

    int k, l;
    int blkSize        = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions
    {
        pixel* refMain;
        pixel* refSide;

        // Initialise the Main and Left reference array.
        if (intraPredAngle < 0)
        {
            refMain = (modeVer ? refAbove : refLeft); // + (blkSize - 1);
            refSide = (modeVer ? refLeft : refAbove); // + (blkSize - 1);

            // Extend the Main reference to the left.
            int invAngleSum    = 128; // rounding for (shift by 8)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
        }
        else
        {
            refMain = modeVer ? refAbove : refLeft;
            refSide = modeVer ? refLeft  : refAbove;
        }

        if (intraPredAngle == 0)
        {
            for (k = 0; k < blkSize; k++)
            {
                for (l = 0; l < blkSize; l++)
                {
                    dst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    dst[k * dstStride] = (pixel)Clip3(0, (1 << X265_DEPTH) - 1, static_cast<short>(dst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1));
                }
            }
        }
        else
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (k = 0; k < blkSize; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < blkSize; l++)
                    {
                        refMainIndex        = l + deltaInt + 1;
                        dst[k * dstStride + l] = (pixel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        dst[k * dstStride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (k = 0; k < blkSize - 1; k++)
            {
                for (l = k + 1; l < blkSize; l++)
                {
                    tmp                    = dst[k * dstStride + l];
                    dst[k * dstStride + l] = dst[l * dstStride + k];
                    dst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}

ALIGN_VAR_32(static const unsigned char, tab_angle_0[][16]) =
{
    { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 },         //  0
    { 15, 0, 0, 1, 2, 3, 4, 5, 7, 0, 0, 9, 10, 11, 12, 13 },    //  1
    { 12, 0, 0, 1, 2, 3, 4, 5, 3, 0, 0, 9, 10, 11, 12, 13 },    //  2
    { 15, 11, 12, 0, 0, 1, 2, 3, 7, 3, 4, 0, 0, 9, 10, 11 },    //  3
    { 13, 12, 11, 8, 8, 1, 2, 3, 5, 4, 3, 0, 0, 9, 10, 11 },    //  4
    { 9, 0, 0, 1, 2, 3, 4, 5, 1, 0, 0, 9, 10, 11, 12, 13 },     //  5
    { 11, 10, 9, 0, 0, 1, 2, 3, 4, 2, 1, 0, 0, 9, 10, 11 },     //  6
    { 15, 12, 11, 10, 9, 0, 0, 1, 7, 4, 3, 2, 1, 0, 0, 9 },     //  7
    { 0, 10, 11, 13, 1, 0, 10, 11, 3, 2, 0, 10, 5, 4, 2, 0 },    //  8

    { 1, 2, 2, 3, 3, 4, 4,  5,  5,  6,  6,  7,  7,  8,  8,  9 },    //  9
    { 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,  9,  9, 10 },    // 10
    { 3, 4, 4, 5, 5, 6, 6,  7,  7,  8,  8,  9,  9, 10, 10, 11 },    // 11
    { 4, 5, 5, 6, 6, 7, 7,  8,  8,  9,  9, 10, 10, 11, 11, 12 },    // 12
    { 5, 6, 6, 7, 7, 8, 8,  9,  9, 10, 10, 11, 11, 12, 12, 13 },    // 13
    { 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 14
    { 9, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,  6,  7 },    // 15
    { 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 16
    { 11, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7 },            // 17
    { 4, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 18
    { 14, 11, 11, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },           // 19
    { 7, 4, 4, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 20
    { 13, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7 },            // 21
    { 6, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14 },    // 22
    { 12, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 23
    { 5, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 24
    { 14, 12, 12, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 25
    { 7, 5, 5, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 26
    { 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 27
    { 4, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 28
    { 13, 11, 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 29
    { 6, 4, 4, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 30
    { 15, 13, 13, 11, 11, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4 },        // 31
    { 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6 },            // 32
    { 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13 },      // 33
    { 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5 },          // 34
    { 5, 3, 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12 },        // 35
    { 13, 12, 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3, 3, 4 },        // 36
    { 6, 5, 5, 3, 3, 2, 2, 0, 0, 8, 8, 9, 9, 10, 10, 11 },          // 37
    { 15, 13, 13, 12, 12, 10, 10, 9, 9, 0, 0, 1, 1, 2, 2, 3 },      // 38
    { 0, 7, 6, 5, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             // 39
    { 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },       // 40

    { 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 },       // 41
};

// TODO: Remove unused table and merge here
ALIGN_VAR_32(static const unsigned char, tab_angle_2[][16]) =
{
    { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },       //  0
};

ALIGN_VAR_32(static const char, tab_angle_1[][16]) =
{
#define MAKE_COEF8(a) \
    { 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a), 32 - (a), (a) \
    },

    MAKE_COEF8(0)
    MAKE_COEF8(1)
    MAKE_COEF8(2)
    MAKE_COEF8(3)
    MAKE_COEF8(4)
    MAKE_COEF8(5)
    MAKE_COEF8(6)
    MAKE_COEF8(7)
    MAKE_COEF8(8)
    MAKE_COEF8(9)
    MAKE_COEF8(10)
    MAKE_COEF8(11)
    MAKE_COEF8(12)
    MAKE_COEF8(13)
    MAKE_COEF8(14)
    MAKE_COEF8(15)
    MAKE_COEF8(16)
    MAKE_COEF8(17)
    MAKE_COEF8(18)
    MAKE_COEF8(19)
    MAKE_COEF8(20)
    MAKE_COEF8(21)
    MAKE_COEF8(22)
    MAKE_COEF8(23)
    MAKE_COEF8(24)
    MAKE_COEF8(25)
    MAKE_COEF8(26)
    MAKE_COEF8(27)
    MAKE_COEF8(28)
    MAKE_COEF8(29)
    MAKE_COEF8(30)
    MAKE_COEF8(31)

#undef MAKE_COEF8
};
}

namespace x265 {
void Setup_Vec_IPredPrimitives_sse3(EncoderPrimitives& p)
{
    p.intra_pred_dc = intra_pred_dc;
    p.intra_pred_planar = intra_pred_planar;
    p.intra_pred_ang = intra_pred_ang;
}
}
