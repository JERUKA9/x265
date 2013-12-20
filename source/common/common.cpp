/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComSlice.h"
#include "x265.h"
#include "threading.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <time.h>

using namespace x265;

#if HIGH_BIT_DEPTH
const int x265_max_bit_depth = 8; // 12;
#else
const int x265_max_bit_depth = 8;
#endif

#define ALIGNBYTES 32

#if _WIN32
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free   __mingw_aligned_free
#include "malloc.h"
#endif

void *x265_malloc(size_t size)
{
    return _aligned_malloc(size, ALIGNBYTES);
}

void x265_free(void *ptr)
{
    if (ptr) _aligned_free(ptr);
}

#else // if _WIN32
void *x265_malloc(size_t size)
{
    void *ptr;

    if (posix_memalign((void**)&ptr, ALIGNBYTES, size) == 0)
        return ptr;
    else
        return NULL;
}

void x265_free(void *ptr)
{
    if (ptr) free(ptr);
}

#endif // if _WIN32

void x265_log(x265_param_t *param, int level, const char *fmt, ...)
{
    if (param && level > param->logLevel)
        return;
    const char *log_level;
    switch (level)
    {
    case X265_LOG_ERROR:
        log_level = "error";
        break;
    case X265_LOG_WARNING:
        log_level = "warning";
        break;
    case X265_LOG_INFO:
        log_level = "info";
        break;
    case X265_LOG_DEBUG:
        log_level = "debug";
        break;
    default:
        log_level = "unknown";
        break;
    }

    fprintf(stderr, "x265 [%s]: ", log_level);
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}

extern "C"
void x265_param_default(x265_param_t *param)
{
    memset(param, 0, sizeof(x265_param_t));

    /* Applying non-zero default values to all elements in the param structure */
    param->logLevel = X265_LOG_INFO;
    param->bEnableWavefront = 1;
    param->frameNumThreads = 1;
    
    param->internalBitDepth = 8;

    /* CU definitions */
    param->maxCUSize = 64;
    param->tuQTMaxInterDepth = 3;
    param->tuQTMaxIntraDepth = 3;

    /* Coding Structure */
    param->decodingRefreshType = 1;
    param->keyframeMin = 0;
    param->keyframeMax = 250;
    param->bframes = 3;
    param->lookaheadDepth = 10;
    param->bFrameAdaptive = X265_B_ADAPT_FAST;
    param->scenecutThreshold = 40; /* Magic number pulled in from x264*/

    /* Intra Coding Tools */
    param->bEnableStrongIntraSmoothing = 1;

    /* Inter Coding tools */
    param->searchMethod = X265_STAR_SEARCH;
    param->subpelRefine = 5;
    param->searchRange = 60;
    param->bipredSearchRange = 4;
    param->maxNumMergeCand = 5u;
    param->bEnableAMP = 1;
    param->bEnableRectInter = 1;
    param->bRDLevel = X265_FULL_RDO;
    param->bEnableRDO = 1;
    param->bEnableRDOQ = 1;
    param->bEnableRDOQTS = 1;
    param->bEnableSignHiding = 1;
    param->bEnableTransformSkip = 1;
    param->bEnableTSkipFast = 1;
    param->maxNumReferences = 1;

    /* Loop Filter */
    param->bEnableLoopFilter = 1;
    
    /* SAO Loop Filter */
    param->bEnableSAO = 1;
    param->saoLcuBasedOptimization = 1;
    
    /* Rate control options */
    param->rc.bitrate = 0;
    param->rc.rateTolerance = 0.1;
    param->rc.qCompress = 0.6;
    param->rc.ipFactor = 1.4f;
    param->rc.pbFactor = 1.3f;
    param->rc.qpStep = 4;
    param->rc.rateControlMode = X265_RC_CQP;
    param->rc.qp = 32;
}

extern "C"
void x265_picture_init(x265_param_t *param, x265_picture_t *pic)
{
    memset(pic, 0, sizeof(x265_picture_t));
    pic->bitDepth = param->internalBitDepth;
}

extern "C"
int x265_param_apply_profile(x265_param_t *param, const char *profile)
{
    if (!profile)
        return 0;
    if (!strcmp(profile, "main"))
    {}
    else if (!strcmp(profile, "main10"))
    {
#if HIGH_BIT_DEPTH
        param->internalBitDepth = 10;
#else
        x265_log(param, X265_LOG_WARNING, "not compiled for 16bpp. Falling back to main profile.\n");
        return -1;
#endif
    }
    else if (!strcmp(profile, "mainstillpicture"))
    {
        param->keyframeMax = 1;
        param->bOpenGOP = 0;
    }
    else
    {
        x265_log(param, X265_LOG_ERROR, "unknown profile <%s>\n", profile);
        return -1;
    }

    return 0;
}

static inline int _confirm(x265_param_t *param, bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    x265_log(param, X265_LOG_ERROR, "%s\n", message);
    return 1;
}

int x265_check_params(x265_param_t *param)
{
#define CHECK(expr, msg) check_failed |= _confirm(param, expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMaxLog2Size = maxCUDepth + 2 - 1;
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    CHECK(param->internalBitDepth > x265_max_bit_depth,
          "InternalBitDepth must be <= x265_max_bit_depth");
    CHECK(param->rc.qp < -6 * (param->internalBitDepth - 8) || param->rc.qp > 51,
          "QP exceeds supported range (-QpBDOffsety to 51)");
    CHECK(param->frameRate <= 0,
          "Frame rate must be more than 1");
    CHECK(param->searchMethod<0 || param->searchMethod> X265_FULL_SEARCH,
          "Search method is not supported value (0:DIA 1:HEX 2:UMH 3:HM 5:FULL)");
    CHECK(param->searchRange < 0,
          "Search Range must be more than 0");
    CHECK(param->searchRange >= 32768,
          "Search Range must be less than 32768");
    CHECK(param->bipredSearchRange < 0,
          "Search Range must be more than 0");
    CHECK(param->keyframeMax < 0,
          "Keyframe interval must be 0 (auto) 1 (intra-only) or greater than 1");
    CHECK(param->frameNumThreads <= 0,
          "frameNumThreads (--frame-threads) must be 1 or higher");
    CHECK(param->cbQpOffset < -12, "Min. Chroma Cb QP Offset is -12");
    CHECK(param->cbQpOffset >  12, "Max. Chroma Cb QP Offset is  12");
    CHECK(param->crQpOffset < -12, "Min. Chroma Cr QP Offset is -12");
    CHECK(param->crQpOffset >  12, "Max. Chroma Cr QP Offset is  12");

    CHECK((param->maxCUSize >> maxCUDepth) < 4,
          "Minimum partition width size should be larger than or equal to 8");
    CHECK(param->maxCUSize < 16,
          "Maximum partition width size should be larger than or equal to 16");
    CHECK((param->sourceWidth  % (param->maxCUSize >> (maxCUDepth - 1))) != 0,
          "Resulting coded frame width must be a multiple of the minimum CU size");
    CHECK((param->sourceHeight % (param->maxCUSize >> (maxCUDepth - 1))) != 0,
          "Resulting coded frame height must be a multiple of the minimum CU size");

    CHECK((1u << tuQTMaxLog2Size) > param->maxCUSize,
          "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");

    CHECK(param->tuQTMaxInterDepth < 1,
          "QuadtreeTUMaxDepthInter must be greater than or equal to 1");
    CHECK(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxInterDepth - 1)),
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CHECK(param->tuQTMaxIntraDepth < 1,
          "QuadtreeTUMaxDepthIntra must be greater than or equal to 1");
    CHECK(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxIntraDepth - 1)),
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");

    CHECK(param->maxNumMergeCand < 1, "MaxNumMergeCand must be 1 or greater.");
    CHECK(param->maxNumMergeCand > 5, "MaxNumMergeCand must be 5 or smaller.");

    CHECK(param->maxNumReferences < 1, "maxNumReferences must be 1 or greater.");
    CHECK(param->maxNumReferences > MAX_NUM_REF, "maxNumReferences must be 16 or smaller.");

    // TODO: ChromaFmt assumes 4:2:0 below
    CHECK(param->sourceWidth  % TComSPS::getWinUnitX(CHROMA_420) != 0,
          "Picture width must be an integer multiple of the specified chroma subsampling");
    CHECK(param->sourceHeight % TComSPS::getWinUnitY(CHROMA_420) != 0,
          "Picture height must be an integer multiple of the specified chroma subsampling");
    CHECK(param->rc.rateControlMode<X265_RC_ABR || param->rc.rateControlMode> X265_RC_CRF,
          "Rate control mode is out of range");
    CHECK(param->bRDLevel < X265_NO_RDO_NO_RDOQ || param->bRDLevel> X265_FULL_RDO,
          "RD Level is out of range");
    CHECK(param->bframes > param->lookaheadDepth,
          "Lookahead depth must be greater than the max consecutive bframe count");

    // max CU size should be power of 2
    uint32_t i = param->maxCUSize;
    while (i)
    {
        i >>= 1;
        if ((i & 1) == 1)
            CHECK(i != 1, "Max CU size should be 2^n");
    }

    CHECK(param->bEnableWavefront < 0, "WaveFrontSynchro cannot be negative");

    return check_failed;
}

int x265_set_globals(x265_param_t *param)
{
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    static int once /* = 0 */;

    if (ATOMIC_CAS(&once, 0, 1) == 1)
    {
        if (param->maxCUSize != g_maxCUWidth)
        {
            x265_log(param, X265_LOG_ERROR, "maxCUSize must be the same for all encoders in a single process");
            return -1;
        }
        if (param->internalBitDepth != g_bitDepth)
        {
            x265_log(param, X265_LOG_ERROR, "internalBitDepth must be the same for all encoders in a single process");
            return -1;
        }
    }
    else
    {
        // set max CU width & height
        g_maxCUWidth  = param->maxCUSize;
        g_maxCUHeight = param->maxCUSize;
        g_bitDepth = param->internalBitDepth;

        // compute actual CU depth with respect to config depth and max transform size
        g_addCUDepth = 0;
        while ((param->maxCUSize >> maxCUDepth) > (1u << (tuQTMinLog2Size + g_addCUDepth)))
        {
            g_addCUDepth++;
        }

        maxCUDepth += g_addCUDepth;
        g_addCUDepth++;
        g_maxCUDepth = maxCUDepth;

        // initialize partition order
        UInt* tmp = &g_zscanToRaster[0];
        initZscanToRaster(g_maxCUDepth + 1, 1, 0, tmp);
        initRasterToZscan(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);

        // initialize conversion matrix from partition index to pel
        initRasterToPelXY(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);
    }
    return 0;
}

void x265_print_params(x265_param_t *param)
{
    if (param->logLevel < X265_LOG_INFO)
        return;
#if HIGH_BIT_DEPTH
    x265_log(param, X265_LOG_INFO, "Internal bit depth           : %d\n", param->internalBitDepth);
#endif
    x265_log(param, X265_LOG_INFO, "CU size                      : %d\n", param->maxCUSize);
    x265_log(param, X265_LOG_INFO, "Max RQT depth inter / intra  : %d / %d\n", param->tuQTMaxInterDepth, param->tuQTMaxIntraDepth);

    x265_log(param, X265_LOG_INFO, "ME / range / subpel / merge  : %s / %d / %d / %d\n",
             x265_motion_est_names[param->searchMethod], param->searchRange, param->subpelRefine, param->maxNumMergeCand);
    x265_log(param, X265_LOG_INFO, "Keyframe min / max           : %d / %d\n", param->keyframeMin, param->keyframeMax);
    switch (param->rc.rateControlMode)
    {
    case X265_RC_ABR:
        x265_log(param, X265_LOG_INFO, "Rate Control                 : ABR-%d kbps\n", param->rc.bitrate);
        break;
    case X265_RC_CQP:
        x265_log(param, X265_LOG_INFO, "Rate Control                 : CQP-%d\n", param->rc.qp);
        break;
    case X265_RC_CRF:
        x265_log(param, X265_LOG_INFO, "Rate Control                 : CRF-%d\n", param->rc.rateFactor);
        break;
    }

    if (param->cbQpOffset || param->crQpOffset)
    {
        x265_log(param, X265_LOG_INFO, "Cb/Cr QP Offset              : %d / %d\n", param->cbQpOffset, param->crQpOffset);
    }
    if (param->rdPenalty)
    {
        x265_log(param, X265_LOG_INFO, "RDpenalty                    : %d\n", param->rdPenalty);
    }
    x265_log(param, X265_LOG_INFO, "Lookahead / bframes / badapt : %d / %d / %d\n", param->lookaheadDepth, param->bframes, param->bFrameAdaptive);
    x265_log(param, X265_LOG_INFO, "tools: ");
#define TOOLOPT(FLAG, STR) if (FLAG) fprintf(stderr, "%s ", STR)
    TOOLOPT(param->bEnableRectInter, "rect");
    TOOLOPT(param->bEnableAMP, "amp");
    TOOLOPT(param->bEnableCbfFastMode, "cfm");
    TOOLOPT(param->bEnableConstrainedIntra, "cip");
    TOOLOPT(param->bEnableEarlySkip, "esd");
    switch (param->bRDLevel)
    {
    case X265_NO_RDO_NO_RDOQ: 
        fprintf(stderr, "%s", "no-rdo no-rdoq "); break;
    case X265_NO_RDO:
        fprintf(stderr, "%s", "no-rdo rdoq "); break;
    case X265_FULL_RDO:
        fprintf(stderr, "%s", "rdo rdoq "); break;
    default: 
        fprintf(stderr, "%s", "Unknown RD Level");
    }

    TOOLOPT(param->bEnableLoopFilter, "lft");
    if (param->bEnableSAO)
    {
        TOOLOPT(param->bEnableSAO, "sao");
        TOOLOPT(param->saoLcuBasedOptimization, "sao-lcu");
    }
    TOOLOPT(param->bEnableSignHiding, "sign-hide");
    if (param->bEnableTransformSkip)
    {
        TOOLOPT(param->bEnableTransformSkip, "tskip");
        TOOLOPT(param->bEnableTSkipFast, "tskip-fast");
        TOOLOPT(param->bEnableRDOQTS, "rdoqts");
    }
    TOOLOPT(param->bEnableWeightedPred, "weightp");
    TOOLOPT(param->bEnableWeightedBiPred, "weightbp");
    fprintf(stderr, "\n");
    fflush(stderr);
}

int64_t x265_mdate(void)
{
#if _WIN32
    struct timeb tb;
    ftime(&tb);
    return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
#else
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}
