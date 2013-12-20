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

/** \file     TComSlice.h
    \brief    slice header and SPS class (header)
*/

#ifndef X265_TCOMSLICE_H
#define X265_TCOMSLICE_H

#include "CommonDef.h"
#include "TComRom.h"
#include "TComList.h"
#include "x265.h"  // NAL type enums
#include "reference.h"

#include <cstring>
#include <map>
#include <vector>

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

class TComPic;
class TComTrQuant;
// ====================================================================================================================
// Constants
// ====================================================================================================================

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Reference Picture Set class
class TComReferencePictureSet
{
private:

    // Parameters for inter RPS prediction
    int  m_deltaRIdxMinus1;
    int  m_deltaRPS;
    int  m_numRefIdc;
    int  m_refIdc[MAX_NUM_REF_PICS + 1];

    // Parameters for long term references
    bool m_bCheckLTMSB[MAX_NUM_REF_PICS];
    int  m_pocLSBLT[MAX_NUM_REF_PICS];
    int  m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
    bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];

public:

    int  m_numberOfPictures;
    int  m_numberOfNegativePictures;
    int  m_numberOfPositivePictures;
    int  m_deltaPOC[MAX_NUM_REF_PICS];
    bool m_used[MAX_NUM_REF_PICS];
    int  m_POC[MAX_NUM_REF_PICS];

    bool m_interRPSPrediction;
    int  m_numberOfLongtermPictures;          // Zero when disabled


    TComReferencePictureSet();
    virtual ~TComReferencePictureSet();
    int   getPocLSBLT(int i)                       { return m_pocLSBLT[i]; }

    void  setPocLSBLT(int i, int x)                { m_pocLSBLT[i] = x; }

    int   getDeltaPocMSBCycleLT(int i)             { return m_deltaPOCMSBCycleLT[i]; }

    void  setDeltaPocMSBCycleLT(int i, int x)      { m_deltaPOCMSBCycleLT[i] = x; }

    bool  getDeltaPocMSBPresentFlag(int i)         { return m_deltaPocMSBPresentFlag[i]; }

    void  setDeltaPocMSBPresentFlag(int i, bool x) { m_deltaPocMSBPresentFlag[i] = x; }

    void setUsed(int bufferNum, bool used);
    void setDeltaPOC(int bufferNum, int deltaPOC);
    void setPOC(int bufferNum, int deltaPOC);
    void setNumberOfPictures(int numberOfPictures);
    void setCheckLTMSBPresent(int bufferNum, bool b);
    bool getCheckLTMSBPresent(int bufferNum);

    int  getUsed(int bufferNum) const;
    int  getDeltaPOC(int bufferNum) const;
    int  getPOC(int bufferNum) const;
    int  getNumberOfPictures() const;

    void setNumberOfNegativePictures(int number)  { m_numberOfNegativePictures = number; }

    int  getNumberOfNegativePictures() const      { return m_numberOfNegativePictures; }

    void setNumberOfPositivePictures(int number)  { m_numberOfPositivePictures = number; }

    int  getNumberOfPositivePictures() const      { return m_numberOfPositivePictures; }

    void setNumberOfLongtermPictures(int number)  { m_numberOfLongtermPictures = number; }

    int  getNumberOfLongtermPictures() const      { return m_numberOfLongtermPictures; }

    void setInterRPSPrediction(bool flag)         { m_interRPSPrediction = flag; }

    bool getInterRPSPrediction() const            { return m_interRPSPrediction; }

    void setDeltaRIdxMinus1(int x)                { m_deltaRIdxMinus1 = x; }

    int  getDeltaRIdxMinus1() const               { return m_deltaRIdxMinus1; }

    void setDeltaRPS(int x)                       { m_deltaRPS = x; }

    int  getDeltaRPS() const                      { return m_deltaRPS; }

    void setNumRefIdc(int x)                      { m_numRefIdc = x; }

    int  getNumRefIdc() const                     { return m_numRefIdc; }

    void setRefIdc(int bufferNum, int refIdc);
    int  getRefIdc(int bufferNum) const;

    void sortDeltaPOC();
    void printDeltaPOC();
};

/// Reference Picture Set set class
class TComRPSList
{
private:

    int  m_numberOfReferencePictureSets;
    TComReferencePictureSet* m_referencePictureSets;

public:

    TComRPSList();
    virtual ~TComRPSList();

    void  create(int numberOfEntries);
    void  destroy();

    TComReferencePictureSet* getReferencePictureSet(int referencePictureSetNum);
    int getNumberOfReferencePictureSets() const;
    void setNumberOfReferencePictureSets(int numberOfReferencePictureSets);
};

/// SCALING_LIST class
class TComScalingList
{
public:

    TComScalingList();
    virtual ~TComScalingList();
    void     setScalingListPresentFlag(bool b)                  { m_scalingListPresentFlag = b; }

    bool     getScalingListPresentFlag()                        { return m_scalingListPresentFlag; }

    bool     getUseTransformSkip()                              { return m_useTransformSkip; }

    void     setUseTransformSkip(bool b)                        { m_useTransformSkip = b; }

    int*     getScalingListAddress(UInt sizeId, UInt listId)    { return m_scalingListCoef[sizeId][listId]; }          //!< get matrix coefficient

    bool     checkPredMode(UInt sizeId, UInt listId);
    void     setRefMatrixId(UInt sizeId, UInt listId, UInt u)   { m_refMatrixId[sizeId][listId] = u;    }                    //!< set reference matrix ID

    UInt     getRefMatrixId(UInt sizeId, UInt listId)           { return m_refMatrixId[sizeId][listId]; }                    //!< get reference matrix ID

    int*     getScalingListDefaultAddress(UInt sizeId, UInt listId);                                                         //!< get default matrix coefficient
    void     processDefaultMarix(UInt sizeId, UInt listId);
    void     setScalingListDC(UInt sizeId, UInt listId, UInt u) { m_scalingListDC[sizeId][listId] = u; }                   //!< set DC value

    int      getScalingListDC(UInt sizeId, UInt listId)         { return m_scalingListDC[sizeId][listId]; }                //!< get DC value

    void     checkDcOfMatrix();
    void     processRefMatrix(UInt sizeId, UInt listId, UInt refListId);
    bool     xParseScalingList(char* pchFile);

private:

    void     init();
    void     destroy();
    int      m_scalingListDC[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];              //!< the DC value of the matrix coefficient for 16x16
    bool     m_useDefaultScalingMatrixFlag[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< UseDefaultScalingMatrixFlag
    UInt     m_refMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];                //!< RefMatrixID
    bool     m_scalingListPresentFlag;                                              //!< flag for using default matrix
    UInt     m_predMatrixId[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];               //!< reference list index
    int      *m_scalingListCoef[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];           //!< quantization matrix
    bool     m_useTransformSkip;                                                    //!< transform skipping flag for setting default scaling matrix for 4x4
};

class ProfileTierLevel
{
    int     m_profileSpace;
    bool    m_tierFlag;
    int     m_profileIdc;
    bool    m_profileCompatibilityFlag[32];
    int     m_levelIdc;
    bool    m_progressiveSourceFlag;
    bool    m_interlacedSourceFlag;
    bool    m_nonPackedConstraintFlag;
    bool    m_frameOnlyConstraintFlag;

public:

    ProfileTierLevel();

    int getProfileSpace() const { return m_profileSpace; }

    void setProfileSpace(int x) { m_profileSpace = x; }

    bool getTierFlag() const    { return m_tierFlag; }

    void setTierFlag(bool x)    { m_tierFlag = x; }

    int getProfileIdc() const   { return m_profileIdc; }

    void setProfileIdc(int x)   { m_profileIdc = x; }

    bool getProfileCompatibilityFlag(int i) const   { return m_profileCompatibilityFlag[i]; }

    void setProfileCompatibilityFlag(int i, bool x) { m_profileCompatibilityFlag[i] = x; }

    int getLevelIdc() const                 { return m_levelIdc; }

    void setLevelIdc(int x)                 { m_levelIdc = x; }

    bool getProgressiveSourceFlag() const   { return m_progressiveSourceFlag; }

    void setProgressiveSourceFlag(bool b)   { m_progressiveSourceFlag = b; }

    bool getInterlacedSourceFlag() const    { return m_interlacedSourceFlag; }

    void setInterlacedSourceFlag(bool b)    { m_interlacedSourceFlag = b; }

    bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }

    void setNonPackedConstraintFlag(bool b) { m_nonPackedConstraintFlag = b; }

    bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }

    void setFrameOnlyConstraintFlag(bool b) { m_frameOnlyConstraintFlag = b; }
};

class TComPTL
{
    ProfileTierLevel m_generalPTL;
    ProfileTierLevel m_subLayerPTL[6];    // max. value of max_sub_layers_minus1 is 6
    bool m_subLayerProfilePresentFlag[6];
    bool m_subLayerLevelPresentFlag[6];

public:

    TComPTL();
    bool getSubLayerProfilePresentFlag(int i) const { return m_subLayerProfilePresentFlag[i]; }

    void setSubLayerProfilePresentFlag(int i, bool x) { m_subLayerProfilePresentFlag[i] = x; }

    bool getSubLayerLevelPresentFlag(int i) const { return m_subLayerLevelPresentFlag[i]; }

    void setSubLayerLevelPresentFlag(int i, bool x) { m_subLayerLevelPresentFlag[i] = x; }

    ProfileTierLevel* getGeneralPTL() { return &m_generalPTL; }

    ProfileTierLevel* getSubLayerPTL(int i) { return &m_subLayerPTL[i]; }
};

/// VPS class

struct HrdSubLayerInfo
{
    bool fixedPicRateFlag;
    bool fixedPicRateWithinCvsFlag;
    UInt picDurationInTcMinus1;
    bool lowDelayHrdFlag;
    UInt cpbCntMinus1;
    UInt bitRateValueMinus1[MAX_CPB_CNT][2];
    UInt cpbSizeValue[MAX_CPB_CNT][2];
    UInt ducpbSizeValue[MAX_CPB_CNT][2];
    bool cbrFlag[MAX_CPB_CNT][2];
    UInt duBitRateValue[MAX_CPB_CNT][2];
};

class TComHRD
{
private:

    bool m_nalHrdParametersPresentFlag;
    bool m_vclHrdParametersPresentFlag;
    bool m_subPicCpbParamsPresentFlag;
    UInt m_tickDivisorMinus2;
    UInt m_duCpbRemovalDelayLengthMinus1;
    bool m_subPicCpbParamsInPicTimingSEIFlag;
    UInt m_dpbOutputDelayDuLengthMinus1;
    UInt m_bitRateScale;
    UInt m_cpbSizeScale;
    UInt m_ducpbSizeScale;
    UInt m_initialCpbRemovalDelayLengthMinus1;
    UInt m_cpbRemovalDelayLengthMinus1;
    UInt m_dpbOutputDelayLengthMinus1;
    UInt m_numDU;
    HrdSubLayerInfo m_HRD[MAX_TLAYER];

public:

    TComHRD()
        : m_nalHrdParametersPresentFlag(0)
        , m_vclHrdParametersPresentFlag(0)
        , m_subPicCpbParamsPresentFlag(false)
        , m_tickDivisorMinus2(0)
        , m_duCpbRemovalDelayLengthMinus1(0)
        , m_subPicCpbParamsInPicTimingSEIFlag(false)
        , m_dpbOutputDelayDuLengthMinus1(0)
        , m_bitRateScale(0)
        , m_cpbSizeScale(0)
        , m_initialCpbRemovalDelayLengthMinus1(0)
        , m_cpbRemovalDelayLengthMinus1(0)
        , m_dpbOutputDelayLengthMinus1(0)
    {}

    virtual ~TComHRD() {}

    void setNalHrdParametersPresentFlag(bool flag) { m_nalHrdParametersPresentFlag = flag; }

    bool getNalHrdParametersPresentFlag() { return m_nalHrdParametersPresentFlag; }

    void setVclHrdParametersPresentFlag(bool flag) { m_vclHrdParametersPresentFlag = flag; }

    bool getVclHrdParametersPresentFlag() { return m_vclHrdParametersPresentFlag; }

    void setSubPicCpbParamsPresentFlag(bool flag) { m_subPicCpbParamsPresentFlag = flag; }

    bool getSubPicCpbParamsPresentFlag() { return m_subPicCpbParamsPresentFlag; }

    void setTickDivisorMinus2(UInt value) { m_tickDivisorMinus2 = value; }

    UInt getTickDivisorMinus2() { return m_tickDivisorMinus2; }

    void setDuCpbRemovalDelayLengthMinus1(UInt value) { m_duCpbRemovalDelayLengthMinus1 = value; }

    UInt getDuCpbRemovalDelayLengthMinus1() { return m_duCpbRemovalDelayLengthMinus1; }

    void setSubPicCpbParamsInPicTimingSEIFlag(bool flag) { m_subPicCpbParamsInPicTimingSEIFlag = flag; }

    bool getSubPicCpbParamsInPicTimingSEIFlag() { return m_subPicCpbParamsInPicTimingSEIFlag; }

    void setDpbOutputDelayDuLengthMinus1(UInt value) { m_dpbOutputDelayDuLengthMinus1 = value; }

    UInt getDpbOutputDelayDuLengthMinus1() { return m_dpbOutputDelayDuLengthMinus1; }

    void setBitRateScale(UInt value) { m_bitRateScale = value; }

    UInt getBitRateScale() { return m_bitRateScale; }

    void setCpbSizeScale(UInt value) { m_cpbSizeScale = value; }

    UInt getCpbSizeScale() { return m_cpbSizeScale; }

    void setDuCpbSizeScale(UInt value) { m_ducpbSizeScale = value; }

    UInt getDuCpbSizeScale() { return m_ducpbSizeScale; }

    void setInitialCpbRemovalDelayLengthMinus1(UInt value) { m_initialCpbRemovalDelayLengthMinus1 = value; }

    UInt getInitialCpbRemovalDelayLengthMinus1() { return m_initialCpbRemovalDelayLengthMinus1; }

    void setCpbRemovalDelayLengthMinus1(UInt value) { m_cpbRemovalDelayLengthMinus1 = value; }

    UInt getCpbRemovalDelayLengthMinus1() { return m_cpbRemovalDelayLengthMinus1; }

    void setDpbOutputDelayLengthMinus1(UInt value) { m_dpbOutputDelayLengthMinus1 = value; }

    UInt getDpbOutputDelayLengthMinus1() { return m_dpbOutputDelayLengthMinus1; }

    void setFixedPicRateFlag(int layer, bool flag) { m_HRD[layer].fixedPicRateFlag = flag; }

    bool getFixedPicRateFlag(int layer) { return m_HRD[layer].fixedPicRateFlag; }

    void setFixedPicRateWithinCvsFlag(int layer, bool flag) { m_HRD[layer].fixedPicRateWithinCvsFlag = flag; }

    bool getFixedPicRateWithinCvsFlag(int layer) { return m_HRD[layer].fixedPicRateWithinCvsFlag; }

    void setPicDurationInTcMinus1(int layer, UInt value) { m_HRD[layer].picDurationInTcMinus1 = value; }

    UInt getPicDurationInTcMinus1(int layer) { return m_HRD[layer].picDurationInTcMinus1; }

    void setLowDelayHrdFlag(int layer, bool flag) { m_HRD[layer].lowDelayHrdFlag = flag; }

    bool getLowDelayHrdFlag(int layer) { return m_HRD[layer].lowDelayHrdFlag; }

    void setCpbCntMinus1(int layer, UInt value) { m_HRD[layer].cpbCntMinus1 = value; }

    UInt getCpbCntMinus1(int layer) { return m_HRD[layer].cpbCntMinus1; }

    void setBitRateValueMinus1(int layer, int cpbcnt, int nalOrVcl, UInt value) { m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl] = value; }

    UInt getBitRateValueMinus1(int layer, int cpbcnt, int nalOrVcl) { return m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl]; }

    void setCpbSizeValueMinus1(int layer, int cpbcnt, int nalOrVcl, UInt value) { m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl] = value; }

    UInt getCpbSizeValueMinus1(int layer, int cpbcnt, int nalOrVcl)  { return m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl]; }

    void setDuCpbSizeValueMinus1(int layer, int cpbcnt, int nalOrVcl, UInt value) { m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl] = value; }

    UInt getDuCpbSizeValueMinus1(int layer, int cpbcnt, int nalOrVcl)  { return m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl]; }

    void setDuBitRateValueMinus1(int layer, int cpbcnt, int nalOrVcl, UInt value) { m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl] = value; }

    UInt getDuBitRateValueMinus1(int layer, int cpbcnt, int nalOrVcl) { return m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl]; }

    void setCbrFlag(int layer, int cpbcnt, int nalOrVcl, bool value) { m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl] = value; }

    bool getCbrFlag(int layer, int cpbcnt, int nalOrVcl) { return m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl]; }

    void setNumDU(UInt value) { m_numDU = value; }

    UInt getNumDU()           { return m_numDU; }

    bool getCpbDpbDelaysPresentFlag() { return getNalHrdParametersPresentFlag() || getVclHrdParametersPresentFlag(); }
};

class TimingInfo
{
    bool m_timingInfoPresentFlag;
    UInt m_numUnitsInTick;
    UInt m_timeScale;
    bool m_pocProportionalToTimingFlag;
    int  m_numTicksPocDiffOneMinus1;

public:

    TimingInfo()
        : m_timingInfoPresentFlag(false)
        , m_numUnitsInTick(1001)
        , m_timeScale(60000)
        , m_pocProportionalToTimingFlag(false)
        , m_numTicksPocDiffOneMinus1(0) {}

    void setTimingInfoPresentFlag(bool flag)    { m_timingInfoPresentFlag = flag; }

    bool getTimingInfoPresentFlag()             { return m_timingInfoPresentFlag; }

    void setNumUnitsInTick(UInt value)          { m_numUnitsInTick = value; }

    UInt getNumUnitsInTick()                    { return m_numUnitsInTick; }

    void setTimeScale(UInt value)               { m_timeScale = value; }

    UInt getTimeScale()                         { return m_timeScale; }

    bool getPocProportionalToTimingFlag()       { return m_pocProportionalToTimingFlag; }

    void setPocProportionalToTimingFlag(bool x) { m_pocProportionalToTimingFlag = x; }

    int  getNumTicksPocDiffOneMinus1()          { return m_numTicksPocDiffOneMinus1; }

    void setNumTicksPocDiffOneMinus1(int x)     { m_numTicksPocDiffOneMinus1 = x; }
};

class TComVPS
{
private:

    int         m_VPSId;
    UInt        m_uiMaxTLayers;
    UInt        m_maxLayers;
    bool        m_bTemporalIdNestingFlag;

    UInt        m_numReorderPics[MAX_TLAYER];
    UInt        m_maxDecPicBuffering[MAX_TLAYER];
    UInt        m_maxLatencyIncrease[MAX_TLAYER];  // Really max latency increase plus 1 (value 0 expresses no limit)

    UInt        m_numHrdParameters;
    UInt        m_maxNuhReservedZeroLayerId;
    TComHRD*    m_hrdParameters;
    UInt*       m_hrdOpSetIdx;
    bool*       m_cprmsPresentFlag;
    UInt        m_numOpSets;
    bool        m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];

    TComPTL     m_ptl;
    TimingInfo  m_timingInfo;

public:

    TComVPS();
    virtual ~TComVPS();

    void    createHrdParamBuffer()
    {
        m_hrdParameters    = new TComHRD[getNumHrdParameters()];
        m_hrdOpSetIdx      = new UInt[getNumHrdParameters()];
        m_cprmsPresentFlag = new bool[getNumHrdParameters()];
    }

    TComHRD* getHrdParameters(UInt i)             { return &m_hrdParameters[i]; }

    UInt    getHrdOpSetIdx(UInt i)                { return m_hrdOpSetIdx[i]; }

    void    setHrdOpSetIdx(UInt val, UInt i)      { m_hrdOpSetIdx[i] = val; }

    bool    getCprmsPresentFlag(UInt i)           { return m_cprmsPresentFlag[i]; }

    void    setCprmsPresentFlag(bool val, UInt i) { m_cprmsPresentFlag[i] = val; }

    int     getVPSId()                            { return m_VPSId; }

    void    setVPSId(int i)                       { m_VPSId = i; }

    UInt    getMaxTLayers()                       { return m_uiMaxTLayers; }

    void    setMaxTLayers(UInt t)                 { m_uiMaxTLayers = t; }

    UInt    getMaxLayers()                        { return m_maxLayers; }

    void    setMaxLayers(UInt l)                  { m_maxLayers = l; }

    bool    getTemporalNestingFlag()              { return m_bTemporalIdNestingFlag; }

    void    setTemporalNestingFlag(bool t)        { m_bTemporalIdNestingFlag = t; }

    void    setNumReorderPics(UInt v, UInt tLayer)     { m_numReorderPics[tLayer] = v; }

    UInt    getNumReorderPics(UInt tLayer)             { return m_numReorderPics[tLayer]; }

    void    setMaxDecPicBuffering(UInt v, UInt tLayer) { m_maxDecPicBuffering[tLayer] = v; }

    UInt    getMaxDecPicBuffering(UInt tLayer)         { return m_maxDecPicBuffering[tLayer]; }

    void    setMaxLatencyIncrease(UInt v, UInt tLayer) { m_maxLatencyIncrease[tLayer] = v; }

    UInt    getMaxLatencyIncrease(UInt tLayer)         { return m_maxLatencyIncrease[tLayer]; }

    UInt    getNumHrdParameters()                      { return m_numHrdParameters; }

    void    setNumHrdParameters(UInt v)                { m_numHrdParameters = v; }

    UInt    getMaxNuhReservedZeroLayerId()             { return m_maxNuhReservedZeroLayerId; }

    void    setMaxNuhReservedZeroLayerId(UInt v)       { m_maxNuhReservedZeroLayerId = v; }

    UInt    getMaxOpSets()                             { return m_numOpSets; }

    void    setMaxOpSets(UInt v)                       { m_numOpSets = v; }

    bool    getLayerIdIncludedFlag(UInt opsIdx, UInt id)        { return m_layerIdIncludedFlag[opsIdx][id]; }

    void    setLayerIdIncludedFlag(bool v, UInt opsIdx, UInt id) { m_layerIdIncludedFlag[opsIdx][id] = v; }

    TComPTL* getPTL()           { return &m_ptl; }

    TimingInfo* getTimingInfo() { return &m_timingInfo; }
};

class Window
{
private:

    bool          m_enabledFlag;
    int           m_winLeftOffset;
    int           m_winRightOffset;
    int           m_winTopOffset;
    int           m_winBottomOffset;

public:

    Window()
        : m_enabledFlag(false)
        , m_winLeftOffset(0)
        , m_winRightOffset(0)
        , m_winTopOffset(0)
        , m_winBottomOffset(0)
    {}

    bool          getWindowEnabledFlag() const      { return m_enabledFlag; }

    void          resetWindow()                     { m_enabledFlag = false; m_winLeftOffset = m_winRightOffset = m_winTopOffset = m_winBottomOffset = 0; }

    int           getWindowLeftOffset() const       { return m_enabledFlag ? m_winLeftOffset : 0; }

    void          setWindowLeftOffset(int val)      { m_winLeftOffset = val; m_enabledFlag = true; }

    int           getWindowRightOffset() const      { return m_enabledFlag ? m_winRightOffset : 0; }

    void          setWindowRightOffset(int val)     { m_winRightOffset = val; m_enabledFlag = true; }

    int           getWindowTopOffset() const        { return m_enabledFlag ? m_winTopOffset : 0; }

    void          setWindowTopOffset(int val)       { m_winTopOffset = val; m_enabledFlag = true; }

    int           getWindowBottomOffset() const     { return m_enabledFlag ? m_winBottomOffset : 0; }

    void          setWindowBottomOffset(int val)    { m_winBottomOffset = val; m_enabledFlag = true; }

    void          setWindow(int offsetLeft, int offsetLRight, int offsetLTop, int offsetLBottom)
    {
        m_enabledFlag       = true;
        m_winLeftOffset     = offsetLeft;
        m_winRightOffset    = offsetLRight;
        m_winTopOffset      = offsetLTop;
        m_winBottomOffset   = offsetLBottom;
    }
};

class TComVUI
{
private:

    bool m_aspectRatioInfoPresentFlag;
    int  m_aspectRatioIdc;
    int  m_sarWidth;
    int  m_sarHeight;
    bool m_overscanInfoPresentFlag;
    bool m_overscanAppropriateFlag;
    bool m_videoSignalTypePresentFlag;
    int  m_videoFormat;
    bool m_videoFullRangeFlag;
    bool m_colourDescriptionPresentFlag;
    int  m_colourPrimaries;
    int  m_transferCharacteristics;
    int  m_matrixCoefficients;
    bool m_chromaLocInfoPresentFlag;
    int  m_chromaSampleLocTypeTopField;
    int  m_chromaSampleLocTypeBottomField;
    bool m_neutralChromaIndicationFlag;
    bool m_fieldSeqFlag;

    Window m_defaultDisplayWindow;
    bool m_frameFieldInfoPresentFlag;
    bool m_hrdParametersPresentFlag;
    bool m_bitstreamRestrictionFlag;
    bool m_motionVectorsOverPicBoundariesFlag;
    bool m_restrictedRefPicListsFlag;
    int  m_minSpatialSegmentationIdc;
    int  m_maxBytesPerPicDenom;
    int  m_maxBitsPerMinCuDenom;
    int  m_log2MaxMvLengthHorizontal;
    int  m_log2MaxMvLengthVertical;
    TComHRD m_hrdParameters;
    TimingInfo m_timingInfo;

public:

    TComVUI()
        : m_aspectRatioInfoPresentFlag(false)
        , m_aspectRatioIdc(0)
        , m_sarWidth(0)
        , m_sarHeight(0)
        , m_overscanInfoPresentFlag(false)
        , m_overscanAppropriateFlag(false)
        , m_videoSignalTypePresentFlag(false)
        , m_videoFormat(5)
        , m_videoFullRangeFlag(false)
        , m_colourDescriptionPresentFlag(false)
        , m_colourPrimaries(2)
        , m_transferCharacteristics(2)
        , m_matrixCoefficients(2)
        , m_chromaLocInfoPresentFlag(false)
        , m_chromaSampleLocTypeTopField(0)
        , m_chromaSampleLocTypeBottomField(0)
        , m_neutralChromaIndicationFlag(false)
        , m_fieldSeqFlag(false)
        , m_frameFieldInfoPresentFlag(false)
        , m_hrdParametersPresentFlag(false)
        , m_bitstreamRestrictionFlag(false)
        , m_motionVectorsOverPicBoundariesFlag(true)
        , m_restrictedRefPicListsFlag(1)
        , m_minSpatialSegmentationIdc(0)
        , m_maxBytesPerPicDenom(2)
        , m_maxBitsPerMinCuDenom(1)
        , m_log2MaxMvLengthHorizontal(15)
        , m_log2MaxMvLengthVertical(15)
    {}

    virtual ~TComVUI() {}

    bool getAspectRatioInfoPresentFlag() { return m_aspectRatioInfoPresentFlag; }

    void setAspectRatioInfoPresentFlag(bool i) { m_aspectRatioInfoPresentFlag = i; }

    int getAspectRatioIdc() { return m_aspectRatioIdc; }

    void setAspectRatioIdc(int i) { m_aspectRatioIdc = i; }

    int getSarWidth() { return m_sarWidth; }

    void setSarWidth(int i) { m_sarWidth = i; }

    int getSarHeight() { return m_sarHeight; }

    void setSarHeight(int i) { m_sarHeight = i; }

    bool getOverscanInfoPresentFlag() { return m_overscanInfoPresentFlag; }

    void setOverscanInfoPresentFlag(bool i) { m_overscanInfoPresentFlag = i; }

    bool getOverscanAppropriateFlag() { return m_overscanAppropriateFlag; }

    void setOverscanAppropriateFlag(bool i) { m_overscanAppropriateFlag = i; }

    bool getVideoSignalTypePresentFlag() { return m_videoSignalTypePresentFlag; }

    void setVideoSignalTypePresentFlag(bool i) { m_videoSignalTypePresentFlag = i; }

    int getVideoFormat() { return m_videoFormat; }

    void setVideoFormat(int i) { m_videoFormat = i; }

    bool getVideoFullRangeFlag() { return m_videoFullRangeFlag; }

    void setVideoFullRangeFlag(bool i) { m_videoFullRangeFlag = i; }

    bool getColourDescriptionPresentFlag() { return m_colourDescriptionPresentFlag; }

    void setColourDescriptionPresentFlag(bool i) { m_colourDescriptionPresentFlag = i; }

    int getColourPrimaries() { return m_colourPrimaries; }

    void setColourPrimaries(int i) { m_colourPrimaries = i; }

    int getTransferCharacteristics() { return m_transferCharacteristics; }

    void setTransferCharacteristics(int i) { m_transferCharacteristics = i; }

    int getMatrixCoefficients() { return m_matrixCoefficients; }

    void setMatrixCoefficients(int i) { m_matrixCoefficients = i; }

    bool getChromaLocInfoPresentFlag() { return m_chromaLocInfoPresentFlag; }

    void setChromaLocInfoPresentFlag(bool i) { m_chromaLocInfoPresentFlag = i; }

    int getChromaSampleLocTypeTopField() { return m_chromaSampleLocTypeTopField; }

    void setChromaSampleLocTypeTopField(int i) { m_chromaSampleLocTypeTopField = i; }

    int getChromaSampleLocTypeBottomField() { return m_chromaSampleLocTypeBottomField; }

    void setChromaSampleLocTypeBottomField(int i) { m_chromaSampleLocTypeBottomField = i; }

    bool getNeutralChromaIndicationFlag() { return m_neutralChromaIndicationFlag; }

    void setNeutralChromaIndicationFlag(bool i) { m_neutralChromaIndicationFlag = i; }

    bool getFieldSeqFlag() { return m_fieldSeqFlag; }

    void setFieldSeqFlag(bool i) { m_fieldSeqFlag = i; }

    bool getFrameFieldInfoPresentFlag() { return m_frameFieldInfoPresentFlag; }

    void setFrameFieldInfoPresentFlag(bool i) { m_frameFieldInfoPresentFlag = i; }

    Window& getDefaultDisplayWindow() { return m_defaultDisplayWindow; }

    void setDefaultDisplayWindow(Window& defaultDisplayWindow) { m_defaultDisplayWindow = defaultDisplayWindow; }

    bool getHrdParametersPresentFlag() { return m_hrdParametersPresentFlag; }

    void setHrdParametersPresentFlag(bool i) { m_hrdParametersPresentFlag = i; }

    bool getBitstreamRestrictionFlag() { return m_bitstreamRestrictionFlag; }

    void setBitstreamRestrictionFlag(bool i) { m_bitstreamRestrictionFlag = i; }

    bool getMotionVectorsOverPicBoundariesFlag() { return m_motionVectorsOverPicBoundariesFlag; }

    void setMotionVectorsOverPicBoundariesFlag(bool i) { m_motionVectorsOverPicBoundariesFlag = i; }

    bool getRestrictedRefPicListsFlag() { return m_restrictedRefPicListsFlag; }

    void setRestrictedRefPicListsFlag(bool b) { m_restrictedRefPicListsFlag = b; }

    int getMinSpatialSegmentationIdc() { return m_minSpatialSegmentationIdc; }

    void setMinSpatialSegmentationIdc(int i) { m_minSpatialSegmentationIdc = i; }

    int getMaxBytesPerPicDenom() { return m_maxBytesPerPicDenom; }

    void setMaxBytesPerPicDenom(int i) { m_maxBytesPerPicDenom = i; }

    int getMaxBitsPerMinCuDenom() { return m_maxBitsPerMinCuDenom; }

    void setMaxBitsPerMinCuDenom(int i) { m_maxBitsPerMinCuDenom = i; }

    int getLog2MaxMvLengthHorizontal() { return m_log2MaxMvLengthHorizontal; }

    void setLog2MaxMvLengthHorizontal(int i) { m_log2MaxMvLengthHorizontal = i; }

    int getLog2MaxMvLengthVertical() { return m_log2MaxMvLengthVertical; }

    void setLog2MaxMvLengthVertical(int i) { m_log2MaxMvLengthVertical = i; }

    TComHRD* getHrdParameters() { return &m_hrdParameters; }

    TimingInfo* getTimingInfo() { return &m_timingInfo; }
};

/// SPS class
class TComSPS
{
private:

    int         m_SPSId;
    int         m_VPSId;
    int         m_chromaFormatIdc;

    UInt        m_maxTLayers;         // maximum number of temporal layers

    // Structure
    UInt        m_picWidthInLumaSamples;
    UInt        m_picHeightInLumaSamples;

    int         m_log2MinCodingBlockSize;
    int         m_log2DiffMaxMinCodingBlockSize;
    UInt        m_maxCUWidth;
    UInt        m_maxCUHeight;
    UInt        m_maxCUDepth;

    Window      m_conformanceWindow;

    TComRPSList m_RPSList;
    bool        m_bLongTermRefsPresent;
    bool        m_TMVPFlagsPresent;
    int         m_numReorderPics[MAX_TLAYER];

    // Tool list
    UInt        m_quadtreeTULog2MaxSize;
    UInt        m_quadtreeTULog2MinSize;
    UInt        m_quadtreeTUMaxDepthInter;
    UInt        m_quadtreeTUMaxDepthIntra;
    bool        m_usePCM;
    UInt        m_pcmLog2MaxSize;
    UInt        m_pcmLog2MinSize;
    bool        m_useAMP;

    // Parameter
    int         m_bitDepthY;
    int         m_bitDepthC;
    int         m_qpBDOffsetY;
    int         m_qpBDOffsetC;

    bool        m_useLossless;

    UInt        m_pcmBitDepthLuma;
    UInt        m_pcmBitDepthChroma;
    bool        m_bPCMFilterDisableFlag;

    UInt        m_bitsForPOC;
    UInt        m_numLongTermRefPicSPS;
    UInt        m_ltRefPicPocLsbSps[33];
    bool        m_usedByCurrPicLtSPSFlag[33];

    // Max physical transform size
    UInt        m_maxTrSize;

    int m_iAMPAcc[MAX_CU_DEPTH];
    bool        m_bUseSAO;

    bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

    bool        m_scalingListEnabledFlag;
    bool        m_scalingListPresentFlag;
    TComScalingList* m_scalingList; //!< ScalingList class pointer
    UInt        m_maxDecPicBuffering[MAX_TLAYER];
    UInt        m_maxLatencyIncrease[MAX_TLAYER]; // Really max latency increase plus 1 (value 0 expresses no limit)

    bool        m_useDF;
    bool        m_useStrongIntraSmoothing;

    bool        m_vuiParametersPresentFlag;
    TComVUI     m_vuiParameters;
    TComPTL     m_ptl;

public:

    TComSPS();
    virtual ~TComSPS();

    int  getVPSId()         { return m_VPSId; }

    void setVPSId(int i)    { m_VPSId = i; }

    int  getSPSId()         { return m_SPSId; }

    void setSPSId(int i)    { m_SPSId = i; }

    int  getChromaFormatIdc()         { return m_chromaFormatIdc; }

    void setChromaFormatIdc(int i)    { m_chromaFormatIdc = i; }

    static int getWinUnitX(int chromaFormatIdc) { assert(chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return g_winUnitX[chromaFormatIdc]; }

    static int getWinUnitY(int chromaFormatIdc) { assert(chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return g_winUnitY[chromaFormatIdc]; }

    // structure
    void setPicWidthInLumaSamples(UInt u) { m_picWidthInLumaSamples = u; }

    UInt getPicWidthInLumaSamples() const { return m_picWidthInLumaSamples; }

    void setPicHeightInLumaSamples(UInt u) { m_picHeightInLumaSamples = u; }

    UInt getPicHeightInLumaSamples() const { return m_picHeightInLumaSamples; }

    Window& getConformanceWindow() { return m_conformanceWindow; }

    void    setConformanceWindow(Window& conformanceWindow) { m_conformanceWindow = conformanceWindow; }

    UInt  getNumLongTermRefPicSPS() const { return m_numLongTermRefPicSPS; }

    void  setNumLongTermRefPicSPS(UInt val) { m_numLongTermRefPicSPS = val; }

    UInt  getLtRefPicPocLsbSps(UInt index) const { return m_ltRefPicPocLsbSps[index]; }

    void  setLtRefPicPocLsbSps(UInt index, UInt val) { m_ltRefPicPocLsbSps[index] = val; }

    bool getUsedByCurrPicLtSPSFlag(int i) const { return m_usedByCurrPicLtSPSFlag[i]; }

    void setUsedByCurrPicLtSPSFlag(int i, bool x) { m_usedByCurrPicLtSPSFlag[i] = x; }

    int  getLog2MinCodingBlockSize() const { return m_log2MinCodingBlockSize; }

    void setLog2MinCodingBlockSize(int val) { m_log2MinCodingBlockSize = val; }

    int  getLog2DiffMaxMinCodingBlockSize() const { return m_log2DiffMaxMinCodingBlockSize; }

    void setLog2DiffMaxMinCodingBlockSize(int val) { m_log2DiffMaxMinCodingBlockSize = val; }

    void setMaxCUWidth(UInt u) { m_maxCUWidth = u; }

    UInt getMaxCUWidth() const  { return m_maxCUWidth; }

    void setMaxCUHeight(UInt u) { m_maxCUHeight = u; }

    UInt getMaxCUHeight() const { return m_maxCUHeight; }

    void setMaxCUDepth(UInt u) { m_maxCUDepth = u; }

    UInt getMaxCUDepth() const { return m_maxCUDepth; }

    void setUsePCM(bool b)   { m_usePCM = b; }

    bool getUsePCM() const   { return m_usePCM; }

    void setPCMLog2MaxSize(UInt u) { m_pcmLog2MaxSize = u; }

    UInt getPCMLog2MaxSize() const { return m_pcmLog2MaxSize; }

    void setPCMLog2MinSize(UInt u) { m_pcmLog2MinSize = u; }

    UInt getPCMLog2MinSize() const { return m_pcmLog2MinSize; }

    void setBitsForPOC(UInt u) { m_bitsForPOC = u; }

    UInt getBitsForPOC() const { return m_bitsForPOC; }

    bool getUseAMP() { return m_useAMP; }

    void setUseAMP(bool b) { m_useAMP = b; }

    void setQuadtreeTULog2MaxSize(UInt u)   { m_quadtreeTULog2MaxSize = u; }

    UInt getQuadtreeTULog2MaxSize() const   { return m_quadtreeTULog2MaxSize; }

    void setQuadtreeTULog2MinSize(UInt u)   { m_quadtreeTULog2MinSize = u; }

    UInt getQuadtreeTULog2MinSize() const   { return m_quadtreeTULog2MinSize; }

    void setQuadtreeTUMaxDepthInter(UInt u) { m_quadtreeTUMaxDepthInter = u; }

    void setQuadtreeTUMaxDepthIntra(UInt u) { m_quadtreeTUMaxDepthIntra = u; }

    UInt getQuadtreeTUMaxDepthInter() const   { return m_quadtreeTUMaxDepthInter; }

    UInt getQuadtreeTUMaxDepthIntra() const    { return m_quadtreeTUMaxDepthIntra; }

    void setNumReorderPics(int i, UInt tlayer) { m_numReorderPics[tlayer] = i; }

    int  getNumReorderPics(UInt tlayer) const  { return m_numReorderPics[tlayer]; }

    void createRPSList(int numRPS);

    TComRPSList* getRPSList()                  { return &m_RPSList; }

    bool      getLongTermRefsPresent() const   { return m_bLongTermRefsPresent; }

    void      setLongTermRefsPresent(bool b)   { m_bLongTermRefsPresent = b; }

    bool      getTMVPFlagsPresent() const   { return m_TMVPFlagsPresent; }

    void      setTMVPFlagsPresent(bool b)   { m_TMVPFlagsPresent = b; }

    // physical transform
    void setMaxTrSize(UInt u)   { m_maxTrSize = u; }

    UInt getMaxTrSize() const   { return m_maxTrSize; }

    // Tool list
    bool getUseLossless() const { return m_useLossless; }

    void setUseLossless(bool b) { m_useLossless  = b; }

    // AMP accuracy
    int       getAMPAcc(UInt depth) const { return m_iAMPAcc[depth]; }

    void      setAMPAcc(UInt depth, int iAccu) { assert(depth < g_maxCUDepth);  m_iAMPAcc[depth] = iAccu; }

    // Bit-depth
    int      getBitDepthY() const { return m_bitDepthY; }

    void     setBitDepthY(int u) { m_bitDepthY = u; }

    int      getBitDepthC() const { return m_bitDepthC; }

    void     setBitDepthC(int u) { m_bitDepthC = u; }

    int       getQpBDOffsetY() const { return m_qpBDOffsetY; }

    void      setQpBDOffsetY(int value) { m_qpBDOffsetY = value; }

    int       getQpBDOffsetC() const { return m_qpBDOffsetC; }

    void      setQpBDOffsetC(int value) { m_qpBDOffsetC = value; }

    void setUseSAO(bool bVal)  { m_bUseSAO = bVal; }

    bool getUseSAO() const { return m_bUseSAO; }

    UInt      getMaxTLayers() const                   { return m_maxTLayers; }

    void      setMaxTLayers(UInt maxTLayers)          { assert(maxTLayers <= MAX_TLAYER); m_maxTLayers = maxTLayers; }

    bool      getTemporalIdNestingFlag() const        { return m_bTemporalIdNestingFlag; }

    void      setTemporalIdNestingFlag(bool bValue)   { m_bTemporalIdNestingFlag = bValue; }

    UInt      getPCMBitDepthLuma() const { return m_pcmBitDepthLuma; }

    void      setPCMBitDepthLuma(UInt u) { m_pcmBitDepthLuma = u; }

    UInt      getPCMBitDepthChroma() const { return m_pcmBitDepthChroma; }

    void      setPCMBitDepthChroma(UInt u) { m_pcmBitDepthChroma = u; }

    void      setPCMFilterDisableFlag(bool bValue)    { m_bPCMFilterDisableFlag = bValue; }

    bool      getPCMFilterDisableFlag() const         { return m_bPCMFilterDisableFlag; }

    bool getScalingListFlag() const { return m_scalingListEnabledFlag; }

    void setScalingListFlag(bool b) { m_scalingListEnabledFlag = b; }

    bool getScalingListPresentFlag() const { return m_scalingListPresentFlag; }

    void setScalingListPresentFlag(bool b) { m_scalingListPresentFlag = b; }

    void setScalingList(TComScalingList *scalingList);
    TComScalingList* getScalingList() { return m_scalingList; } //!< get ScalingList class pointer in SPS

    UInt getMaxDecPicBuffering(UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }

    void setMaxDecPicBuffering(UInt ui, UInt tlayer) { m_maxDecPicBuffering[tlayer] = ui; }

    UInt getMaxLatencyIncrease(UInt tlayer) { return m_maxLatencyIncrease[tlayer]; }

    void setMaxLatencyIncrease(UInt ui, UInt tlayer) { m_maxLatencyIncrease[tlayer] = ui; }

    void setUseStrongIntraSmoothing(bool bVal) { m_useStrongIntraSmoothing = bVal; }

    bool getUseStrongIntraSmoothing() const { return m_useStrongIntraSmoothing; }

    bool getVuiParametersPresentFlag() { return m_vuiParametersPresentFlag; }

    void setVuiParametersPresentFlag(bool b) { m_vuiParametersPresentFlag = b; }

    TComVUI* getVuiParameters() { return &m_vuiParameters; }

    void setHrdParameters(UInt frameRate, UInt numDU, UInt bitRate, bool randomAccess);

    TComPTL* getPTL() { return &m_ptl; }
};

/// Reference Picture Lists class
class TComRefPicListModification
{
private:

    bool m_bRefPicListModificationFlagL0;
    bool m_bRefPicListModificationFlagL1;
    UInt m_RefPicSetIdxL0[32];
    UInt m_RefPicSetIdxL1[32];

public:

    TComRefPicListModification();
    virtual ~TComRefPicListModification();

    void create();
    void destroy();

    bool getRefPicListModificationFlagL0() { return m_bRefPicListModificationFlagL0; }

    void setRefPicListModificationFlagL0(bool flag) { m_bRefPicListModificationFlagL0 = flag; }

    bool getRefPicListModificationFlagL1() { return m_bRefPicListModificationFlagL1; }

    void setRefPicListModificationFlagL1(bool flag) { m_bRefPicListModificationFlagL1 = flag; }

    void setRefPicSetIdxL0(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL0[idx] = refPicSetIdx; }

    UInt getRefPicSetIdxL0(UInt idx) { return m_RefPicSetIdxL0[idx]; }

    void setRefPicSetIdxL1(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL1[idx] = refPicSetIdx; }

    UInt getRefPicSetIdxL1(UInt idx) { return m_RefPicSetIdxL1[idx]; }
};

/// PPS class
class TComPPS
{
private:

    int         m_PPSId;                  // pic_parameter_set_id
    int         m_SPSId;                  // seq_parameter_set_id
    int         m_picInitQPMinus26;
    bool        m_useDQP;
    bool        m_bConstrainedIntraPred;  // constrained_intra_pred_flag
    bool        m_bSliceChromaQpFlag;     // slicelevel_chroma_qp_flag

    // access channel
    TComSPS*    m_sps;
    UInt        m_maxCuDQPDepth;
    UInt        m_minCuDQPSize;

    int         m_chromaCbQpOffset;
    int         m_chromaCrQpOffset;

    UInt        m_numRefIdxL0DefaultActive;
    UInt        m_numRefIdxL1DefaultActive;

    bool        m_bUseWeightPred;         // Use of Weighting Prediction (P_SLICE)
    bool        m_useWeightedBiPred;      // Use of Weighting Bi-Prediction (B_SLICE)
    bool        m_outputFlagPresentFlag; // Indicates the presence of output_flag in slice header

    bool        m_transquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
    bool        m_useTransformSkip;
    bool        m_entropyCodingSyncEnabledFlag; //!< Indicates the presence of wavefronts

    bool     m_loopFilterAcrossTilesEnabledFlag;

    int      m_signHideFlag;

    bool     m_cabacInitPresentFlag;
    UInt     m_encCABACTableIdx;         // Used to transmit table selection across slices

    bool     m_sliceHeaderExtensionPresentFlag;
    bool     m_deblockingFilterControlPresentFlag;
    bool     m_deblockingFilterOverrideEnabledFlag;
    bool     m_picDisableDeblockingFilterFlag;
    int      m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    int      m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    bool     m_scalingListPresentFlag;
    TComScalingList* m_scalingList; //!< ScalingList class pointer
    bool m_listsModificationPresentFlag;
    UInt m_log2ParallelMergeLevelMinus2;
    int m_numExtraSliceHeaderBits;

public:

    TComPPS();
    virtual ~TComPPS();

    int       getPPSId() const { return m_PPSId; }

    void      setPPSId(int i) { m_PPSId = i; }

    int       getSPSId() const{ return m_SPSId; }

    void      setSPSId(int i) { m_SPSId = i; }

    int       getPicInitQPMinus26() const { return m_picInitQPMinus26; }

    void      setPicInitQPMinus26(int i)  { m_picInitQPMinus26 = i; }

    bool      getUseDQP() const           { return m_useDQP; }

    void      setUseDQP(bool b)           { m_useDQP = b; }

    bool      getConstrainedIntraPred() const { return m_bConstrainedIntraPred; }

    void      setConstrainedIntraPred(bool b) { m_bConstrainedIntraPred = b; }

    bool      getSliceChromaQpFlag() const { return m_bSliceChromaQpFlag; }

    void      setSliceChromaQpFlag(bool b) { m_bSliceChromaQpFlag = b; }

    void      setSPS(TComSPS* sps) { m_sps = sps; }

    TComSPS*  getSPS() { return m_sps; }

    void      setMaxCuDQPDepth(UInt u) { m_maxCuDQPDepth = u; }

    UInt      getMaxCuDQPDepth() const { return m_maxCuDQPDepth; }

    void      setMinCuDQPSize(UInt u) { m_minCuDQPSize = u; }

    UInt      getMinCuDQPSize() const { return m_minCuDQPSize; }

    void      setChromaCbQpOffset(int i) { m_chromaCbQpOffset = i; }

    int       getChromaCbQpOffset() const { return m_chromaCbQpOffset; }

    void      setChromaCrQpOffset(int i) { m_chromaCrQpOffset = i; }

    int       getChromaCrQpOffset() const { return m_chromaCrQpOffset; }

    void      setNumRefIdxL0DefaultActive(UInt i)    { m_numRefIdxL0DefaultActive = i; }

    UInt      getNumRefIdxL0DefaultActive() const     { return m_numRefIdxL0DefaultActive; }

    void      setNumRefIdxL1DefaultActive(UInt i)    { m_numRefIdxL1DefaultActive = i; }

    UInt      getNumRefIdxL1DefaultActive() const     { return m_numRefIdxL1DefaultActive; }

    bool getUseWP() const    { return m_bUseWeightPred; }

    bool getWPBiPred() const { return m_useWeightedBiPred; }

    void setUseWP(bool b)    { m_bUseWeightPred = b; }

    void setWPBiPred(bool b) { m_useWeightedBiPred = b; }

    void      setOutputFlagPresentFlag(bool b)  { m_outputFlagPresentFlag = b; }

    bool      getOutputFlagPresentFlag() const  { return m_outputFlagPresentFlag; }

    void      setTransquantBypassEnableFlag(bool b) { m_transquantBypassEnableFlag = b; }

    bool      getTransquantBypassEnableFlag() const { return m_transquantBypassEnableFlag; }

    bool      getUseTransformSkip() const { return m_useTransformSkip; }

    void      setUseTransformSkip(bool b) { m_useTransformSkip = b; }

    void    setLoopFilterAcrossTilesEnabledFlag(bool b) { m_loopFilterAcrossTilesEnabledFlag = b; }

    bool    getLoopFilterAcrossTilesEnabledFlag()      { return m_loopFilterAcrossTilesEnabledFlag; }

    bool    getEntropyCodingSyncEnabledFlag() const    { return m_entropyCodingSyncEnabledFlag; }

    void    setEntropyCodingSyncEnabledFlag(bool val)  { m_entropyCodingSyncEnabledFlag = val; }

    void    setSignHideFlag(int signHideFlag)       { m_signHideFlag = signHideFlag; }

    int     getSignHideFlag() const                 { return m_signHideFlag; }

    void     setCabacInitPresentFlag(bool flag)     { m_cabacInitPresentFlag = flag; }

    void     setEncCABACTableIdx(int idx)           { m_encCABACTableIdx = idx; }

    bool     getCabacInitPresentFlag() const        { return m_cabacInitPresentFlag; }

    UInt     getEncCABACTableIdx() const            { return m_encCABACTableIdx; }

    void     setDeblockingFilterControlPresentFlag(bool val)  { m_deblockingFilterControlPresentFlag = val; }

    bool     getDeblockingFilterControlPresentFlag() const    { return m_deblockingFilterControlPresentFlag; }

    void     setDeblockingFilterOverrideEnabledFlag(bool val) { m_deblockingFilterOverrideEnabledFlag = val; }

    bool     getDeblockingFilterOverrideEnabledFlag() const   { return m_deblockingFilterOverrideEnabledFlag; }

    void     setPicDisableDeblockingFilterFlag(bool val)      { m_picDisableDeblockingFilterFlag = val; }     //!< set offset for deblocking filter disabled

    bool     getPicDisableDeblockingFilterFlag() const        { return m_picDisableDeblockingFilterFlag; }    //!< get offset for deblocking filter disabled

    void     setDeblockingFilterBetaOffsetDiv2(int val)       { m_deblockingFilterBetaOffsetDiv2 = val; }     //!< set beta offset for deblocking filter

    int      getDeblockingFilterBetaOffsetDiv2() const        { return m_deblockingFilterBetaOffsetDiv2; }    //!< get beta offset for deblocking filter

    void     setDeblockingFilterTcOffsetDiv2(int val)         { m_deblockingFilterTcOffsetDiv2 = val; }       //!< set tc offset for deblocking filter

    int      getDeblockingFilterTcOffsetDiv2()                { return m_deblockingFilterTcOffsetDiv2; }      //!< get tc offset for deblocking filter

    bool     getScalingListPresentFlag() const { return m_scalingListPresentFlag; }

    void     setScalingListPresentFlag(bool b) { m_scalingListPresentFlag = b; }

    void     setScalingList(TComScalingList *scalingList);

    const TComScalingList* getScalingList() const { return m_scalingList; } //!< get ScalingList class pointer in PPS

    bool getListsModificationPresentFlag() const { return m_listsModificationPresentFlag; }

    void setListsModificationPresentFlag(bool b) { m_listsModificationPresentFlag = b; }

    UInt getLog2ParallelMergeLevelMinus2() const { return m_log2ParallelMergeLevelMinus2; }

    void setLog2ParallelMergeLevelMinus2(UInt mrgLevel) { m_log2ParallelMergeLevelMinus2 = mrgLevel; }

    int getNumExtraSliceHeaderBits() const { return m_numExtraSliceHeaderBits; }

    void setNumExtraSliceHeaderBits(int i) { m_numExtraSliceHeaderBits = i; }

    bool getSliceHeaderExtensionPresentFlag() const    { return m_sliceHeaderExtensionPresentFlag; }

    void setSliceHeaderExtensionPresentFlag(bool val)  { m_sliceHeaderExtensionPresentFlag = val; }
};

struct WpScalingParam
{
    // Explicit weighted prediction parameters parsed in slice header,
    // or Implicit weighted prediction parameters (8 bits depth values).
    bool        bPresentFlag;
    UInt        log2WeightDenom;
    int         inputWeight;
    int         inputOffset;

    // Weighted prediction scaling values built from above parameters (bitdepth scaled):
    int         w, o, offset, shift, round;
};

typedef WpScalingParam wpScalingParam;

typedef struct
{
    Int64 ac;
    Int64 dc;
} wpACDCParam;

/// slice header class
class TComSlice
{
private:

    //  Bitstream writing
    bool        m_saoEnabledFlag;
    bool        m_saoEnabledFlagChroma; ///< SAO Cb&Cr enabled flag
    int         m_ppsId;                ///< picture parameter set ID
    bool        m_picOutputFlag;        ///< pic_output_flag
    int         m_poc;
    int         m_lastIDR;

    TComReferencePictureSet *m_rps;
    TComReferencePictureSet m_localRPS;
    int         m_bdIdx;
    TComRefPicListModification m_refPicListModification;
    NalUnitType m_nalUnitType;       ///< Nal unit type for the slice
    SliceType   m_sliceType;
    int         m_sliceQp;
    bool        m_dependentSliceSegmentFlag;
    int         m_sliceQpBase;
    bool        m_deblockingFilterDisable;
    bool        m_deblockingFilterOverrideFlag;    //< offsets for deblocking filter inherit from PPS
    int         m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    int         m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    int         m_list1IdxToList0Idx[MAX_NUM_REF];
    int         m_numRefIdx[2];     //  for multiple reference of current slice

    bool        m_bCheckLDC;

    //  Data
    int         m_sliceQpDelta;
    int         m_sliceQpDeltaCb;
    int         m_sliceQpDeltaCr;
    TComPic*    m_refPicList[2][MAX_NUM_REF + 1];
    int         m_refPOCList[2][MAX_NUM_REF + 1];
    bool        m_bIsUsedAsLongTerm[2][MAX_NUM_REF + 1];

    // referenced slice?
    bool        m_bReferenced;

    // access channel
    TComSPS*    m_sps;
    TComPPS*    m_pps;
    TComVPS*    m_vps;
    TComPic*    m_pic;
    UInt        m_colFromL0Flag; // collocated picture from List0 flag

    UInt        m_colRefIdx;
    UInt        m_maxNumMergeCand;

    bool        m_bEqualRef[2][MAX_NUM_REF][MAX_NUM_REF];

    UInt        m_sliceCurEndCUAddr;
    bool        m_nextSlice;
    UInt        m_sliceBits;
    UInt        m_sliceSegmentBits;
    bool        m_bFinalized;

    wpACDCParam     m_weightACDCParam[3];                 // [0:Y, 1:U, 2:V]

    UInt        m_tileOffstForMultES;

    UInt*       m_substreamSizes;
    TComScalingList* m_scalingList; //!< pointer of quantization matrix
    bool        m_cabacInitFlag;

    bool       m_bLMvdL1Zero;
    int        m_numEntryPointOffsets;
    bool       m_temporalLayerNonReferenceFlag;

    bool       m_enableTMVPFlag;

public:

    MotionReference * m_mref[2][MAX_NUM_REF + 1];
    wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]

    TComSlice();
    virtual ~TComSlice();
    void      initSlice();

    void      setVPS(TComVPS* vps)            { m_vps = vps; }

    TComVPS*  getVPS()                        { return m_vps; }

    void      setSPS(TComSPS* sps)            { m_sps = sps; }

    TComSPS*  getSPS()                        { return m_sps; }

    void      setPPS(TComPPS* pps)            { assert(pps != NULL); m_pps = pps; m_ppsId = pps->getPPSId(); }

    TComPPS*  getPPS()                        { return m_pps; }

    void      setPPSId(int ppsid)             { m_ppsId = ppsid; }

    int       getPPSId()                      { return m_ppsId; }

    void      setPicOutputFlag(bool b)        { m_picOutputFlag = b; }

    bool      getPicOutputFlag()              { return m_picOutputFlag; }

    void      setSaoEnabledFlag(bool s)       { m_saoEnabledFlag = s; }

    bool      getSaoEnabledFlag()             { return m_saoEnabledFlag; }

    void      setSaoEnabledFlagChroma(bool s) { m_saoEnabledFlagChroma = s; }   //!< set SAO Cb&Cr enabled flag

    bool      getSaoEnabledFlagChroma()       { return m_saoEnabledFlagChroma; }      //!< get SAO Cb&Cr enabled flag

    void      setRPS(TComReferencePictureSet *rps) { m_rps = rps; }

    TComReferencePictureSet*  getRPS()            { return m_rps; }

    TComReferencePictureSet*  getLocalRPS()       { return &m_localRPS; }

    void      setRPSidx(int bdidx)                { m_bdIdx = bdidx; }

    int       getRPSidx()                         { return m_bdIdx; }

    TComRefPicListModification* getRefPicListModification() { return &m_refPicListModification; }

    void      setLastIDR(int idrPoc)              { m_lastIDR = idrPoc; }

    int       getLastIDR()                        { return m_lastIDR; }

    SliceType getSliceType()                      { return m_sliceType; }

    int       getPOC()                            { return m_poc; }

    int       getSliceQp()                        { return m_sliceQp; }

    bool      getDependentSliceSegmentFlag() const   { return m_dependentSliceSegmentFlag; }

    void      setDependentSliceSegmentFlag(bool val) { m_dependentSliceSegmentFlag = val; }

    int       getSliceQpBase()                    { return m_sliceQpBase; }

    int       getSliceQpDelta()                   { return m_sliceQpDelta; }

    int       getSliceQpDeltaCb()                 { return m_sliceQpDeltaCb; }

    int       getSliceQpDeltaCr()                 { return m_sliceQpDeltaCr; }

    bool      getDeblockingFilterDisable()        { return m_deblockingFilterDisable; }

    bool      getDeblockingFilterOverrideFlag()   { return m_deblockingFilterOverrideFlag; }

    int       getDeblockingFilterBetaOffsetDiv2() { return m_deblockingFilterBetaOffsetDiv2; }

    int       getDeblockingFilterTcOffsetDiv2()   { return m_deblockingFilterTcOffsetDiv2; }

    int       getNumRefIdx(RefPicList e)          { return m_numRefIdx[e]; }

    TComPic*  getPic()                            { return m_pic; }

    TComPic*  getRefPic(RefPicList e, int refIdx) { return m_refPicList[e][refIdx]; }

    int       getRefPOC(RefPicList e, int refIdx) { return m_refPOCList[e][refIdx]; }

    UInt      getColFromL0Flag()                  { return m_colFromL0Flag; }

    UInt      getColRefIdx()                      { return m_colRefIdx; }

    bool      getIsUsedAsLongTerm(int i, int j)   { return m_bIsUsedAsLongTerm[i][j]; }

    bool      getCheckLDC()                       { return m_bCheckLDC; }

    bool      getMvdL1ZeroFlag()                  { return m_bLMvdL1Zero; }

    int       getNumRpsCurrTempList();

    int       getList1IdxToList0Idx(int list1Idx) { return m_list1IdxToList0Idx[list1Idx]; }

    void      setReferenced(bool b)            { m_bReferenced = b; }

    bool      isReferenced()                   { return m_bReferenced; }

    void      setPOC(int i)                    { m_poc = i; }

    void      setNalUnitType(NalUnitType e)    { m_nalUnitType = e; }

    NalUnitType getNalUnitType() const         { return m_nalUnitType; }

    bool      getRapPicFlag();
    bool      getIdrPicFlag()                  { return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
                                                        getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP; }

    bool      isIRAP() const                   { return (getNalUnitType() >= 16) && (getNalUnitType() <= 23); }

    void      checkCRA(TComReferencePictureSet *rps, int& pocCRA, bool& prevRAPisBLA);

    void      setSliceType(SliceType e)               { m_sliceType = e; }

    void      setSliceQp(int i)                       { m_sliceQp = i; }

    void      setSliceQpBase(int i)                   { m_sliceQpBase = i; }

    void      setSliceQpDelta(int i)                  { m_sliceQpDelta = i; }

    void      setSliceQpDeltaCb(int i)                { m_sliceQpDeltaCb = i; }

    void      setSliceQpDeltaCr(int i)                { m_sliceQpDeltaCr = i; }

    void      setDeblockingFilterDisable(bool b)      { m_deblockingFilterDisable = b; }

    void      setDeblockingFilterOverrideFlag(bool b) { m_deblockingFilterOverrideFlag = b; }

    void      setDeblockingFilterBetaOffsetDiv2(int i) { m_deblockingFilterBetaOffsetDiv2 = i; }

    void      setDeblockingFilterTcOffsetDiv2(int i)   { m_deblockingFilterTcOffsetDiv2 = i; }

    void      setRefPic(TComPic* p, RefPicList e, int refIdx) { m_refPicList[e][refIdx] = p; }

    void      setRefPOC(int i, RefPicList e, int refIdx) { m_refPOCList[e][refIdx] = i; }

    void      setNumRefIdx(RefPicList e, int i)   { m_numRefIdx[e] = i; }

    void      setPic(TComPic* p)                  { m_pic = p; }

    void      setRefPicList(TComList<TComPic*>& picList, bool checkNumPocTotalCurr = false);

    void      setRefPOCList();

    void      setColFromL0Flag(UInt colFromL0) { m_colFromL0Flag = colFromL0; }

    void      setColRefIdx(UInt refIdx)     { m_colRefIdx = refIdx; }

    void      setCheckLDC(bool b)           { m_bCheckLDC = b; }

    void      setMvdL1ZeroFlag(bool b)      { m_bLMvdL1Zero = b; }

    bool      isIntra()                     { return m_sliceType == I_SLICE; }

    bool      isInterB()                    { return m_sliceType == B_SLICE; }

    bool      isInterP()                    { return m_sliceType == P_SLICE; }

    void      initEqualRef();

    bool      isEqualRef(RefPicList e, int refIdx1, int refIdx2)
    {
        if (refIdx1 < 0 || refIdx2 < 0) return false;
        return m_bEqualRef[e][refIdx1][refIdx2];
    }

    void setEqualRef(RefPicList e, int refIdx1, int refIdx2, bool b)
    {
        m_bEqualRef[e][refIdx1][refIdx2] = m_bEqualRef[e][refIdx2][refIdx1] = b;
    }

    static void sortPicList(TComList<TComPic*>& picList);

    void setList1IdxToList0Idx();

    void setTLayerInfo(UInt tlayer);
    void decodingMarking(TComList<TComPic*>& picList, int gopSize, int& maxRefPicNum);
    int  checkThatAllRefPicsAreAvailable(TComList<TComPic*>& picList, TComReferencePictureSet *rps, bool printErrors, int pocRandomAccess = 0);
    void createExplicitReferencePictureSetFromReference(TComList<TComPic*>& picList, TComReferencePictureSet *rps, bool isRAP);

    void setMaxNumMergeCand(UInt val)          { m_maxNumMergeCand = val; }

    UInt getMaxNumMergeCand()                  { return m_maxNumMergeCand; }

    void setSliceCurEndCUAddr(UInt uiAddr)     { m_sliceCurEndCUAddr = uiAddr; }

    UInt getSliceCurEndCUAddr()                { return m_sliceCurEndCUAddr; }

    void copySliceInfo(TComSlice *pcSliceSrc);

    void setNextSlice(bool b)                  { m_nextSlice = b; }

    bool isNextSlice()                         { return m_nextSlice; }

    void setSliceBits(UInt val)                { m_sliceBits = val; }

    UInt getSliceBits()                        { return m_sliceBits; }

    void setSliceSegmentBits(UInt val)         { m_sliceSegmentBits = val; }

    UInt getSliceSegmentBits()                 { return m_sliceSegmentBits; }

    void setFinalized(bool val)                { m_bFinalized = val; }

    bool getFinalized()                        { return m_bFinalized; }

    void  setWpScaling(wpScalingParam wp[2][MAX_NUM_REF][3]) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam) * 2 * MAX_NUM_REF * 3); }

    void  getWpScaling(RefPicList e, int refIdx, wpScalingParam *&wp);

    void  resetWpScaling();
    void  initWpScaling();
    inline bool applyWP() { return (m_sliceType == P_SLICE && m_pps->getUseWP()) || (m_sliceType == B_SLICE && m_pps->getWPBiPred()); }

    void  setWpAcDcParam(wpACDCParam wp[3]) { memcpy(m_weightACDCParam, wp, sizeof(wpACDCParam) * 3); }

    void  getWpAcDcParam(wpACDCParam *&wp);
    void  initWpAcDcParam();

    void setTileOffstForMultES(UInt offset)      { m_tileOffstForMultES = offset; }

    UInt getTileOffstForMultES()                 { return m_tileOffstForMultES; }

    void allocSubstreamSizes(UInt uiNumSubstreams);
    UInt* getSubstreamSizes()                    { return m_substreamSizes; }

    void  setScalingList(TComScalingList* scalingList) { m_scalingList = scalingList; }

    TComScalingList*   getScalingList()          { return m_scalingList; }

    void  setDefaultScalingList();
    bool  checkDefaultScalingList();
    void      setCabacInitFlag(bool val)   { m_cabacInitFlag = val; }   //!< set CABAC initial flag

    bool      getCabacInitFlag()           { return m_cabacInitFlag; }  //!< get CABAC initial flag

    void      setNumEntryPointOffsets(int val)  { m_numEntryPointOffsets = val; }

    int       getNumEntryPointOffsets()         { return m_numEntryPointOffsets; }

    bool      getTemporalLayerNonReferenceFlag()       { return m_temporalLayerNonReferenceFlag; }

    void      setTemporalLayerNonReferenceFlag(bool x) { m_temporalLayerNonReferenceFlag = x; }

    void      setEnableTMVPFlag(bool b)    { m_enableTMVPFlag = b; }

    bool      getEnableTMVPFlag()          { return m_enableTMVPFlag; }

protected:

    TComPic*  xGetRefPic(TComList<TComPic*>& picList, int poc);

    TComPic*  xGetLongTermRefPic(TComList<TComPic*>& picList, int poc, bool pocHasMsb);
}; // END CLASS DEFINITION TComSlice

template<class T>
class ParameterSetMap
{
public:

    ParameterSetMap(int maxId)
        : m_maxId(maxId)
    {}

    ~ParameterSetMap()
    {
        for (typename std::map<int, T *>::iterator i = m_paramsetMap.begin(); i != m_paramsetMap.end(); i++)
        {
            delete (*i).second;
        }
    }

    void storePS(int psId, T *ps)
    {
        assert(psId < m_maxId);
        if (m_paramsetMap.find(psId) != m_paramsetMap.end())
        {
            delete m_paramsetMap[psId];
        }
        m_paramsetMap[psId] = ps;
    }

    void mergePSList(ParameterSetMap<T> &rPsList)
    {
        for (typename std::map<int, T *>::iterator i = rPsList.m_paramsetMap.begin(); i != rPsList.m_paramsetMap.end(); i++)
        {
            storePS(i->first, i->second);
        }

        rPsList.m_paramsetMap.clear();
    }

    T* getPS(int psId)
    {
        return (m_paramsetMap.find(psId) == m_paramsetMap.end()) ? NULL : m_paramsetMap[psId];
    }

    T* getFirstPS()
    {
        return (m_paramsetMap.begin() == m_paramsetMap.end()) ? NULL : m_paramsetMap.begin()->second;
    }

private:

    std::map<int, T *> m_paramsetMap;
    int               m_maxId;
};

class ParameterSetManager
{
public:

    ParameterSetManager();
    virtual ~ParameterSetManager();

    //! store sequence parameter set and take ownership of it
    void storeVPS(TComVPS *vps) { m_vpsMap.storePS(vps->getVPSId(), vps); }

    //! get pointer to existing video parameter set
    TComVPS* getVPS(int vpsId)  { return m_vpsMap.getPS(vpsId); }

    TComVPS* getFirstVPS()      { return m_vpsMap.getFirstPS(); }

    //! store sequence parameter set and take ownership of it
    void storeSPS(TComSPS *sps) { m_spsMap.storePS(sps->getSPSId(), sps); }

    //! get pointer to existing sequence parameter set
    TComSPS* getSPS(int spsId)  { return m_spsMap.getPS(spsId); }

    TComSPS* getFirstSPS()      { return m_spsMap.getFirstPS(); }

    //! store picture parameter set and take ownership of it
    void storePPS(TComPPS *pps) { m_ppsMap.storePS(pps->getPPSId(), pps); }

    //! get pointer to existing picture parameter set
    TComPPS* getPPS(int ppsId)  { return m_ppsMap.getPS(ppsId); }

    TComPPS* getFirstPPS()      { return m_ppsMap.getFirstPS(); }

    //! activate a SPS from a active parameter sets SEI message
    //! \returns true, if activation is successful
    bool activateSPSWithSEI(int SPSId);

    //! activate a PPS and depending on isIDR parameter also SPS and VPS
    //! \returns true, if activation is successful
    bool activatePPS(int ppsId, bool isIRAP);

    TComVPS* getActiveVPS() { return m_vpsMap.getPS(m_activeVPSId); }

    TComSPS* getActiveSPS() { return m_spsMap.getPS(m_activeSPSId); }

    TComPPS* getActivePPS() { return m_ppsMap.getPS(m_activePPSId); }

protected:

    ParameterSetMap<TComVPS> m_vpsMap;
    ParameterSetMap<TComSPS> m_spsMap;
    ParameterSetMap<TComPPS> m_ppsMap;

    int m_activeVPSId;
    int m_activeSPSId;
    int m_activePPSId;
};
}
//! \}

#endif // ifndef X265_TCOMSLICE_H
