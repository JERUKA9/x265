/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#include "primitives.h"
#include "common.h"
#include "motion.h"
#include "x265.h"
#include "TLibCommon/TComRom.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#if _MSC_VER
#pragma warning(disable: 4127) // conditional  expression is constant (macros use this construct)
#endif

using namespace x265;

namespace {

struct SubpelWorkload
{
    int hpel_iters;
    int hpel_dirs;
    int qpel_iters;
    int qpel_dirs;
    bool hpel_satd;
};

SubpelWorkload workload[X265_MAX_SUBPEL_LEVEL+1] = {
    { 1, 4, 0, 4, false }, // 4 SAD HPEL only
    { 1, 4, 1, 4, false }, // 4 SAD HPEL + 4 SATD QPEL
    { 1, 4, 1, 4, true },  // 4 SATD HPEL + 4 SATD QPEL
    { 2, 4, 1, 4, true },  // 2x4 SATD HPEL + 4 SATD QPEL
    { 2, 4, 2, 4, true },  // 2x4 SATD HPEL + 2x4 SATD QPEL
    { 1, 8, 1, 8, true },  // 8 SATD HPEL + 8 SATD QPEL (default)
    { 2, 8, 1, 8, true },  // 2x8 SATD HPEL + 8 SATD QPEL
    { 2, 8, 2, 8, true },  // 2x8 SATD HPEL + 2x8 SATD QPEL
};

}

static int size_scale[NUM_PARTITIONS];
#define SAD_THRESH(v) (bcost < (((v >> 4) * size_scale[partEnum])))

static void init_scales(void)
{
    int dims[] = { 4, 8, 12, 16, 24, 32, 48, 64 };

    int i = 0;

    for (size_t h = 0; h < sizeof(dims) / sizeof(int); h++)
    {
        for (size_t w = 0; w < sizeof(dims) / sizeof(int); w++)
        {
            size_scale[i++] = (dims[h] * dims[w]) >> 4;
        }
    }
}

MotionEstimate::MotionEstimate()
    : searchMethod(3)
    , subpelRefine(5)
{
    if (size_scale[0] == 0)
        init_scales();

    fenc = (pixel*)X265_MALLOC(pixel, MAX_CU_SIZE * MAX_CU_SIZE);
    subpelbuf = (pixel*)X265_MALLOC(pixel, (MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1));
    immedVal = (short*)X265_MALLOC(short, (MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1 + NTAPS_LUMA - 1));
}

MotionEstimate::~MotionEstimate()
{
    if (fenc)
        X265_FREE(fenc);
    if (subpelbuf)
        X265_FREE(subpelbuf);
    if (immedVal)
        X265_FREE(immedVal);
}

void MotionEstimate::setSourcePU(int offset, int width, int height)
{
    /* copy PU block into cache */
    primitives.blockcpy_pp(width, height, fenc, FENC_STRIDE, fencplane + offset, fencLumaStride);

    partEnum = PartitionFromSizes(width, height);
    sad = primitives.sad[partEnum];
    satd = primitives.satd[partEnum];
    sa8d = primitives.sa8d_inter[partEnum];
    sad_x3 = primitives.sad_x3[partEnum];
    sad_x4 = primitives.sad_x4[partEnum];

    blockwidth = width;
    blockheight = height;
    blockOffset = offset;
}

/* radius 2 hexagon. repeated entries are to avoid having to compute mod6 every time. */
static const MV hex2[8] = { MV(-1, -2), MV(-2, 0), MV(-1, 2), MV(1, 2), MV(2, 0), MV(1, -2), MV(-1, -2), MV(-2, 0) };
static const uint8_t mod6m1[8] = { 5, 0, 1, 2, 3, 4, 5, 0 };  /* (x-1)%6 */
static const MV square1[9] = { MV(0, 0), MV(0, -1), MV(0, 1), MV(-1, 0), MV(1, 0), MV(-1, -1), MV(-1, 1), MV(1, -1), MV(1, 1) };
static const int square1_dir[9] = { 0, 1, 1, 2, 2, 1, 1, 1, 1 };
static const MV hex4[16] =
{
    MV(0, -4),  MV(0, 4),  MV(-2, -3), MV(2, -3),
    MV(-4, -2), MV(4, -2), MV(-4, -1), MV(4, -1),
    MV(-4, 0),  MV(4, 0),  MV(-4, 1),  MV(4, 1),
    MV(-4, 2), MV(4, 2), MV(-2, 3), MV(2, 3),
};
static const MV offsets[] =
{
    MV(-1, 0), MV(0, -1),
    MV(-1, -1), MV(1, -1),
    MV(-1, 0), MV(1, 0),
    MV(-1, 1), MV(-1, -1),
    MV(1, -1), MV(1, 1),
    MV(-1, 0), MV(0, 1),
    MV(-1, 1), MV(1, 1),
    MV(1, 0), MV(0, 1),
}; // offsets for Two Point Search

/* sum of absolute differences between MV candidates */
static inline int x265_predictor_difference(const MV *mvc, intptr_t numCandidates)
{
    int sum = 0;

    for (int i = 0; i < numCandidates - 1; i++)
    {
        sum += abs(mvc[i].x - mvc[i + 1].x)
            +  abs(mvc[i].y - mvc[i + 1].y);
    }

    return sum;
}

#define COST_MV_PT_DIST(mx, my, point, dist) \
    do \
    { \
        MV tmv(mx, my); \
        int cost = sad(fenc, FENC_STRIDE, fref + mx + my * stride, stride); \
        cost += mvcost(tmv << 2); \
        if (cost < bcost) { \
            bcost = cost; \
            bmv = tmv; \
            bPointNr = point; \
            bDistance = dist; \
        } \
    } while (0)

#define COST_MV(mx, my) \
    do \
    { \
        int cost = sad(fenc, FENC_STRIDE, fref + mx + my * stride, stride); \
        cost += mvcost(MV(mx, my) << 2); \
        COPY2_IF_LT(bcost, cost, bmv, MV(mx, my)); \
    } while (0)

#define COST_MV_X3_DIR(m0x, m0y, m1x, m1y, m2x, m2y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x3(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               stride, costs); \
        (costs)[0] += mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] += mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] += mvcost((bmv + MV(m2x, m2y)) << 2); \
    }

#define COST_MV_PT_DIST_X4(m0x, m0y, p0, d0, m1x, m1y, p1, d1, m2x, m2y, p2, d2, m3x, m3y, p3, d3) \
    { \
        sad_x4(fenc, \
               fref + (m0x) + (m0y) * stride, \
               fref + (m1x) + (m1y) * stride, \
               fref + (m2x) + (m2y) * stride, \
               fref + (m3x) + (m3y) * stride, \
               stride, costs); \
        costs[0] += mvcost(MV(m0x, m0y) << 2); \
        costs[1] += mvcost(MV(m1x, m1y) << 2); \
        costs[2] += mvcost(MV(m2x, m2y) << 2); \
        costs[3] += mvcost(MV(m3x, m3y) << 2); \
        COPY4_IF_LT(bcost, costs[0], bmv, MV(m0x, m0y), bPointNr, p0, bDistance, d0); \
        COPY4_IF_LT(bcost, costs[1], bmv, MV(m1x, m1y), bPointNr, p1, bDistance, d1); \
        COPY4_IF_LT(bcost, costs[2], bmv, MV(m2x, m2y), bPointNr, p2, bDistance, d2); \
        COPY4_IF_LT(bcost, costs[3], bmv, MV(m3x, m3y), bPointNr, p3, bDistance, d3); \
    }

#define COST_MV_X4(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y) \
    { \
        sad_x4(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               stride, costs); \
        costs[0] += mvcost((omv + MV(m0x, m0y)) << 2); \
        costs[1] += mvcost((omv + MV(m1x, m1y)) << 2); \
        costs[2] += mvcost((omv + MV(m2x, m2y)) << 2); \
        costs[3] += mvcost((omv + MV(m3x, m3y)) << 2); \
        COPY2_IF_LT(bcost, costs[0], bmv, omv + MV(m0x, m0y)); \
        COPY2_IF_LT(bcost, costs[1], bmv, omv + MV(m1x, m1y)); \
        COPY2_IF_LT(bcost, costs[2], bmv, omv + MV(m2x, m2y)); \
        COPY2_IF_LT(bcost, costs[3], bmv, omv + MV(m3x, m3y)); \
    }

#define COST_MV_X4_DIR(m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs) \
    { \
        pixel *pix_base = fref + bmv.x + bmv.y * stride; \
        sad_x4(fenc, \
               pix_base + (m0x) + (m0y) * stride, \
               pix_base + (m1x) + (m1y) * stride, \
               pix_base + (m2x) + (m2y) * stride, \
               pix_base + (m3x) + (m3y) * stride, \
               stride, costs); \
        (costs)[0] += mvcost((bmv + MV(m0x, m0y)) << 2); \
        (costs)[1] += mvcost((bmv + MV(m1x, m1y)) << 2); \
        (costs)[2] += mvcost((bmv + MV(m2x, m2y)) << 2); \
        (costs)[3] += mvcost((bmv + MV(m3x, m3y)) << 2); \
    }

#define DIA1_ITER(mx, my) \
    { \
        omv.x = mx; omv.y = my; \
        COST_MV_X4(0, -1, 0, 1, -1, 0, 1, 0); \
    }

#define CROSS(start, x_max, y_max) \
    { \
        int16_t i = start; \
        if ((x_max) <= X265_MIN(mvmax.x - omv.x, omv.x - mvmin.x)) \
            for (; i < (x_max) - 2; i += 4) { \
                COST_MV_X4(i, 0, -i, 0, i + 2, 0, -i - 2, 0); } \
        for (; i < (x_max); i += 2) \
        { \
            if (omv.x + i <= mvmax.x) \
                COST_MV(omv.x + i, omv.y); \
            if (omv.x - i >= mvmin.x) \
                COST_MV(omv.x - i, omv.y); \
        } \
        i = start; \
        if ((y_max) <= X265_MIN(mvmax.y - omv.y, omv.y - mvmin.y)) \
            for (; i < (y_max) - 2; i += 4) { \
                COST_MV_X4(0, i, 0, -i, 0, i + 2, 0, -i - 2); } \
        for (; i < (y_max); i += 2) \
        { \
            if (omv.y + i <= mvmax.y) \
                COST_MV(omv.x, omv.y + i); \
            if (omv.y - i >= mvmin.y) \
                COST_MV(omv.x, omv.y - i); \
        } \
    }

int MotionEstimate::motionEstimate(ReferencePlanes *ref,
                                   const MV &       mvmin,
                                   const MV &       mvmax,
                                   const MV &       qmvp,
                                   int              numCandidates,
                                   const MV *       mvc,
                                   int              merange,
                                   MV &             outQMv)
{
    ALIGN_VAR_16(int, costs[16]);
    size_t stride = ref->lumaStride;
    pixel *fref = ref->fpelPlane + blockOffset;

    setMVP(qmvp);

    MV qmvmin = mvmin.toQPel();
    MV qmvmax = mvmax.toQPel();

    /* The term cost used here means satd/sad values for that particular search.
     * The costs used in ME integer search only includes the SAD cost of motion
     * residual and sqrtLambda times MVD bits.  The subpel refine steps use SATD
     * cost of residual and sqrtLambda * MVD bits.  Mode decision will be based
     * on video distortion cost (SSE/PSNR) plus lambda times all signaling bits
     * (mode + MVD bits). */

    // measure SAD cost at clipped QPEL MVP
    MV pmv = qmvp.clipped(qmvmin, qmvmax);
    MV bestpre = pmv;
    int bprecost = subpelCompare(ref, pmv, sad);

    /* re-measure full pel rounded MVP with SAD as search start point */
    MV bmv = pmv.roundToFPel();
    int bcost = bprecost;
    if (pmv.isSubpel())
    {
        bcost = sad(fenc, FENC_STRIDE, fref + bmv.x + bmv.y * stride, stride) + mvcost(bmv << 2);
    }

    // measure SAD cost at MV(0) if MVP is not zero
    if (pmv.notZero())
    {
        int cost = sad(fenc, FENC_STRIDE, fref, stride) + mvcost(MV(0, 0));
        if (cost < bcost)
        {
            bcost = cost;
            bmv = 0;
        }
    }

    // measure SAD cost at each QPEL motion vector candidate
    for (int i = 0; i < numCandidates; i++)
    {
        MV m = mvc[i].clipped(qmvmin, qmvmax);
        if (m.notZero() && m != pmv && m != bestpre) // check already measured
        {
            int cost = subpelCompare(ref, m, sad) + mvcost(m);
            if (cost < bprecost)
            {
                bprecost = cost;
                bestpre = m;
            }
        }
    }

    pmv = pmv.roundToFPel();
    MV omv = bmv;  // current search origin or starting point

    switch (searchMethod)
    {
    case X265_DIA_SEARCH:
    {
        /* diamond search, radius 1 */
        bcost <<= 4;
        int i = merange;
        do
        {
            COST_MV_X4_DIR(0, -1, 0, 1, -1, 0, 1, 0, costs);
            COPY1_IF_LT(bcost, (costs[0] << 4) + 1);
            COPY1_IF_LT(bcost, (costs[1] << 4) + 3);
            COPY1_IF_LT(bcost, (costs[2] << 4) + 4);
            COPY1_IF_LT(bcost, (costs[3] << 4) + 12);
            if (!(bcost & 15))
                break;
            bmv.x -= (bcost << 28) >> 30;
            bmv.y -= (bcost << 30) >> 30;
            bcost &= ~15;
        }
        while (--i && bmv.checkRange(mvmin, mvmax));
        bcost >>= 4;
        break;
    }

    case X265_HEX_SEARCH:
    {
me_hex2:
        /* hexagon search, radius 2 */
#if 0
        for (int i = 0; i < merange / 2; i++)
        {
            omv = bmv;
            COST_MV(omv.x - 2, omv.y);
            COST_MV(omv.x - 1, omv.y + 2);
            COST_MV(omv.x + 1, omv.y + 2);
            COST_MV(omv.x + 2, omv.y);
            COST_MV(omv.x + 1, omv.y - 2);
            COST_MV(omv.x - 1, omv.y - 2);
            if (omv == bmv)
                break;
            if (!bmv.checkRange(mvmin, mvmax))
                break;
        }

#else // if 0
      /* equivalent to the above, but eliminates duplicate candidates */
        COST_MV_X3_DIR(-2, 0, -1, 2,  1, 2, costs);
        COST_MV_X3_DIR(2, 0,  1, -2, -1, -2, costs + 3);
        bcost <<= 3;
        COPY1_IF_LT(bcost, (costs[0] << 3) + 2);
        COPY1_IF_LT(bcost, (costs[1] << 3) + 3);
        COPY1_IF_LT(bcost, (costs[2] << 3) + 4);
        COPY1_IF_LT(bcost, (costs[3] << 3) + 5);
        COPY1_IF_LT(bcost, (costs[4] << 3) + 6);
        COPY1_IF_LT(bcost, (costs[5] << 3) + 7);

        if (bcost & 7)
        {
            int dir = (bcost & 7) - 2;
            bmv += hex2[dir + 1];

            /* half hexagon, not overlapping the previous iteration */
            for (int i = (merange >> 1) - 1; i > 0 && bmv.checkRange(mvmin, mvmax); i--)
            {
                COST_MV_X3_DIR(hex2[dir + 0].x, hex2[dir + 0].y,
                               hex2[dir + 1].x, hex2[dir + 1].y,
                               hex2[dir + 2].x, hex2[dir + 2].y,
                               costs);
                bcost &= ~7;
                COPY1_IF_LT(bcost, (costs[0] << 3) + 1);
                COPY1_IF_LT(bcost, (costs[1] << 3) + 2);
                COPY1_IF_LT(bcost, (costs[2] << 3) + 3);
                if (!(bcost & 7))
                    break;
                dir += (bcost & 7) - 2;
                dir = mod6m1[dir + 1];
                bmv += hex2[dir + 1];
            }
        }
        bcost >>= 3;
#endif // if 0

        /* square refine */
        int dir = 0;
        COST_MV_X4_DIR(0, -1,  0, 1, -1, 0, 1, 0, costs);
        COPY2_IF_LT(bcost, costs[0], dir, 1);
        COPY2_IF_LT(bcost, costs[1], dir, 2);
        COPY2_IF_LT(bcost, costs[2], dir, 3);
        COPY2_IF_LT(bcost, costs[3], dir, 4);
        COST_MV_X4_DIR(-1, -1, -1, 1, 1, -1, 1, 1, costs);
        COPY2_IF_LT(bcost, costs[0], dir, 5);
        COPY2_IF_LT(bcost, costs[1], dir, 6);
        COPY2_IF_LT(bcost, costs[2], dir, 7);
        COPY2_IF_LT(bcost, costs[3], dir, 8);
        bmv += square1[dir];
        break;
    }

    case X265_UMH_SEARCH:
    {
        int ucost1, ucost2;
        int16_t cross_start = 1;
        assert(PARTITION_4x4 != partEnum);

        /* refine predictors */
        omv = bmv;
        pixel *pix_base = fref + omv.x + omv.y * stride;
        ucost1 = bcost;
        DIA1_ITER(pmv.x, pmv.y);
        if (pmv.notZero())
            DIA1_ITER(0, 0);

        ucost2 = bcost;
        if (bmv.notZero() && bmv != pmv)
            DIA1_ITER(bmv.x, bmv.y);
        if (bcost == ucost2)
            cross_start = 3;

        /* Early Termination */
        omv = bmv;
        if (bcost == ucost2 && SAD_THRESH(2000))
        {
            COST_MV_X4(0, -2, -1, -1, 1, -1, -2, 0);
            COST_MV_X4(2, 0, -1, 1, 1, 1,  0, 2);
            if (bcost == ucost1 && SAD_THRESH(500))
                break;
            if (bcost == ucost2)
            {
                int16_t range = (int16_t)(merange >> 1) | 1;
                CROSS(3, range, range);
                COST_MV_X4(-1, -2, 1, -2, -2, -1, 2, -1);
                COST_MV_X4(-2, 1, 2, 1, -1, 2, 1, 2);
                if (bcost == ucost2)
                    break;
                cross_start = range + 2;
            }
        }

        // TODO: Need to study x264's logic for building mvc list to understand why they
        //       have special cases here for 16x16, and whether they apply to HEVC CTU

        // adaptive search range based on mvc variability
        if (numCandidates)
        {
            /* range multipliers based on casual inspection of some statistics of
             * average distance between current predictor and final mv found by ESA.
             * these have not been tuned much by actual encoding. */
            static const uint8_t range_mul[4][4] =
            {
                { 3, 3, 4, 4 },
                { 3, 4, 4, 4 },
                { 4, 4, 4, 5 },
                { 4, 4, 5, 6 },
            };

            int mvd;
            int sad_ctx, mvd_ctx;
            int denom = 1;

            if (numCandidates == 1)
            {
                if (PARTITION_64x64 == partEnum)
                    /* mvc is probably the same as mvp, so the difference isn't meaningful.
                     * but prediction usually isn't too bad, so just use medium range */
                    mvd = 25;
                else
                    mvd = abs(qmvp.x - mvc[0].x) + abs(qmvp.y - mvc[0].y);
            }
            else
            {
                /* calculate the degree of agreement between predictors. */

                /* in 64x64, mvc includes all the neighbors used to make mvp,
                 * so don't count mvp separately. */

                denom = numCandidates - 1;
                mvd = 0;
                if (partEnum != PARTITION_64x64)
                {
                    mvd = abs(qmvp.x - mvc[0].x) + abs(qmvp.y - mvc[0].y);
                    denom++;
                }
                mvd += x265_predictor_difference(mvc, numCandidates);
            }

            sad_ctx = SAD_THRESH(1000) ? 0
                : SAD_THRESH(2000) ? 1
                : SAD_THRESH(4000) ? 2 : 3;
            mvd_ctx = mvd < 10 * denom ? 0
                : mvd < 20 * denom ? 1
                : mvd < 40 * denom ? 2 : 3;

            merange = (merange * range_mul[mvd_ctx][sad_ctx]) >> 2;
        }

        /* FIXME if the above DIA2/OCT2/CROSS found a new mv, it has not updated omx/omy.
         * we are still centered on the same place as the DIA2. is this desirable? */
        CROSS(cross_start, merange, merange >> 1);
        COST_MV_X4(-2, -2, -2, 2, 2, -2, 2, 2);

        /* hexagon grid */
        omv = bmv;
        const uint16_t *p_cost_omvx = m_cost_mvx + omv.x * 4;
        const uint16_t *p_cost_omvy = m_cost_mvy + omv.y * 4;
        int16_t i = 1;
        do
        {
            if (4 * i > X265_MIN4(mvmax.x - omv.x, omv.x - mvmin.x,
                                  mvmax.y - omv.y, omv.y - mvmin.y))
            {
                for (int j = 0; j < 16; j++)
                {
                    MV mv = omv + (hex4[j] * i);
                    if (mv.checkRange(mvmin, mvmax))
                        COST_MV(mv.x, mv.y);
                }
            }
            else
            {
                int16_t dir = 0;
                pixel *fref_base = fref + omv.x + (omv.y - 4 * i) * stride;
                size_t dy = (size_t)i * stride;
#define SADS(k, x0, y0, x1, y1, x2, y2, x3, y3) \
    sad_x4(fenc, \
           fref_base x0 * i + (y0 - 2 * k + 4) * dy, \
           fref_base x1 * i + (y1 - 2 * k + 4) * dy, \
           fref_base x2 * i + (y2 - 2 * k + 4) * dy, \
           fref_base x3 * i + (y3 - 2 * k + 4) * dy, \
           stride, costs + 4 * k); \
    fref_base += 2 * dy;
#define ADD_MVCOST(k, x, y) costs[k] += p_cost_omvx[x * 4 * i] + p_cost_omvy[y * 4 * i]
#define MIN_MV(k, x, y)     COPY2_IF_LT(bcost, costs[k], dir, x * 16 + (y & 15))

                SADS(0, +0, -4, +0, +4, -2, -3, +2, -3);
                SADS(1, -4, -2, +4, -2, -4, -1, +4, -1);
                SADS(2, -4, +0, +4, +0, -4, +1, +4, +1);
                SADS(3, -4, +2, +4, +2, -2, +3, +2, +3);
                ADD_MVCOST(0, 0, -4);
                ADD_MVCOST(1, 0, 4);
                ADD_MVCOST(2, -2, -3);
                ADD_MVCOST(3, 2, -3);
                ADD_MVCOST(4, -4, -2);
                ADD_MVCOST(5, 4, -2);
                ADD_MVCOST(6, -4, -1);
                ADD_MVCOST(7, 4, -1);
                ADD_MVCOST(8, -4, 0);
                ADD_MVCOST(9, 4, 0);
                ADD_MVCOST(10, -4, 1);
                ADD_MVCOST(11, 4, 1);
                ADD_MVCOST(12, -4, 2);
                ADD_MVCOST(13, 4, 2);
                ADD_MVCOST(14, -2, 3);
                ADD_MVCOST(15, 2, 3);
                MIN_MV(0, 0, -4);
                MIN_MV(1, 0, 4);
                MIN_MV(2, -2, -3);
                MIN_MV(3, 2, -3);
                MIN_MV(4, -4, -2);
                MIN_MV(5, 4, -2);
                MIN_MV(6, -4, -1);
                MIN_MV(7, 4, -1);
                MIN_MV(8, -4, 0);
                MIN_MV(9, 4, 0);
                MIN_MV(10, -4, 1);
                MIN_MV(11, 4, 1);
                MIN_MV(12, -4, 2);
                MIN_MV(13, 4, 2);
                MIN_MV(14, -2, 3);
                MIN_MV(15, 2, 3);
#undef SADS
#undef ADD_MVCOST
#undef MIN_MV
                if (dir)
                {
                    bmv.x = omv.x + i * (dir >> 4);
                    bmv.y = omv.y + i * ((dir << 28) >> 28);
                }
            }
        }
        while (++i <= merange >> 2);
        if (bmv.checkRange(mvmin, mvmax))
            goto me_hex2;
        break;
    }

    case X265_STAR_SEARCH: // Adapted from HM ME
    {
        int bPointNr = 0;
        int bDistance = 0;

        const int EarlyExitIters = 3;
        StarPatternSearch(ref, mvmin, mvmax, bmv, bcost, bPointNr, bDistance, EarlyExitIters, merange);
        if (bDistance == 1)
        {
            // if best distance was only 1, check two missing points.  If no new point is found, stop
            if (bPointNr)
            {
                /* For a given direction 1 to 8, check nearest two outer X pixels
                     X   X
                   X 1 2 3 X
                     4 * 5
                   X 6 7 8 X
                     X   X
                */
                int saved = bcost;
                const MV mv1 = bmv + offsets[(bPointNr - 1) * 2];
                const MV mv2 = bmv + offsets[(bPointNr - 1) * 2 + 1];
                if (mv1.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv1.x, mv1.y);
                }
                if (mv2.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv2.x, mv2.y);
                }
                if (bcost == saved)
                    break;
            }
            else
                break;
        }

        const int RasterDistance = 5;
        if (bDistance > RasterDistance)
        {
            // raster search refinement if original search distance was too big
            MV tmv;
            for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y += RasterDistance)
            {
                for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x += RasterDistance)
                {
                    if (tmv.x + (RasterDistance * 3) <= mvmax.x)
                    {
                        pixel *pix_base = fref + tmv.y * stride + tmv.x;
                        sad_x4(fenc,
                               pix_base,
                               pix_base + RasterDistance,
                               pix_base + RasterDistance * 2,
                               pix_base + RasterDistance * 3,
                               stride, costs);
                        costs[0] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[0], bmv, tmv);
                        tmv.x += RasterDistance;
                        costs[1] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[1], bmv, tmv);
                        tmv.x += RasterDistance;
                        costs[2] += mvcost(tmv << 2);
                        COPY2_IF_LT(bcost, costs[2], bmv, tmv);
                        tmv.x += RasterDistance;
                        costs[3] += mvcost(tmv << 3);
                        COPY2_IF_LT(bcost, costs[3], bmv, tmv);
                    }
                    else
                        COST_MV(tmv.x, tmv.y);
                }
            }
        }

        while (bDistance > 0)
        {
            // center a new search around current best
            bDistance = 0;
            bPointNr = 0;
            const int MaxIters = 32;
            StarPatternSearch(ref, mvmin, mvmax, bmv, bcost, bPointNr, bDistance, MaxIters, merange);

            if (bDistance == 1)
            {
                if (!bPointNr)
                    break;

                /* For a given direction 1 to 8, check nearest 2 outer X pixels
                        X   X
                    X 1 2 3 X
                        4 * 5
                    X 6 7 8 X
                        X   X
                */
                const MV mv1 = bmv + offsets[(bPointNr - 1) * 2];
                const MV mv2 = bmv + offsets[(bPointNr - 1) * 2 + 1];
                if (mv1.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv1.x, mv1.y);
                }
                if (mv2.checkRange(mvmin, mvmax))
                {
                    COST_MV(mv2.x, mv2.y);
                }
                break;
            }
        }
    }
    break;
    case X265_FULL_SEARCH:
    {
        // dead slow exhaustive search, but at least it uses sad_x4()
        MV tmv;
        for (tmv.y = mvmin.y; tmv.y <= mvmax.y; tmv.y++)
        {
            for (tmv.x = mvmin.x; tmv.x <= mvmax.x; tmv.x++)
            {
                if (tmv.x + 3 <= mvmax.x)
                {
                    pixel *pix_base = fref + tmv.y * stride + tmv.x;
                    sad_x4(fenc,
                           pix_base,
                           pix_base + 1,
                           pix_base + 2,
                           pix_base + 3,
                           stride, costs);
                    costs[0] += mvcost(tmv << 2);
                    COPY2_IF_LT(bcost, costs[0], bmv, tmv);
                    tmv.x++;
                    costs[1] += mvcost(tmv << 2);
                    COPY2_IF_LT(bcost, costs[1], bmv, tmv);
                    tmv.x++;
                    costs[2] += mvcost(tmv << 2);
                    COPY2_IF_LT(bcost, costs[2], bmv, tmv);
                    tmv.x++;
                    costs[3] += mvcost(tmv << 2);
                    COPY2_IF_LT(bcost, costs[3], bmv, tmv);
                }
                else
                    COST_MV(tmv.x, tmv.y);
            }
        }
    }

    default:
        assert(0);
        break;
    }

    if (bprecost < bcost)
    {
        bmv = bestpre;
        bcost = bprecost;
    }
    else
        bmv = bmv.toQPel(); // promote search bmv to qpel

    SubpelWorkload& wl = workload[this->subpelRefine];
    pixelcmp_t hpelcomp;

    if (wl.hpel_satd)
    {
        bcost = subpelCompare(ref, bmv, satd) + mvcost(bmv);
        hpelcomp = satd;
    }
    else
        hpelcomp = sad;

    if (ref->isLowres)
    {
        for (int iter = 0; iter < wl.hpel_iters; iter++)
        {
            int bdir = 0, cost;
            for (int i = 1; i <= wl.hpel_dirs; i++)
            {
                MV qmv = bmv + square1[i] * 2;
                cost = subpelCompare(ref, qmv, hpelcomp) + mvcost(qmv);
                COPY2_IF_LT(bcost, cost, bdir, i+0);
            }
            bmv += square1[bdir] * 2;
        }
    }
    else
    {
        for (int iter = 0; iter < wl.hpel_iters; iter++)
        {
            int bdir = 0, cost0, cost1;
            for (int i = 1; i <= wl.hpel_dirs; i += 2)
            {
                MV qmv0 = bmv + square1[i  ] * 2;
                MV qmv1 = bmv + square1[i+1] * 2;
                int mvcost0 = mvcost(qmv0);
                int mvcost1 = mvcost(qmv1);
                int dir = square1_dir[i];

                pixel *fqref = ref->fpelPlane + blockOffset + (qmv0.x >> 2) + (qmv0.y >> 2) * ref->lumaStride;
                int xFrac = qmv0.x & 0x3;
                int yFrac = qmv0.y & 0x3;

                // TODO: sad_x2
                if (xFrac == 0 && yFrac == 0)
                {
                    intptr_t offset = (dir == 2) + (dir == 1 ? ref->lumaStride : 0);
                    cost0 = hpelcomp(fenc, FENC_STRIDE, fqref, ref->lumaStride) + mvcost0;
                    cost1 = hpelcomp(fenc, FENC_STRIDE, fqref + offset, ref->lumaStride) + mvcost1;
                }
                else
                {
                    subpelInterpolate(fqref, ref->lumaStride, xFrac, yFrac, dir);
                    cost0 = hpelcomp(fenc, FENC_STRIDE, subpelbuf, FENC_STRIDE + (dir == 2)) + mvcost0;
                    cost1 = hpelcomp(fenc, FENC_STRIDE, subpelbuf + (dir == 2) + (dir == 1 ? FENC_STRIDE : 0), FENC_STRIDE + (dir == 2)) + mvcost1;
                }
                COPY2_IF_LT(bcost, cost0, bdir, i+0);
                COPY2_IF_LT(bcost, cost1, bdir, i+1);
            }
            bmv += square1[bdir] * 2;
        }
    }
    /* if HPEL search used SAD, remeasure with SATD before QPEL */
    if (!wl.hpel_satd)
        bcost = subpelCompare(ref, bmv, satd) + mvcost(bmv);

    for (int iter = 0; iter < wl.qpel_iters; iter++)
    {
        int bdir = 0, cost;
        for (int i = 1; i <= wl.qpel_dirs; i++)
        {
            MV qmv = bmv + square1[i];
            cost = subpelCompare(ref, qmv, satd) + mvcost(qmv);
            COPY2_IF_LT(bcost, cost, bdir, i);
        }
        bmv += square1[bdir];
    }

    x265_emms();
    outQMv = bmv;
    return bcost;
}

void MotionEstimate::StarPatternSearch(ReferencePlanes *ref,
                                       const MV &       mvmin,
                                       const MV &       mvmax,
                                       MV &             bmv,
                                       int &            bcost,
                                       int &            bPointNr,
                                       int &            bDistance,
                                       int              earlyExitIters,
                                       int              merange)
{
    ALIGN_VAR_16(int, costs[16]);
    pixel *fref = ref->fpelPlane + blockOffset;
    size_t stride = ref->lumaStride;

    MV omv = bmv;
    int saved = bcost;
    int rounds = 0;

    {
        int16_t dist = 1;

        /* bPointNr
              2
            4 * 5
              7
         */
        const int16_t top    = omv.y - dist;
        const int16_t bottom = omv.y + dist;
        const int16_t left   = omv.x - dist;
        const int16_t right  = omv.x + dist;

        if (top >= mvmin.y && left >= mvmin.x && right <= mvmax.x && bottom <= mvmax.y)
        {
            COST_MV_PT_DIST_X4(omv.x,  top,    2, dist,
                               left,  omv.y,   4, dist,
                               right, omv.y,   5, dist,
                               omv.x,  bottom, 7, dist);
        }
        else
        {
            if (top >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, top, 2, dist);
            }
            if (left >= mvmin.x) // check middle left
            {
                COST_MV_PT_DIST(left, omv.y, 4, dist);
            }
            if (right <= mvmax.x) // check middle right
            {
                COST_MV_PT_DIST(right, omv.y, 5, dist);
            }
            if (bottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, bottom, 7, dist);
            }
        }
        if (bcost < saved)
            rounds = 0;
        else if (++rounds >= earlyExitIters)
            return;
    }

    for (int16_t dist = 2; dist <= 8; dist <<= 1)
    {
        /* bPointNr
              2
             1 3
            4 * 5
             6 8
              7
         Points 2, 4, 5, 7 are dist
         Points 1, 3, 6, 8 are dist>>1
         */
        const int16_t top     = omv.y - dist;
        const int16_t bottom  = omv.y + dist;
        const int16_t left    = omv.x - dist;
        const int16_t right   = omv.x + dist;
        const int16_t top2    = omv.y - (dist >> 1);
        const int16_t bottom2 = omv.y + (dist >> 1);
        const int16_t left2   = omv.x - (dist >> 1);
        const int16_t right2  = omv.x + (dist >> 1);
        saved = bcost;

        if (top >= mvmin.y && left >= mvmin.x &&
            right <= mvmax.x && bottom <= mvmax.y) // check border
        {
            COST_MV_PT_DIST_X4(omv.x,  top,   2, dist,
                               left2,  top2,  1, dist >> 1,
                               right2, top2,  3, dist >> 1,
                               left,   omv.y, 4, dist);
            COST_MV_PT_DIST_X4(right,  omv.y,   5, dist,
                               left2,  bottom2, 6, dist >> 1,
                               right2, bottom2, 8, dist >> 1,
                               omv.x,  bottom,  7, dist);
        }
        else // check border for each mv
        {
            if (top >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, top, 2, dist);
            }
            if (top2 >= mvmin.y) // check half top
            {
                if (left2 >= mvmin.x) // check half left
                {
                    COST_MV_PT_DIST(left2, top2, 1, (dist >> 1));
                }
                if (right2 <= mvmax.x) // check half right
                {
                    COST_MV_PT_DIST(right2, top2, 3, (dist >> 1));
                }
            } // check half top
            if (left >= mvmin.x) // check left
            {
                COST_MV_PT_DIST(left, omv.y, 4, dist);
            }
            if (right <= mvmax.x) // check right
            {
                COST_MV_PT_DIST(right, omv.y, 5, dist);
            }
            if (bottom2 <= mvmax.y) // check half bottom
            {
                if (left2 >= mvmin.x) // check half left
                {
                    COST_MV_PT_DIST(left2, bottom2, 6, (dist >> 1));
                }
                if (right2 <= mvmax.x) // check half right
                {
                    COST_MV_PT_DIST(right2, bottom2, 8, (dist >> 1));
                }
            } // check half bottom
            if (bottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, bottom, 7, dist);
            }
        } // check border for each mv

        if (bcost < saved)
            rounds = 0;
        else if (++rounds >= earlyExitIters)
            return;
    }

    for (int16_t dist = 16; dist <= (int16_t)merange; dist <<= 1)
    {
        const int16_t top    = omv.y - dist;
        const int16_t bottom = omv.y + dist;
        const int16_t left   = omv.x - dist;
        const int16_t right  = omv.x + dist;

        saved = bcost;
        if (top >= mvmin.y && left >= mvmin.x &&
            right <= mvmax.x && bottom <= mvmax.y) // check border
        {
            /* index
                  0
                  3
                  2
                  1
          0 3 2 1 * 1 2 3 0
                  1
                  2
                  3
                  0
            */

            COST_MV_PT_DIST_X4(omv.x,  top,    0, dist,
                               left,   omv.y,  0, dist,
                               right,  omv.y,  0, dist,
                               omv.x,  bottom, 0, dist);

            for (int16_t index = 1; index < 4; index++)
            {
                int16_t posYT = top    + ((dist >> 2) * index);
                int16_t posYB = bottom - ((dist >> 2) * index);
                int16_t posXL = omv.x  - ((dist >> 2) * index);
                int16_t posXR = omv.x  + ((dist >> 2) * index);

                COST_MV_PT_DIST_X4(posXL, posYT, 0, dist,
                                   posXR, posYT, 0, dist,
                                   posXL, posYB, 0, dist,
                                   posXR, posYB, 0, dist);
            }
        }
        else // check border for each mv
        {
            if (top >= mvmin.y) // check top
            {
                COST_MV_PT_DIST(omv.x, top, 0, dist);
            }
            if (left >= mvmin.x) // check left
            {
                COST_MV_PT_DIST(left, omv.y, 0, dist);
            }
            if (right <= mvmax.x) // check right
            {
                COST_MV_PT_DIST(right, omv.y, 0, dist);
            }
            if (bottom <= mvmax.y) // check bottom
            {
                COST_MV_PT_DIST(omv.x, bottom, 0, dist);
            }
            for (int16_t index = 1; index < 4; index++)
            {
                int16_t posYT = top    + ((dist >> 2) * index);
                int16_t posYB = bottom - ((dist >> 2) * index);
                int16_t posXL = omv.x - ((dist >> 2) * index);
                int16_t posXR = omv.x + ((dist >> 2) * index);

                if (posYT >= mvmin.y) // check top
                {
                    if (posXL >= mvmin.x) // check left
                    {
                        COST_MV_PT_DIST(posXL, posYT, 0, dist);
                    }
                    if (posXR <= mvmax.x) // check right
                    {
                        COST_MV_PT_DIST(posXR, posYT, 0, dist);
                    }
                } // check top
                if (posYB <= mvmax.y) // check bottom
                {
                    if (posXL >= mvmin.x) // check left
                    {
                        COST_MV_PT_DIST(posXL, posYB, 0, dist);
                    }
                    if (posXR <= mvmax.x) // check right
                    {
                        COST_MV_PT_DIST(posXR, posYB, 0, dist);
                    }
                } // check bottom
            } // for ...
        } // check border for each mv

        if (bcost < saved)
            rounds = 0;
        else if (++rounds >= earlyExitIters)
            return;
    } // dist > 8
}

int MotionEstimate::subpelCompare(ReferencePlanes *ref, const MV& qmv, pixelcmp_t cmp)
{
    if (ref->isLowres)
    {
        if ((qmv.x | qmv.y) & 1)
        {
            /* QPel: use H.264's simple QPEL generation from averaged HPEL pixels */
            int hpelA = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *frefA = ref->lowresPlane[hpelA] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * ref->lumaStride;

            MV qmvB = qmv + MV((qmv.x & 1) * 2, (qmv.y & 1) * 2);
            int hpelB = (qmvB.y & 2) | ((qmvB.x & 2) >> 1);
            pixel *frefB = ref->lowresPlane[hpelB] + blockOffset + (qmvB.x >> 2) + (qmvB.y >> 2) * ref->lumaStride;

            /* TODO: make this into a lowres MC primitive */
            for (int y = 0; y < blockheight; y++)
                for (int x = 0; x < blockwidth; x++)
                    subpelbuf[y * FENC_STRIDE + x] = (frefA[y * ref->lumaStride + x] + frefB[y * ref->lumaStride + x] + 1) >> 1;
            return cmp(fenc, FENC_STRIDE, subpelbuf, FENC_STRIDE);
        }
        else
        {
            /* FPEL/HPEL */
            int hpel = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *fref = ref->lowresPlane[hpel] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * ref->lumaStride;
            return cmp(fenc, FENC_STRIDE, fref, ref->lumaStride);
        }
    }
    else
    {
        pixel *fref = ref->fpelPlane + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * ref->lumaStride;
        int xFrac = qmv.x & 0x3;
        int yFrac = qmv.y & 0x3;

        if ((yFrac | xFrac) == 0)
        {
            return cmp(fenc, FENC_STRIDE, fref, ref->lumaStride);
        }
        else if (yFrac == 0)
        {
            primitives.ipfilter_pp[FILTER_H_P_P_8](fref, ref->lumaStride, subpelbuf, FENC_STRIDE, blockwidth, blockheight, g_lumaFilter[xFrac]);
        }
        else if (xFrac == 0)
        {
            primitives.ipfilter_pp[FILTER_V_P_P_8](fref, ref->lumaStride, subpelbuf, FENC_STRIDE, blockwidth, blockheight, g_lumaFilter[yFrac]);
        }
        else
        {
            int filterSize = NTAPS_LUMA;
            int halfFilterSize = (filterSize >> 1);
            primitives.ipfilter_ps[FILTER_H_P_S_8](fref - (halfFilterSize - 1) * ref->lumaStride, ref->lumaStride, immedVal, blockwidth, blockwidth, blockheight + filterSize - 1, g_lumaFilter[xFrac]);
            primitives.ipfilter_sp[FILTER_V_S_P_8](immedVal + (halfFilterSize - 1) * blockwidth, blockwidth, subpelbuf, FENC_STRIDE, blockwidth, blockheight, g_lumaFilter[yFrac]);
        }

        return cmp(fenc, FENC_STRIDE, subpelbuf, FENC_STRIDE);
    }
}

void MotionEstimate::subpelInterpolate(pixel *fref, intptr_t lumaStride, int xFrac, int yFrac, int dir)
{
    assert(yFrac | xFrac);

    int realWidth = blockwidth + (dir == 2);
    int realHeight = blockheight + (dir == 1);
    intptr_t realStride = FENC_STRIDE + (dir == 2);

    if (yFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_H_P_P_8](fref, lumaStride, subpelbuf, realStride, realWidth, realHeight, g_lumaFilter[xFrac]);
    }
    else if (xFrac == 0)
    {
        primitives.ipfilter_pp[FILTER_V_P_P_8](fref, lumaStride, subpelbuf, realStride, realWidth, realHeight, g_lumaFilter[yFrac]);
    }
    else
    {
        int filterSize = NTAPS_LUMA;
        int halfFilterSize = (filterSize >> 1);
        primitives.ipfilter_ps[FILTER_H_P_S_8](fref - (halfFilterSize - 1) * lumaStride, lumaStride, immedVal, realWidth, realWidth, realHeight + filterSize - 1, g_lumaFilter[xFrac]);
        primitives.ipfilter_sp[FILTER_V_S_P_8](immedVal + (halfFilterSize - 1) * realWidth, realWidth, subpelbuf, realStride, realWidth, realHeight, g_lumaFilter[yFrac]);
    }
}
