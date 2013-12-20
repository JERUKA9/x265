/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComPic.h
    \brief    picture class (header)
*/

#ifndef X265_TCOMPIC_H
#define X265_TCOMPIC_H

// Include files
#include "CommonDef.h"
#include "TComPicSym.h"
#include "TComPicYuv.h"
#include "lowres.h"
#include "threading.h"

namespace x265 {
// private namespace

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture class (symbol + YUV buffers)
class TComPic
{
private:

    TComPicSym*           m_picSym;
    TComPicYuv*           m_origPicYuv;
    TComPicYuv*           m_reconPicYuv;

    Window                m_conformanceWindow;
    Window                m_defaultDisplayWindow;

    bool                  m_bUsedByCurr;          // Used by current picture
    bool                  m_bIsLongTerm;          // IS long term picture
    bool                  m_bCheckLTMSB;

public:

    //** Frame Parallelism - notification between FrameEncoders of available motion reference rows **
    volatile uint32_t     m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
    volatile uint32_t     m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount
    Event                 m_reconRowWait;       // event triggered m_countRefEncoders times each time a recon row is completed
    void*                 m_userData;           // user provided pointer passed in with this picture

    Lowres                m_lowres;

    TComPic();
    virtual ~TComPic();

    void          create(int width, int height, UInt maxWidth, UInt maxHeight, UInt maxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, int bframes);

    virtual void  destroy(int bframes);

    bool          getUsedByCurr()           { return m_bUsedByCurr; }

    void          setUsedByCurr(bool bUsed) { m_bUsedByCurr = bUsed; }

    bool          getIsLongTerm()           { return m_bIsLongTerm; }

    void          setIsLongTerm(bool lt)    { m_bIsLongTerm = lt; }

    void          setCheckLTMSBPresent(bool b) { m_bCheckLTMSB = b; }

    bool          getCheckLTMSBPresent()  { return m_bCheckLTMSB; }

    TComPicSym*   getPicSym()             { return m_picSym; }

    TComSlice*    getSlice()              { return m_picSym->getSlice(); }

    int           getPOC()                { return m_picSym->getSlice()->getPOC(); }

    TComDataCU*   getCU(UInt cuAddr)    { return m_picSym->getCU(cuAddr); }

    TComPicYuv*   getPicYuvOrg()          { return m_origPicYuv; }

    TComPicYuv*   getPicYuvRec()          { return m_reconPicYuv; }

    UInt          getNumCUsInFrame()      { return m_picSym->getNumberOfCUsInFrame(); }

    UInt          getNumPartInWidth()     { return m_picSym->getNumPartInWidth(); }

    UInt          getNumPartInHeight()    { return m_picSym->getNumPartInHeight(); }

    UInt          getNumPartInCU()        { return m_picSym->getNumPartition(); }

    UInt          getFrameWidthInCU()     { return m_picSym->getFrameWidthInCU(); }

    UInt          getFrameHeightInCU()    { return m_picSym->getFrameHeightInCU(); }

    UInt          getMinCUWidth()         { return m_picSym->getMinCUWidth(); }

    UInt          getMinCUHeight()        { return m_picSym->getMinCUHeight(); }

    UInt          getParPelX(UChar partIdx) { return getParPelX(partIdx); }

    UInt          getParPelY(UChar partIdx) { return getParPelX(partIdx); }

    int           getStride()             { return m_reconPicYuv->getStride(); }

    int           getCStride()            { return m_reconPicYuv->getCStride(); }

    Window&       getConformanceWindow()  { return m_conformanceWindow; }

    Window&       getDefDisplayWindow()   { return m_defaultDisplayWindow; }

    void          createNonDBFilterInfo(int lastSliceCUAddr, int sliceGranularityDepth);
    void          createNonDBFilterInfoLCU(int sliceID, TComDataCU* cu, UInt startSU, UInt endSU, int sliceGranularyDepth, UInt picWidth, UInt picHeight);
    void          destroyNonDBFilterInfo();
}; // END CLASS DEFINITION TComPic
}
//! \}

#endif // ifndef X265_TCOMPIC_H
