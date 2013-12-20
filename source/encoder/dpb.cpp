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

#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComSlice.h"
#include "TLibEncoder/TEncCfg.h"

#include "PPA/ppa.h"
#include "dpb.h"
#include "frameencoder.h"

using namespace x265;

DPB::~DPB()
{
    while (!m_picList.empty())
    {
        TComPic* pic = m_picList.popFront();
        pic->destroy(m_cfg->param.bframes);
        delete pic;
    }
}

// move unreferenced pictures from picList to freeList for recycle
void DPB::recycleUnreferenced(TComList<TComPic*>& freeList)
{
    TComList<TComPic*>::iterator iterPic = m_picList.begin();
    while (iterPic != m_picList.end())
    {
        TComPic *pic = *(iterPic++);
        if (pic->getSlice()->isReferenced() == false && pic->m_countRefEncoders == 0)
        {
            pic->getPicYuvRec()->clearReferences();
            pic->m_reconRowCount = 0;

            // iterator is invalidated by remove, restart scan
            m_picList.remove(pic);
            iterPic = m_picList.begin();

            freeList.pushBack(pic);
        }
    }
}

void DPB::prepareEncode(TComPic *pic)
{
    PPAScopeEvent(DPB_prepareEncode);

    int pocCurr = pic->getSlice()->getPOC();

    m_picList.pushFront(pic);

    TComSlice* slice = pic->getSlice();
    if (getNalUnitType(pocCurr, m_lastIDR, pic) == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
        getNalUnitType(pocCurr, m_lastIDR, pic) == NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
        m_lastIDR = pocCurr;
    }
    slice->setLastIDR(m_lastIDR);
    slice->setReferenced(slice->getSliceType() != B_SLICE);
    slice->setTemporalLayerNonReferenceFlag(!slice->isReferenced());
    // Set the nal unit type
    slice->setNalUnitType(getNalUnitType(pocCurr, m_lastIDR, pic));

    // If the slice is un-referenced, change from _R "referenced" to _N "non-referenced" NAL unit type
    if (slice->getTemporalLayerNonReferenceFlag())
    {
        switch (slice->getNalUnitType())
        {
        case NAL_UNIT_CODED_SLICE_TRAIL_R:
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_TRAIL_N);
            break;
        case NAL_UNIT_CODED_SLICE_RADL_R:
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_RADL_N);
            break;
        case NAL_UNIT_CODED_SLICE_RASL_R:
            slice->setNalUnitType(NAL_UNIT_CODED_SLICE_RASL_N);
            break;
        default:
            break;
        }
    }

    // Do decoding refresh marking if any
    decodingRefreshMarking(pocCurr, slice->getNalUnitType());

    computeRPS(pocCurr, slice->isIRAP(), slice->getLocalRPS(), slice->getSPS()->getMaxDecPicBuffering(0));
    slice->setRPS(slice->getLocalRPS());
    slice->setRPSidx(-1); // Force use of RPS from slice, rather than from SPS

    applyReferencePictureSet(slice->getRPS(), pocCurr); // Mark pictures in m_piclist as unreferenced if they are not included in RPS

    arrangeLongtermPicturesInRPS(slice);
    TComRefPicListModification* refPicListModification = slice->getRefPicListModification();
    refPicListModification->setRefPicListModificationFlagL0(false);
    refPicListModification->setRefPicListModificationFlagL1(false);
    slice->setNumRefIdx(REF_PIC_LIST_0, X265_MIN(m_maxRefL0, slice->getRPS()->getNumberOfNegativePictures())); // Ensuring L0 contains just the -ve POC
    slice->setNumRefIdx(REF_PIC_LIST_1, X265_MIN(m_maxRefL1, slice->getRPS()->getNumberOfPositivePictures()));

    slice->setRefPicList(m_picList);

    // Slice type refinement
    if ((slice->getSliceType() == B_SLICE) && (slice->getNumRefIdx(REF_PIC_LIST_1) == 0))
    {
        slice->setSliceType(P_SLICE);
    }

    if (slice->getSliceType() == B_SLICE)
    {
        // TODO: Can we estimate this from lookahead?
        slice->setColFromL0Flag(0);

        bool bLowDelay = true;
        int curPOC = slice->getPOC();
        int refIdx = 0;

        for (refIdx = 0; refIdx < slice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; refIdx++)
        {
            if (slice->getRefPic(REF_PIC_LIST_0, refIdx)->getPOC() > curPOC)
            {
                bLowDelay = false;
            }
        }

        for (refIdx = 0; refIdx < slice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; refIdx++)
        {
            if (slice->getRefPic(REF_PIC_LIST_1, refIdx)->getPOC() > curPOC)
            {
                bLowDelay = false;
            }
        }

        slice->setCheckLDC(bLowDelay);
    }
    else
    {
        slice->setCheckLDC(true);
    }

    slice->setRefPOCList();
    slice->setList1IdxToList0Idx();
    slice->setEnableTMVPFlag(1);

    bool bGPBcheck = false;
    if (slice->getSliceType() == B_SLICE)
    {
        if (slice->getNumRefIdx(REF_PIC_LIST_0) == slice->getNumRefIdx(REF_PIC_LIST_1))
        {
            bGPBcheck = true;
            for (int i = 0; i < slice->getNumRefIdx(REF_PIC_LIST_1); i++)
            {
                if (slice->getRefPOC(REF_PIC_LIST_1, i) != slice->getRefPOC(REF_PIC_LIST_0, i))
                {
                    bGPBcheck = false;
                    break;
                }
            }
        }
    }

    slice->setMvdL1ZeroFlag(bGPBcheck);
    slice->setNextSlice(false);

    /* Increment reference count of all motion-referenced frames.  This serves two purposes. First
     * it prevents the frame from being recycled, and second the referenced frames know how many
     * other FrameEncoders are using them for motion reference */
    int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
    for (int l = 0; l < numPredDir; l++)
    {
        RefPicList list = (l ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
        for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
        {
            TComPic *refpic = slice->getRefPic(list, ref);
            ATOMIC_INC(&refpic->m_countRefEncoders);
        }
    }
}

void DPB::computeRPS(int curPoc, bool isRAP, TComReferencePictureSet * rps, unsigned int maxDecPicBuffer)
{
    TComPic * refPic;
    unsigned int poci = 0, numNeg = 0, numPos = 0;

    TComList<TComPic*>::iterator iterPic = m_picList.begin();
    while ((iterPic != m_picList.end()) && (poci < maxDecPicBuffer - 1))
    {
        refPic = *(iterPic);
        if ((refPic->getPOC() != curPoc) && (refPic->getSlice()->isReferenced()))
        {
            rps->m_POC[poci] = refPic->getPOC();
            rps->m_deltaPOC[poci] = rps->m_POC[poci] - curPoc;
            (rps->m_deltaPOC[poci] < 0) ? numNeg++ : numPos++;
            rps->m_used[poci] = !isRAP;
            poci++;
        }
        iterPic++;
    }

    rps->m_numberOfPictures = poci;
    rps->m_numberOfPositivePictures = numPos;
    rps->m_numberOfNegativePictures = numNeg;
    rps->m_numberOfLongtermPictures = 0;
    rps->m_interRPSPrediction = false;          // To be changed later when needed

    rps->sortDeltaPOC();
}

/** Function for marking the reference pictures when an IDR/CRA/CRANT/BLA/BLANT is encountered.
 * \param pocCRA POC of the CRA/CRANT/BLA/BLANT picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param picList reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR/BLA/BLANT, all pictures in the reference picture list
 * are marked as "unused for reference"
 *    If the nal_unit_type is BLA/BLANT, set the pocCRA to the temporal reference of the current picture.
 * Otherwise
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current
 *    temporal reference is greater than the temporal reference of the latest CRA/CRANT/BLA/BLANT picture (pocCRA),
 *    mark all reference pictures except the latest CRA/CRANT/BLA/BLANT picture as "unused for reference" and set
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA/CRANT, set the bRefreshPending flag to true and pocCRA to the temporal
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
void DPB::decodingRefreshMarking(int pocCurr, NalUnitType nalUnitType)
{
    TComPic* outPic;

    if (nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
        || nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
        || nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP) // IDR or BLA picture
    {
        // mark all pictures as not used for reference
        TComList<TComPic*>::iterator iterPic = m_picList.begin();
        while (iterPic != m_picList.end())
        {
            outPic = *(iterPic);
            if (outPic->getPOC() != pocCurr)
                outPic->getSlice()->setReferenced(false);
            iterPic++;
        }

        if (nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP
            || nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP)
        {
            m_pocCRA = pocCurr;
        }
    }
    else // CRA or No DR
    {
        if (m_bRefreshPending == true && pocCurr > m_pocCRA) // CRA reference marking pending
        {
            TComList<TComPic*>::iterator iterPic = m_picList.begin();
            while (iterPic != m_picList.end())
            {
                outPic = *(iterPic);
                if (outPic->getPOC() != pocCurr && outPic->getPOC() != m_pocCRA)
                    outPic->getSlice()->setReferenced(false);
                iterPic++;
            }

            m_bRefreshPending = false;
        }
        if (nalUnitType == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
        {
            m_bRefreshPending = true;
            m_pocCRA = pocCurr;
        }
    }
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet */
void DPB::applyReferencePictureSet(TComReferencePictureSet *rps, int curPoc)
{
    TComPic* outPic;
    int i, isReference;

    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = m_picList.begin();
    while (iterPic != m_picList.end())
    {
        outPic = *(iterPic++);

        if (!outPic->getSlice()->isReferenced())
        {
            continue;
        }

        isReference = 0;
        // loop through all pictures in the Reference Picture Set
        // to see if the picture should be kept as reference picture
        for (i = 0; i < rps->getNumberOfPositivePictures() + rps->getNumberOfNegativePictures(); i++)
        {
            if (!outPic->getIsLongTerm() && outPic->getPicSym()->getSlice()->getPOC() == curPoc + rps->getDeltaPOC(i))
            {
                isReference = 1;
                outPic->setUsedByCurr(rps->getUsed(i) == 1);
                outPic->setIsLongTerm(0);
            }
        }

        for (; i < rps->getNumberOfPictures(); i++)
        {
            if (rps->getCheckLTMSBPresent(i) == true)
            {
                if (outPic->getIsLongTerm() && (outPic->getPicSym()->getSlice()->getPOC()) == rps->getPOC(i))
                {
                    isReference = 1;
                    outPic->setUsedByCurr(rps->getUsed(i) == 1);
                }
            }
            else
            {
                if (outPic->getIsLongTerm() && (outPic->getPicSym()->getSlice()->getPOC() %
                                                (1 << outPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC())) == rps->getPOC(i) %
                    (1 << outPic->getPicSym()->getSlice()->getSPS()->getBitsForPOC()))
                {
                    isReference = 1;
                    outPic->setUsedByCurr(rps->getUsed(i) == 1);
                }
            }
        }

        // mark the picture as "unused for reference" if it is not in
        // the Reference Picture Set
        if (outPic->getPicSym()->getSlice()->getPOC() != curPoc && isReference == 0)
        {
            outPic->getSlice()->setReferenced(false);
            outPic->setUsedByCurr(0);
            outPic->setIsLongTerm(0);
        }
        // TODO: Do we require this check here
        // check that pictures of higher or equal temporal layer are not in the RPS if the current picture is a TSA picture

        /*if (this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TLA_R || this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N)
        {
            assert(outPic->getSlice()->isReferenced() == 0);
        }*/
    }
}

/** Function for deciding the nal_unit_type.
 * \param pocCurr POC of the current picture
 * \returns the nal unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType DPB::getNalUnitType(int curPOC, int lastIDR, TComPic* pic)
{
    if (curPOC == 0)
    {
        return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
    }
    if (pic->m_lowres.bKeyframe)
    {
        if (m_cfg->param.decodingRefreshType == 1)
        {
            return NAL_UNIT_CODED_SLICE_CRA;
        }
        else if (m_cfg->param.decodingRefreshType == 2)
        {
            return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
        }
    }
    if (m_pocCRA > 0)
    {
        if (curPOC < m_pocCRA)
        {
            // All leading pictures are being marked as TFD pictures here since current encoder uses all
            // reference pictures while encoding leading pictures. An encoder can ensure that a leading
            // picture can be still decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by
            // controlling the reference pictures used for encoding that leading picture. Such a leading
            // picture need not be marked as a TFD picture.
            return NAL_UNIT_CODED_SLICE_RASL_R;
        }
    }
    if (lastIDR > 0)
    {
        if (curPOC < lastIDR)
        {
            return NAL_UNIT_CODED_SLICE_RADL_R;
        }
    }
    return NAL_UNIT_CODED_SLICE_TRAIL_R;
}

static inline int getLSB(int poc, int maxLSB)
{
    if (poc >= 0)
    {
        return poc % maxLSB;
    }
    else
    {
        return (maxLSB - ((-poc) % maxLSB)) % maxLSB;
    }
}

// Function will arrange the long-term pictures in the decreasing order of poc_lsb_lt,
// and among the pictures with the same lsb, it arranges them in increasing delta_poc_msb_cycle_lt value
void DPB::arrangeLongtermPicturesInRPS(TComSlice *slice)
{
    TComReferencePictureSet *rps = slice->getRPS();

    if (!rps->getNumberOfLongtermPictures())
    {
        return;
    }

    // Arrange long-term reference pictures in the correct order of LSB and MSB,
    // and assign values for pocLSBLT and MSB present flag
    int longtermPicsPoc[MAX_NUM_REF_PICS], longtermPicsLSB[MAX_NUM_REF_PICS], indices[MAX_NUM_REF_PICS];
    int longtermPicsMSB[MAX_NUM_REF_PICS];
    bool mSBPresentFlag[MAX_NUM_REF_PICS];
    ::memset(longtermPicsPoc, 0, sizeof(longtermPicsPoc));  // Store POC values of LTRP
    ::memset(longtermPicsLSB, 0, sizeof(longtermPicsLSB));  // Store POC LSB values of LTRP
    ::memset(longtermPicsMSB, 0, sizeof(longtermPicsMSB));  // Store POC LSB values of LTRP
    ::memset(indices, 0, sizeof(indices));                  // Indices to aid in tracking sorted LTRPs
    ::memset(mSBPresentFlag, 0, sizeof(mSBPresentFlag));    // Indicate if MSB needs to be present

    // Get the long-term reference pictures
    int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
    int i, ctr = 0;
    int maxPicOrderCntLSB = 1 << slice->getSPS()->getBitsForPOC();
    for (i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
    {
        longtermPicsPoc[ctr] = rps->getPOC(i);                                  // LTRP POC
        longtermPicsLSB[ctr] = getLSB(longtermPicsPoc[ctr], maxPicOrderCntLSB); // LTRP POC LSB
        indices[ctr] = i;
        longtermPicsMSB[ctr] = longtermPicsPoc[ctr] - longtermPicsLSB[ctr];
    }

    int numLongPics = rps->getNumberOfLongtermPictures();
    assert(ctr == numLongPics);

    // Arrange pictures in decreasing order of MSB;
    for (i = 0; i < numLongPics; i++)
    {
        for (int j = 0; j < numLongPics - 1; j++)
        {
            if (longtermPicsMSB[j] < longtermPicsMSB[j + 1])
            {
                std::swap(longtermPicsPoc[j], longtermPicsPoc[j + 1]);
                std::swap(longtermPicsLSB[j], longtermPicsLSB[j + 1]);
                std::swap(longtermPicsMSB[j], longtermPicsMSB[j + 1]);
                std::swap(indices[j], indices[j + 1]);
            }
        }
    }

    for (i = 0; i < numLongPics; i++)
    {
        // Check if MSB present flag should be enabled.
        // Check if the buffer contains any pictures that have the same LSB.
        TComList<TComPic*>::iterator iterPic = m_picList.begin();
        TComPic* pic;
        while (iterPic != m_picList.end())
        {
            pic = *iterPic;
            if ((getLSB(pic->getPOC(), maxPicOrderCntLSB) == longtermPicsLSB[i])   && // Same LSB
                (pic->getSlice()->isReferenced()) &&                                  // Reference picture
                (pic->getPOC() != longtermPicsPoc[i]))                                // Not the LTRP itself
            {
                mSBPresentFlag[i] = true;
                break;
            }
            iterPic++;
        }
    }

    // tempArray for usedByCurr flag
    bool tempArray[MAX_NUM_REF_PICS];
    ::memset(tempArray, 0, sizeof(tempArray));
    for (i = 0; i < numLongPics; i++)
    {
        tempArray[i] = rps->getUsed(indices[i]) ? true : false;
    }

    // Now write the final values;
    ctr = 0;
    int currMSB = 0, currLSB = 0;
    // currPicPoc = currMSB + currLSB
    currLSB = getLSB(slice->getPOC(), maxPicOrderCntLSB);
    currMSB = slice->getPOC() - currLSB;

    for (i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
    {
        rps->setPOC(i, longtermPicsPoc[ctr]);
        rps->setDeltaPOC(i, -slice->getPOC() + longtermPicsPoc[ctr]);
        rps->setUsed(i, tempArray[ctr]);
        rps->setPocLSBLT(i, longtermPicsLSB[ctr]);
        rps->setDeltaPocMSBCycleLT(i, (currMSB - (longtermPicsPoc[ctr] - longtermPicsLSB[ctr])) / maxPicOrderCntLSB);
        rps->setDeltaPocMSBPresentFlag(i, mSBPresentFlag[ctr]);

        assert(rps->getDeltaPocMSBCycleLT(i) >= 0); // Non-negative value
    }

    for (i = rps->getNumberOfPictures() - 1, ctr = 1; i >= offset; i--, ctr++)
    {
        for (int j = rps->getNumberOfPictures() - 1 - ctr; j >= offset; j--)
        {
            // Here at the encoder we know that we have set the full POC value for the LTRPs, hence we
            // don't have to check the MSB present flag values for this constraint.
            assert(rps->getPOC(i) != rps->getPOC(j)); // If assert fails, LTRP entry repeated in RPS!!!
        }
    }
}
