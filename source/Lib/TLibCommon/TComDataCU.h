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

/** \file     TComDataCU.h
    \brief    CU data structure (header)
    \todo     not all entities are documented
*/

#ifndef X265_TCOMDATACU_H
#define X265_TCOMDATACU_H

#include <assert.h>

// Include files
#include "CommonDef.h"
#include "TComMotionInfo.h"
#include "TComSlice.h"
#include "TComRdCost.h"
#include "TComPattern.h"

#include <algorithm>
#include <vector>

namespace x265 {
// private namespace


//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Non-deblocking in-loop filter processing block data structure
// ====================================================================================================================

/// Non-deblocking filter processing block border tag
enum NDBFBlockBorderTag
{
    SGU_L = 0,
    SGU_R,
    SGU_T,
    SGU_B,
    SGU_TL,
    SGU_TR,
    SGU_BL,
    SGU_BR,
    NUM_SGU_BORDER
};

/// Non-deblocking filter processing block information
struct NDBFBlockInfo
{
    int   sliceID; //!< slice ID
    UInt  startSU; //!< starting SU z-scan address in LCU
    UInt  endSU;  //!< ending SU z-scan address in LCU
    UInt  widthSU; //!< number of SUs in width
    UInt  heightSU; //!< number of SUs in height
    UInt  posX;   //!< top-left X coordinate in picture
    UInt  posY;   //!< top-left Y coordinate in picture
    UInt  width;  //!< number of pixels in width
    UInt  height; //!< number of pixels in height
    bool  isBorderAvailable[NUM_SGU_BORDER]; //!< the border availabilities
    bool  allBordersAvailable;

    NDBFBlockInfo() : sliceID(0), startSU(0), endSU(0) {} //!< constructor

    const NDBFBlockInfo& operator =(const NDBFBlockInfo& src); //!< "=" operator
};

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU data structure class
class TComDataCU
{
private:

    // -------------------------------------------------------------------------------------------------------------------
    // class pointers
    // -------------------------------------------------------------------------------------------------------------------

    TComPic*      m_pic;            ///< picture class pointer
    TComSlice*    m_slice;          ///< slice header pointer
    TComPattern*  m_pattern;        ///< neighbor access class pointer

    // -------------------------------------------------------------------------------------------------------------------
    // CU description
    // -------------------------------------------------------------------------------------------------------------------

    UInt          m_cuAddr;         ///< CU address in a slice
    UInt          m_absIdxInLCU;    ///< absolute address in a CU. It's Z scan order
    UInt          m_cuPelX;         ///< CU position in a pixel (X)
    UInt          m_cuPelY;         ///< CU position in a pixel (Y)
    UInt          m_numPartitions;   ///< total number of minimum partitions in a CU
    UChar*        m_width;         ///< array of widths
    UChar*        m_height;        ///< array of heights
    UChar*        m_depth;         ///< array of depths
    int           m_unitSize;         ///< size of a "minimum partition"
    UInt          m_unitMask;       ///< mask for mapping index to CompressMV field

    // -------------------------------------------------------------------------------------------------------------------
    // CU data
    // -------------------------------------------------------------------------------------------------------------------
    bool*         m_skipFlag;         ///< array of skip flags
    char*         m_partSizes;       ///< array of partition sizes
    char*         m_predModes;       ///< array of prediction modes
    bool*         m_cuTransquantBypass; ///< array of cu_transquant_bypass flags
    char*         m_qp;             ///< array of QP values
    UChar*        m_trIdx;         ///< array of transform indices
    UChar*        m_transformSkip[3]; ///< array of transform skipping flags
    UChar*        m_cbf[3];        ///< array of coded block flags (CBF)
    TComCUMvField m_cuMvField[2];   ///< array of motion vectors
    TCoeff*       m_trCoeffY;       ///< transformed coefficient buffer (Y)
    TCoeff*       m_trCoeffCb;      ///< transformed coefficient buffer (Cb)
    TCoeff*       m_trCoeffCr;      ///< transformed coefficient buffer (Cr)

    Pel*          m_iPCMSampleY;    ///< PCM sample buffer (Y)
    Pel*          m_iPCMSampleCb;   ///< PCM sample buffer (Cb)
    Pel*          m_iPCMSampleCr;   ///< PCM sample buffer (Cr)

    std::vector<NDBFBlockInfo> m_vNDFBlock;

    // -------------------------------------------------------------------------------------------------------------------
    // neighbor access variables
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   m_cuAboveLeft;    ///< pointer of above-left CU
    TComDataCU*   m_cuAboveRight;   ///< pointer of above-right CU
    TComDataCU*   m_cuAbove;        ///< pointer of above CU
    TComDataCU*   m_cuLeft;         ///< pointer of left CU
    TComDataCU*   m_cuColocated[2]; ///< pointer of temporally colocated CU's for both directions
    TComMvField   m_mvFieldA;        ///< motion vector of position A
    TComMvField   m_mvFieldB;        ///< motion vector of position B
    TComMvField   m_mvFieldC;        ///< motion vector of position C

    // -------------------------------------------------------------------------------------------------------------------
    // coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         m_bMergeFlags;      ///< array of merge flags
    UChar*        m_mergeIndex;    ///< array of merge candidate indices
    bool          m_bIsMergeAMP;
    UChar*        m_lumaIntraDir;  ///< array of intra directions (luma)
    UChar*        m_chromaIntraDir; ///< array of intra directions (chroma)
    UChar*        m_interDir;      ///< array of inter directions
    char*         m_mvpIdx[2];     ///< array of motion vector predictor candidates
    char*         m_mvpNum[2];     ///< array of number of possible motion vectors predictors
    bool*         m_iPCMFlags;       ///< array of intra_pcm flags

    // -------------------------------------------------------------------------------------------------------------------
    // misc. variables
    // -------------------------------------------------------------------------------------------------------------------
protected:

    /// add possible motion vector predictor candidates
    bool          xAddMVPCand(AMVPInfo* info, RefPicList picList, int refIdx, UInt partUnitIdx, MVP_DIR dir);

    bool          xAddMVPCandOrder(AMVPInfo* info, RefPicList picList, int refIdx, UInt partUnitIdx, MVP_DIR dir);

    void          deriveRightBottomIdx(UInt partIdx, UInt& outPartIdxRB);

    bool          xGetColMVP(RefPicList picList, int cuAddr, int partUnitIdx, MV& outMV, int& outRefIdx);

    /// compute scaling factor from POC difference
    int           xGetDistScaleFactor(int curPOC, int curRefPOC, int colPOC, int colRefPOC);

    void xDeriveCenterIdx(UInt partIdx, UInt& outPartIdxCenter);

public:

    TComDataCU();
    virtual ~TComDataCU();

    UInt64        m_totalCost;       ///< sum of partition RD costs
    UInt          m_totalDistortion; ///< sum of partition distortion
    UInt          m_totalBits;       ///< sum of partition signal bits
    UInt          m_totalBins;       ///< sum of partition bins
    
    // -------------------------------------------------------------------------------------------------------------------
    // create / destroy / initialize / copy
    // -------------------------------------------------------------------------------------------------------------------

    void          create(UInt numPartition, UInt width, UInt height, int unitSize);
    void          destroy();

    void          initCU(TComPic* pic, UInt cuAddr);
    void          initEstData(UInt depth, int qp);
    void          initSubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth, int qp);
    void          setOutsideCUPart(UInt absPartIdx, UInt depth);

    void          copySubCU(TComDataCU* cu, UInt partUnitIdx, UInt depth);
    void          copyInterPredInfoFrom(TComDataCU* cu, UInt absPartIdx, RefPicList picList);
    void          copyPartFrom(TComDataCU* cu, UInt partUnitIdx, UInt depth, bool isRDObasedAnalysis = true);

    void          copyToPic(UChar depth);
    void          copyToPic(UChar depth, UInt partIdx, UInt partDepth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU description
    // -------------------------------------------------------------------------------------------------------------------

    TComPic*      getPic()                         { return m_pic; }

    TComSlice*    getSlice()                       { return m_slice; }

    UInt&         getAddr()                        { return m_cuAddr; }

    UInt&         getZorderIdxInCU()               { return m_absIdxInLCU; }

    UInt          getSCUAddr();

    UInt          getCUPelX()                        { return m_cuPelX; }

    UInt          getCUPelY()                        { return m_cuPelY; }

    TComPattern*  getPattern()                        { return m_pattern; }

    UChar*        getDepth()                        { return m_depth; }

    UChar         getDepth(UInt uiIdx)            { return m_depth[uiIdx]; }

    void          setDepth(UInt uiIdx, UChar  uh) { m_depth[uiIdx] = uh; }

    void          setDepthSubParts(UInt depth, UInt absPartIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for CU data
    // -------------------------------------------------------------------------------------------------------------------

    char*         getPartitionSize()                      { return m_partSizes; }

    int           getUnitSize()                           { return m_unitSize; }

    PartSize      getPartitionSize(UInt uiIdx)            { return static_cast<PartSize>(m_partSizes[uiIdx]); }

    void          setPartitionSize(UInt uiIdx, PartSize uh) { m_partSizes[uiIdx] = (char)uh; }

    void          setPartSizeSubParts(PartSize eMode, UInt absPartIdx, UInt depth);
    void          setCUTransquantBypassSubParts(bool flag, UInt absPartIdx, UInt depth);

    bool*        getSkipFlag()                        { return m_skipFlag; }

    bool         getSkipFlag(UInt idx)                { return m_skipFlag[idx]; }

    void         setSkipFlag(UInt idx, bool skip)     { m_skipFlag[idx] = skip; }

    void         setSkipFlagSubParts(bool skip, UInt absPartIdx, UInt depth);

    char*         getPredictionMode()                        { return m_predModes; }

    PredMode      getPredictionMode(UInt uiIdx)            { return static_cast<PredMode>(m_predModes[uiIdx]); }

    bool*         getCUTransquantBypass()                        { return m_cuTransquantBypass; }

    bool          getCUTransquantBypass(UInt uiIdx)             { return m_cuTransquantBypass[uiIdx]; }

    void          setPredictionMode(UInt uiIdx, PredMode uh) { m_predModes[uiIdx] = (char)uh; }

    void          setPredModeSubParts(PredMode eMode, UInt absPartIdx, UInt depth);

    UChar*        getWidth()                        { return m_width; }

    UChar         getWidth(UInt uiIdx)            { return m_width[uiIdx]; }

    void          setWidth(UInt uiIdx, UChar  uh) { m_width[uiIdx] = uh; }

    UChar*        getHeight()                        { return m_height; }

    UChar         getHeight(UInt uiIdx)            { return m_height[uiIdx]; }

    void          setHeight(UInt uiIdx, UChar  uh) { m_height[uiIdx] = uh; }

    void          setSizeSubParts(UInt width, UInt height, UInt absPartIdx, UInt depth);

    char*         getQP()                        { return m_qp; }

    char          getQP(UInt uiIdx)            { return m_qp[uiIdx]; }

    void          setQP(UInt uiIdx, char value) { m_qp[uiIdx] =  value; }

    void          setQPSubParts(int qp,   UInt absPartIdx, UInt depth);
    int           getLastValidPartIdx(int iAbsPartIdx);
    char          getLastCodedQP(UInt absPartIdx);
    void          setQPSubCUs(int qp, TComDataCU* cu, UInt absPartIdx, UInt depth, bool &foundNonZeroCbf);
  
    bool          isLosslessCoded(UInt absPartIdx);

    UChar*        getTransformIdx()                        { return m_trIdx; }

    UChar         getTransformIdx(UInt uiIdx)            { return m_trIdx[uiIdx]; }

    void          setTrIdxSubParts(UInt uiTrIdx, UInt absPartIdx, UInt depth);

    UChar*        getTransformSkip(TextType ttype)    { return m_transformSkip[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getTransformSkip(UInt uiIdx, TextType ttype)    { return m_transformSkip[g_convertTxtTypeToIdx[ttype]][uiIdx]; }

    void          setTransformSkipSubParts(UInt useTransformSkip, TextType ttype, UInt absPartIdx, UInt depth);
    void          setTransformSkipSubParts(UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt absPartIdx, UInt depth);

    UInt          getQuadtreeTULog2MinSizeInCU(UInt absPartIdx);

    TComCUMvField* getCUMvField(RefPicList e)          { return &m_cuMvField[e]; }

    TCoeff*&      getCoeffY()                        { return m_trCoeffY; }

    TCoeff*&      getCoeffCb()                        { return m_trCoeffCb; }

    TCoeff*&      getCoeffCr()                        { return m_trCoeffCr; }

    Pel*&         getPCMSampleY()                        { return m_iPCMSampleY; }

    Pel*&         getPCMSampleCb()                        { return m_iPCMSampleCb; }

    Pel*&         getPCMSampleCr()                        { return m_iPCMSampleCr; }

    UChar         getCbf(UInt uiIdx, TextType ttype)                  { return m_cbf[g_convertTxtTypeToIdx[ttype]][uiIdx]; }

    UChar*        getCbf(TextType ttype)                              { return m_cbf[g_convertTxtTypeToIdx[ttype]]; }

    UChar         getCbf(UInt uiIdx, TextType ttype, UInt trDepth)  { return (getCbf(uiIdx, ttype) >> trDepth) & 0x1; }

    void          setCbf(UInt uiIdx, TextType ttype, UChar uh)        { m_cbf[g_convertTxtTypeToIdx[ttype]][uiIdx] = uh; }

    void          clearCbf(UInt uiIdx, TextType ttype, UInt uiNumParts);
    UChar         getQtRootCbf(UInt uiIdx)                      { return getCbf(uiIdx, TEXT_LUMA, 0) || getCbf(uiIdx, TEXT_CHROMA_U, 0) || getCbf(uiIdx, TEXT_CHROMA_V, 0); }

    void          setCbfSubParts(UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt absPartIdx, UInt depth);
    void          setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt depth);
    void          setCbfSubParts(UInt uiCbf, TextType eTType, UInt absPartIdx, UInt partIdx, UInt depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for coding tool information
    // -------------------------------------------------------------------------------------------------------------------

    bool*         getMergeFlag()                        { return m_bMergeFlags; }

    bool          getMergeFlag(UInt uiIdx)            { return m_bMergeFlags[uiIdx]; }

    void          setMergeFlag(UInt uiIdx, bool b)    { m_bMergeFlags[uiIdx] = b; }

    void          setMergeFlagSubParts(bool bMergeFlag, UInt absPartIdx, UInt partIdx, UInt depth);

    UChar*        getMergeIndex()                        { return m_mergeIndex; }

    UChar         getMergeIndex(UInt uiIdx)            { return m_mergeIndex[uiIdx]; }

    void          setMergeIndex(UInt uiIdx, UInt uiMergeIndex) { m_mergeIndex[uiIdx] = (UChar)uiMergeIndex; }

    void          setMergeIndexSubParts(UInt uiMergeIndex, UInt absPartIdx, UInt partIdx, UInt depth);
    template<typename T>
    void          setSubPart(T bParameter, T* pbBaseLCU, UInt cuAddr, UInt uiCUDepth, UInt uiPUIdx);

    void          setMergeAMP(bool b)      { m_bIsMergeAMP = b; }

    bool          getMergeAMP()             { return m_bIsMergeAMP; }

    UChar*        getLumaIntraDir()                        { return m_lumaIntraDir; }

    UChar         getLumaIntraDir(UInt uiIdx)            { return m_lumaIntraDir[uiIdx]; }

    void          setLumaIntraDir(UInt uiIdx, UChar  uh) { m_lumaIntraDir[uiIdx] = uh; }

    void          setLumaIntraDirSubParts(UInt uiDir,  UInt absPartIdx, UInt depth);

    UChar*        getChromaIntraDir()                        { return m_chromaIntraDir; }

    UChar         getChromaIntraDir(UInt uiIdx)            { return m_chromaIntraDir[uiIdx]; }

    void          setChromaIntraDir(UInt uiIdx, UChar  uh) { m_chromaIntraDir[uiIdx] = uh; }

    void          setChromIntraDirSubParts(UInt uiDir,  UInt absPartIdx, UInt depth);

    UChar*        getInterDir()                        { return m_interDir; }

    UChar         getInterDir(UInt uiIdx)            { return m_interDir[uiIdx]; }

    void          setInterDir(UInt uiIdx, UChar  uh) { m_interDir[uiIdx] = uh; }

    void          setInterDirSubParts(UInt uiDir,  UInt absPartIdx, UInt partIdx, UInt depth);
    bool*         getIPCMFlag()                        { return m_iPCMFlags; }

    bool          getIPCMFlag(UInt uiIdx)             { return m_iPCMFlags[uiIdx]; }

    void          setIPCMFlag(UInt uiIdx, bool b)     { m_iPCMFlags[uiIdx] = b; }

    void          setIPCMFlagSubParts(bool bIpcmFlag, UInt absPartIdx, UInt depth);

    std::vector<NDBFBlockInfo>* getNDBFilterBlocks()      { return &m_vNDFBlock; }

    void setNDBFilterBlockBorderAvailability(UInt numLCUInPicWidth, UInt numLCUInPicHeight, UInt numSUInLCUWidth, UInt numSUInLCUHeight, UInt picWidth, UInt picHeight
                                             , bool bTopTileBoundary, bool bDownTileBoundary, bool bLeftTileBoundary, bool bRightTileBoundary
                                             , bool bIndependentTileBoundaryEnabled);
    // -------------------------------------------------------------------------------------------------------------------
    // member functions for accessing partition information
    // -------------------------------------------------------------------------------------------------------------------

    void          getPartIndexAndSize(UInt partIdx, UInt& ruiPartAddr, int& riWidth, int& riHeight);
    UChar         getNumPartInter();
    bool          isFirstAbsZorderIdxInDepth(UInt absPartIdx, UInt depth);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for motion vector
    // -------------------------------------------------------------------------------------------------------------------

    void          getMvField(TComDataCU* cu, UInt absPartIdx, RefPicList picList, TComMvField& rcMvField);

    void          fillMvpCand(UInt partIdx, UInt partAddr, RefPicList picList, int refIdx, AMVPInfo* info);
    bool          isDiffMER(int xN, int yN, int xP, int yP);
    void          getPartPosition(UInt partIdx, int& xP, int& yP, int& nPSW, int& nPSH);
    void          setMVPIdx(RefPicList picList, UInt uiIdx, int mvpIdx)  { m_mvpIdx[picList][uiIdx] = (char)mvpIdx; }

    int           getMVPIdx(RefPicList picList, UInt uiIdx)               { return m_mvpIdx[picList][uiIdx]; }

    char*         getMVPIdx(RefPicList picList)                          { return m_mvpIdx[picList]; }

    void          setMVPNum(RefPicList picList, UInt uiIdx, int iMVPNum) { m_mvpNum[picList][uiIdx] = (char)iMVPNum; }

    int           getMVPNum(RefPicList picList, UInt uiIdx)              { return m_mvpNum[picList][uiIdx]; }

    char*         getMVPNum(RefPicList picList)                          { return m_mvpNum[picList]; }

    void          setMVPIdxSubParts(int mvpIdx, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth);
    void          setMVPNumSubParts(int iMVPNum, RefPicList picList, UInt absPartIdx, UInt partIdx, UInt depth);

    void          clipMv(MV& outMV);

    void          getMvPredLeft(MV& mvPred)       { mvPred = m_mvFieldA.mv; }

    void          getMvPredAbove(MV& mvPred)      { mvPred = m_mvFieldB.mv; }

    void          getMvPredAboveRight(MV& mvPred) { mvPred = m_mvFieldC.mv; }

    // -------------------------------------------------------------------------------------------------------------------
    // utility functions for neighboring information
    // -------------------------------------------------------------------------------------------------------------------

    TComDataCU*   getCULeft() { return m_cuLeft; }

    TComDataCU*   getCUAbove() { return m_cuAbove; }

    TComDataCU*   getCUAboveLeft() { return m_cuAboveLeft; }

    TComDataCU*   getCUAboveRight() { return m_cuAboveRight; }

    TComDataCU*   getCUColocated(RefPicList picList) { return m_cuColocated[picList]; }

    TComDataCU*   getPULeft(UInt& uiLPartUnitIdx,
                            UInt  curPartUnitIdx,
                            bool  bEnforceSliceRestriction = true,
                            bool  bEnforceTileRestriction = true);
    TComDataCU*   getPUAbove(UInt& uiAPartUnitIdx,
                             UInt  curPartUnitIdx,
                             bool  bEnforceSliceRestriction = true,
                             bool  planarAtLCUBoundary = false,
                             bool  bEnforceTileRestriction = true);
    TComDataCU*   getPUAboveLeft(UInt& alPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUAboveRight(UInt& uiARPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeft(UInt& uiBLPartUnitIdx, UInt curPartUnitIdx, bool bEnforceSliceRestriction = true);

    TComDataCU*   getQpMinCuLeft(UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInLCU);
    TComDataCU*   getQpMinCuAbove(UInt& aPartUnitIdx, UInt currAbsIdxInLCU);
    char          getRefQP(UInt uiCurrAbsIdxInLCU);

    TComDataCU*   getPUAboveRightAdi(UInt& uiARPartUnitIdx, UInt curPartUnitIdx, UInt partUnitOffset = 1, bool bEnforceSliceRestriction = true);
    TComDataCU*   getPUBelowLeftAdi(UInt& uiBLPartUnitIdx, UInt curPartUnitIdx, UInt partUnitOffset = 1, bool bEnforceSliceRestriction = true);

    void          deriveLeftRightTopIdx(UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT);
    void          deriveLeftBottomIdx(UInt partIdx, UInt& ruiPartIdxLB);

    void          deriveLeftRightTopIdxAdi(UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt partOffset, UInt partDepth);
    void          deriveLeftBottomIdxAdi(UInt& ruiPartIdxLB, UInt  partOffset, UInt partDepth);

    bool          hasEqualMotion(UInt absPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx);
    void          getInterMergeCandidates(UInt absPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, int& numValidMergeCand, int mrgCandIdx = -1);
    void          deriveLeftRightTopIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT);
    void          deriveLeftBottomIdxGeneral(UInt absPartIdx, UInt partIdx, UInt& ruiPartIdxLB);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for modes
    // -------------------------------------------------------------------------------------------------------------------

    bool          isIntra(UInt partIdx)  { return m_predModes[partIdx] == MODE_INTRA; }

    bool          isSkipped(UInt partIdx);                                                      ///< SKIP (no residual)
    bool          isBipredRestriction(UInt puIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for symbol prediction (most probable / mode conversion)
    // -------------------------------------------------------------------------------------------------------------------

    UInt          getIntraSizeIdx(UInt absPartIdx);

    void          getAllowedChromaDir(UInt absPartIdx, UInt* uiModeList);
    int           getIntraDirLumaPredictor(UInt absPartIdx, int* uiIntraDirPred, int* piMode = NULL);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for SBAC context
    // -------------------------------------------------------------------------------------------------------------------

    UInt          getCtxSplitFlag(UInt absPartIdx, UInt depth);
    UInt          getCtxQtCbf(TextType ttype, UInt trDepth);

    UInt          getCtxSkipFlag(UInt absPartIdx);
    UInt          getCtxInterDir(UInt absPartIdx);

    // -------------------------------------------------------------------------------------------------------------------
    // member functions for RD cost storage
    // -------------------------------------------------------------------------------------------------------------------

    UInt&         getTotalNumPart()               { return m_numPartitions; }

    UInt          getCoefScanIdx(UInt absPartIdx, UInt width, bool bIsLuma, bool bIsIntra);
};

namespace RasterAddress {
/** Check whether 2 addresses point to the same column
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualCol(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA % numUnitsPerRow == addrB % numUnitsPerRow
    return ((addrA ^ addrB) &  (numUnitsPerRow - 1)) == 0;
}

/** Check whether 2 addresses point to the same row
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualRow(int addrA, int addrB, int numUnitsPerRow)
{
    // addrA / numUnitsPerRow == addrB / numUnitsPerRow
    return ((addrA ^ addrB) & ~(numUnitsPerRow - 1)) == 0;
}

/** Check whether 2 addresses point to the same row or column
 * \param addrA          First address in raster scan order
 * \param addrB          Second address in raters scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isEqualRowOrCol(int addrA, int addrB, int numUnitsPerRow)
{
    return isEqualCol(addrA, addrB, numUnitsPerRow) | isEqualRow(addrA, addrB, numUnitsPerRow);
}

/** Check whether one address points to the first column
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isZeroCol(int addr, int numUnitsPerRow)
{
    // addr % numUnitsPerRow == 0
    return (addr & (numUnitsPerRow - 1)) == 0;
}

/** Check whether one address points to the first row
 * \param addr           Address in raster scan order
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool isZeroRow(int addr, int numUnitsPerRow)
{
    // addr / numUnitsPerRow == 0
    return (addr & ~(numUnitsPerRow - 1)) == 0;
}

/** Check whether one address points to a column whose index is smaller than a given value
 * \param addr           Address in raster scan order
 * \param val            Given column index value
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool lessThanCol(int addr, int val, int numUnitsPerRow)
{
    // addr % numUnitsPerRow < val
    return (addr & (numUnitsPerRow - 1)) < val;
}

/** Check whether one address points to a row whose index is smaller than a given value
 * \param addr           Address in raster scan order
 * \param val            Given row index value
 * \param numUnitsPerRow Number of units in a row
 * \return Result of test
 */
static inline bool lessThanRow(int addr, int val, int numUnitsPerRow)
{
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
}
}
}
//! \}

#endif // ifndef X265_TCOMDATACU_H
