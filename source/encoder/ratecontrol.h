/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Sumalatha Polureddy <sumalatha@multicorewareinc.com>
 *          Aarthi Priya Thirumalai <aarthi@multicorewareinc.com>
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

#ifndef X265_RATECONTROL_H
#define X265_RATECONTROL_H

#include "TLibCommon/CommonDef.h"

namespace x265 {
struct Lookahead;
class TComPic;

struct RateControlEntry
{
    int pictType;
    int texBits;
    int mvBits;
    double blurredComplexity;
    double qpaRc;
    double qRceq;
};

struct RateControl
{
    TComSlice *curFrame;        /* all info abt the current frame */
    SliceType frameType;        /* Current frame type */
    int ncu;                    /* number of CUs in a frame */
    int framerate;              /* current frame rate TODO: need to initialize in init */
    int frameThreads;
    int keyFrameInterval;       /* TODO: need to initialize in init */
    int qp;                     /* updated qp for current frame */
    int baseQp;                 /* CQP base QP */
    double frameDuration;        /* current frame duration in seconds */
    double bitrate;
    double rateTolerance;
    double qCompress;
    int    lastSatd;
    int    qpConstant[3];
    double cplxrSum;           /* sum of bits*qscale/rceq */
    double wantedBitsWindow;  /* target bitrate * window */
    double ipOffset;
    double pbOffset;
    double ipFactor;
    double pbFactor;
    int lastNonBPictType;
    double accumPQp;          /* for determining I-frame quant */
    double accumPNorm;
    double lastQScaleFor[3];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff */
    double lstep;
    double lmin[3];           /* min qscale by frame type */
    double lmax[3];
    double shortTermCplxSum;
    double shortTermCplxCount;
    RcMethod rateControlMode;
    int64_t totalBits;   /* totalbits used for already encoded frames */
    double lastRceq;
    int framesDone;   /* framesDone keeps track of # of frames passed through RateCotrol already */
    RateControl(x265_param_t * param);

    // to be called for each frame to process RateCOntrol and set QP
    void rateControlStart(TComPic* pic, Lookahead *, RateControlEntry* rce);

    int rateControlEnd(int64_t bits, RateControlEntry* rce);

protected:

    double getQScale(RateControlEntry *rce, double rateFactor);
    double rateEstimateQscale(RateControlEntry *rce); // main logic for calculating QP based on ABR
    void accumPQpUpdate();
};
}

#endif // ifndef X265_RATECONTROL_H
