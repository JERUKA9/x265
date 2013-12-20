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

/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include "common.h"

#include "TEncTop.h"
#include "NALwrite.h"

#include "slicetype.h"
#include "frameencoder.h"
#include "ratecontrol.h"
#include "dpb.h"

#include <cstdlib>
#include <math.h> // log10

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
    m_pocLast = -1;
    m_maxRefPicNum = 0;
    m_curEncoder = 0;
    m_lookahead = NULL;
    m_frameEncoder = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;

#if ENC_DEC_TRACE
    g_hTrace = fopen("TraceEnc.txt", "wb");
    g_bJustDoIt = g_bEncDecTraceDisable;
    g_nSymbolCounter = 0;
#endif
}

TEncTop::~TEncTop()
{
#if ENC_DEC_TRACE
    fclose(g_hTrace);
#endif
}

void TEncTop::create()
{
    if (!primitives.sad[0])
    {
        // this should be an impossible condition when using our public API, and indicates a serious bug.
        x265_log(&param, X265_LOG_ERROR, "Primitives must be initialized before encoder is created\n");
        abort();
    }

    m_frameEncoder = new FrameEncoder[param.frameNumThreads];
    if (m_frameEncoder)
    {
        for (int i = 0; i < param.frameNumThreads; i++)
        {
            m_frameEncoder[i].setThreadPool(m_threadPool);
        }
    }
    m_lookahead = new Lookahead(this);
    m_dpb = new DPB(this);
    m_rateControl = new RateControl(&param);
}

void TEncTop::destroy()
{
    if (m_frameEncoder)
    {
        for (int i = 0; i < param.frameNumThreads; i++)
        {
            // Ensure frame encoder is idle before destroying it
            m_frameEncoder[i].getEncodedPicture(NULL);
            m_frameEncoder[i].destroy();
        }

        delete [] m_frameEncoder;
    }

    while (!m_freeList.empty())
    {
        TComPic* pic = m_freeList.popFront();
        pic->destroy(param.bframes);
        delete pic;
    }

    if (m_lookahead)
    {
        m_lookahead->destroy();
        delete m_lookahead;
    }

    delete m_dpb;
    delete m_rateControl;

    // thread pool release should always happen last
    if (m_threadPool)
        m_threadPool->release();
}

void TEncTop::init()
{
    if (m_frameEncoder)
    {
        int numRows = (param.sourceHeight + g_maxCUHeight - 1) / g_maxCUHeight;
        for (int i = 0; i < param.frameNumThreads; i++)
        {
            m_frameEncoder[i].init(this, numRows);
        }
    }
}

int TEncTop::getStreamHeaders(NALUnitEBSP **nalunits)
{
    return m_frameEncoder->getStreamHeaders(nalunits);
}

/**
 \param   flush               force encoder to encode a frame
 \param   pic_in              input original YUV picture or NULL
 \param   pic_out             pointer to reconstructed picture struct
 \param   nalunits            output NAL packets
 \retval                      number of encoded pictures
 */
int TEncTop::encode(bool flush, const x265_picture_t* pic_in, x265_picture_t *pic_out, NALUnitEBSP **nalunits)
{
    if (pic_in)
    {
        TComPic *pic;
        if (m_freeList.empty())
        {
            pic = new TComPic;
            pic->create(param.sourceWidth, param.sourceHeight, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth,
                        getConformanceWindow(), getDefaultDisplayWindow(), param.bframes);
            if (param.bEnableSAO)
            {
                // TODO: these should be allocated on demand within the encoder
                // NOTE: the SAO pointer from m_frameEncoder for read m_maxSplitLevel, etc, we can remove it later
                pic->getPicSym()->allocSaoParam(m_frameEncoder->getSAO());
            }
        }
        else
            pic = m_freeList.popBack();

        /* Copy input picture into a TComPic, send to lookahead */
        pic->getSlice()->setPOC(++m_pocLast);
        pic->getPicYuvOrg()->copyFromPicture(*pic_in);
        pic->m_userData = pic_in->userData;

        // TEncTop holds a reference count until collecting stats
        ATOMIC_INC(&pic->m_countRefEncoders);
        m_lookahead->addPicture(pic, pic_in->sliceType);
    }

    if (flush)
        m_lookahead->flush();

    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % param.frameNumThreads;
    int ret = 0;

    // getEncodedPicture() should block until the FrameEncoder has completed
    // encoding the frame.  This is how back-pressure through the API is
    // accomplished when the encoder is full.
    TComPic *out = curEncoder->getEncodedPicture(nalunits);

    if (!out && flush)
    {
        // if the current encoder did not return an output picture and we are
        // flushing, check all the other encoders in logical order until
        // we find an output picture or have cycled around.  We cannot return
        // 0 until the entire stream is flushed
        // (can only be an issue when --frames < --frame-threads)
        int flushed = m_curEncoder;
        do
        {
            curEncoder = &m_frameEncoder[m_curEncoder];
            m_curEncoder = (m_curEncoder + 1) % param.frameNumThreads;
            out = curEncoder->getEncodedPicture(nalunits);
        }
        while (!out && flushed != m_curEncoder);
    }
    if (out)
    {
        if (pic_out)
        {
            TComPicYuv *recpic = out->getPicYuvRec();
            pic_out->poc = out->getSlice()->getPOC();
            pic_out->bitDepth = sizeof(Pel) * 8;
            pic_out->userData = out->m_userData;
            switch (out->getSlice()->getSliceType())
            {
            case I_SLICE:
                pic_out->sliceType = X265_TYPE_I;
                break;
            case P_SLICE:
                pic_out->sliceType = X265_TYPE_P;
                break;
            case B_SLICE:
                pic_out->sliceType = X265_TYPE_B;
                break;
            }
            pic_out->planes[0] = recpic->getLumaAddr();
            pic_out->stride[0] = recpic->getStride();
            pic_out->planes[1] = recpic->getCbAddr();
            pic_out->stride[1] = recpic->getCStride();
            pic_out->planes[2] = recpic->getCrAddr();
            pic_out->stride[2] = recpic->getCStride();
        }

        double bits = calculateHashAndPSNR(out, nalunits);
        // Allow this frame to be recycled if no frame encoders are using it for reference
        ATOMIC_DEC(&out->m_countRefEncoders);

        m_rateControl->rateControlEnd(bits, &(curEncoder->m_rce));

        m_dpb->recycleUnreferenced(m_freeList);

        ret = 1;
    }

    if (!m_lookahead->outputQueue.empty())
    {
        // pop a single frame from decided list, then provide to frame encoder
        // curEncoder is guaranteed to be idle at this point
        TComPic *fenc = m_lookahead->outputQueue.popFront();

        // Initialize slice for encoding with this FrameEncoder
        curEncoder->initSlice(fenc);

        // determine references, setup RPS, etc
        m_dpb->prepareEncode(fenc);

        // set slice QP
        m_rateControl->rateControlStart(fenc, m_lookahead, &(curEncoder->m_rce));

        // Allow FrameEncoder::compressFrame() to start in a worker thread
        curEncoder->m_enable.trigger();
    }

    return ret;
}

double TEncTop::printSummary()
{
    double fps = (double)param.frameRate;
    if (param.logLevel >= X265_LOG_INFO)
    {
        m_analyzeI.printOut('i', fps);
        m_analyzeP.printOut('p', fps);
        m_analyzeB.printOut('b', fps);
        m_analyzeAll.printOut('a', fps);
    }

#if _SUMMARY_OUT_
    m_analyzeAll.printSummaryOut(fps);
#endif
#if _SUMMARY_PIC_
    m_analyzeI.printSummary('I', fps);
    m_analyzeP.printSummary('P', fps);
    m_analyzeB.printSummary('B', fps);
#endif

    if (m_analyzeAll.getNumPic())
        return (m_analyzeAll.getPsnrY() * 6 + m_analyzeAll.getPsnrU() + m_analyzeAll.getPsnrV()) / (8 * m_analyzeAll.getNumPic());
    else
        return 100.0;
}

#define VERBOSE_RATE 0
#if VERBOSE_RATE
static const char* nalUnitTypeToString(NalUnitType type)
{
    switch (type)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:    return "TRAIL_R";
    case NAL_UNIT_CODED_SLICE_TRAIL_N:    return "TRAIL_N";
    case NAL_UNIT_CODED_SLICE_TLA_R:      return "TLA_R";
    case NAL_UNIT_CODED_SLICE_TSA_N:      return "TSA_N";
    case NAL_UNIT_CODED_SLICE_STSA_R:     return "STSA_R";
    case NAL_UNIT_CODED_SLICE_STSA_N:     return "STSA_N";
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:   return "BLA_W_LP";
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL: return "BLA_W_RADL";
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:   return "BLA_N_LP";
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL: return "IDR_W_RADL";
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:   return "IDR_N_LP";
    case NAL_UNIT_CODED_SLICE_CRA:        return "CRA";
    case NAL_UNIT_CODED_SLICE_RADL_R:     return "RADL_R";
    case NAL_UNIT_CODED_SLICE_RASL_R:     return "RASL_R";
    case NAL_UNIT_VPS:                    return "VPS";
    case NAL_UNIT_SPS:                    return "SPS";
    case NAL_UNIT_PPS:                    return "PPS";
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:  return "AUD";
    case NAL_UNIT_EOS:                    return "EOS";
    case NAL_UNIT_EOB:                    return "EOB";
    case NAL_UNIT_FILLER_DATA:            return "FILLER";
    case NAL_UNIT_PREFIX_SEI:             return "SEI";
    case NAL_UNIT_SUFFIX_SEI:             return "SEI";
    default:                              return "UNK";
    }
}

#endif // if VERBOSE_RATE

// TODO:
//   1 - as a performance optimization, if we're not reporting PSNR we do not have to measure PSNR
//       (we do not yet have a switch to disable PSNR reporting)
//   2 - it would be better to accumulate SSD of each CTU at the end of processCTU() while it is cache-hot
//       in fact, we almost certainly are already measuring the CTU distortion and not accumulating it
static UInt64 computeSSD(Pel *fenc, Pel *rec, int stride, int width, int height)
{
    UInt64 ssd = 0;

    if ((width | height) & 3)
    {
        /* Slow Path */
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int diff = (int)(fenc[x] - rec[x]);
                ssd += diff * diff;
            }

            fenc += stride;
            rec += stride;
        }

        return ssd;
    }

    int y = 0;
    /* Consume Y in chunks of 64 */
    for (; y + 64 <= height; y += 64)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x64](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x64](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x64](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 64;
        rec += stride * 64;
    }

    /* Consume Y in chunks of 16 */
    for (; y + 16 <= height; y += 16)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x16](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x16](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x16](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 16;
        rec += stride * 16;
    }

    /* Consume Y in chunks of 4 */
    for (; y + 4 <= height; y += 4)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x4](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x4](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x4](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 4;
        rec += stride * 4;
    }

    return ssd;
}

/**
 * Produce an ascii(hex) representation of picture digest.
 *
 * Returns: a statically allocated null-terminated string.  DO NOT FREE.
 */
static const char*digestToString(const unsigned char digest[3][16], int numChar)
{
    const char* hex = "0123456789abcdef";
    static char string[99];
    int cnt = 0;

    for (int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
    {
        for (int i = 0; i < numChar; i++)
        {
            string[cnt++] = hex[digest[yuvIdx][i] >> 4];
            string[cnt++] = hex[digest[yuvIdx][i] & 0xf];
        }

        string[cnt++] = ',';
    }

    string[cnt - 1] = '\0';
    return string;
}

/* Returns Number of bits in current encoded pic */

double TEncTop::calculateHashAndPSNR(TComPic* pic, NALUnitEBSP **nalunits)
{
    TComPicYuv* recon = pic->getPicYuvRec();
    TComPicYuv* orig  = pic->getPicYuvOrg();

    //===== calculate PSNR =====
    int stride = recon->getStride();
    int width  = recon->getWidth() - getPad(0);
    int height = recon->getHeight() - getPad(1);
    int size = width * height;

    UInt64 ssdY = computeSSD(orig->getLumaAddr(), recon->getLumaAddr(), stride, width, height);

    height >>= 1;
    width  >>= 1;
    stride = recon->getCStride();

    UInt64 ssdU = computeSSD(orig->getCbAddr(), recon->getCbAddr(), stride, width, height);
    UInt64 ssdV = computeSSD(orig->getCrAddr(), recon->getCrAddr(), stride, width, height);

    int maxvalY = 255 << (X265_DEPTH - 8);
    int maxvalC = 255 << (X265_DEPTH - 8);
    double refValueY = (double)maxvalY * maxvalY * size;
    double refValueC = (double)maxvalC * maxvalC * size / 4.0;
    double psnrY = (ssdY ? 10.0 * log10(refValueY / (double)ssdY) : 99.99);
    double psnrU = (ssdU ? 10.0 * log10(refValueC / (double)ssdU) : 99.99);
    double psnrV = (ssdV ? 10.0 * log10(refValueC / (double)ssdV) : 99.99);

    const char* digestStr = NULL;
    if (param.decodedPictureHashSEI)
    {
        SEIDecodedPictureHash sei_recon_picture_digest;
        if (param.decodedPictureHashSEI == 1)
        {
            /* calculate MD5sum for entire reconstructed picture */
            sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
            calcMD5(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if (param.decodedPictureHashSEI == 2)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
            calcCRC(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if (param.decodedPictureHashSEI == 3)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
            calcChecksum(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }

        /* write the SEI messages */
        OutputNALUnit onalu(NAL_UNIT_SUFFIX_SEI, 0);
        m_frameEncoder->m_seiWriter.writeSEImessage(onalu.m_Bitstream, sei_recon_picture_digest, pic->getSlice()->getSPS());
        writeRBSPTrailingBits(onalu.m_Bitstream);

        int count = 0;
        while(nalunits[count] != NULL)
            count++;
        nalunits[count] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
        if (nalunits[count])
            nalunits[count]->init(onalu);
        else
            digestStr = NULL;
    }

    /* calculate the size of the access unit, excluding:
     *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
     *  - SEI NAL units
     */
    UInt numRBSPBytes = 0;
    for (int count = 0; nalunits[count] != NULL; count++)
    {
        UInt numRBSPBytes_nal = nalunits[count]->m_packetSize;
#if VERBOSE_RATE
        printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
        if (nalunits[count]->m_nalUnitType != NAL_UNIT_PREFIX_SEI && nalunits[count]->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
        {
            numRBSPBytes += numRBSPBytes_nal;
        }
    }

    UInt bits = numRBSPBytes * 8;

    //===== add PSNR =====
    m_analyzeAll.addResult(psnrY, psnrU, psnrV, (double)bits);
    TComSlice*  slice = pic->getSlice();
    if (slice->isIntra())
    {
        m_analyzeI.addResult(psnrY, psnrU, psnrV, (double)bits);
    }
    if (slice->isInterP())
    {
        m_analyzeP.addResult(psnrY, psnrU, psnrV, (double)bits);
    }
    if (slice->isInterB())
    {
        m_analyzeB.addResult(psnrY, psnrU, psnrV, (double)bits);
    }

    if (param.logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');

        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced

        fprintf(stderr, "\rPOC %4d ( %c-SLICE, nQP %d QP %d) %10d bits",
                slice->getPOC(),
                c,
                slice->getSliceQpBase(),
                slice->getSliceQp(),
                bits);

        fprintf(stderr, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);

        if (!slice->isIntra())
        {
            int numLists = slice->isInterP() ? 1 : 2;
            for (int list = 0; list < numLists; list++)
            {
                fprintf(stderr, " [L%d ", list);
                for (int ref = 0; ref < slice->getNumRefIdx(RefPicList(list)); ref++)
                {
                    fprintf(stderr, "%d ", slice->getRefPOC(RefPicList(list), ref) - slice->getLastIDR());
                }

                fprintf(stderr, "]");
            }
        }
        if (digestStr && param.logLevel >= 4)
        {
            if (param.decodedPictureHashSEI == 1)
            {
                fprintf(stderr, " [MD5:%s]", digestStr);
            }
            else if (param.decodedPictureHashSEI == 2)
            {
                fprintf(stderr, " [CRC:%s]", digestStr);
            }
            else if (param.decodedPictureHashSEI == 3)
            {
                fprintf(stderr, " [Checksum:%s]", digestStr);
            }
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    return bits;
}

void TEncTop::xInitSPS(TComSPS *sps)
{
    ProfileTierLevel& profileTierLevel = *sps->getPTL()->getGeneralPTL();

    profileTierLevel.setLevelIdc(m_level);
    profileTierLevel.setTierFlag(m_levelTier);
    profileTierLevel.setProfileIdc(m_profile);
    profileTierLevel.setProfileCompatibilityFlag(m_profile, 1);
    profileTierLevel.setProgressiveSourceFlag(m_progressiveSourceFlag);
    profileTierLevel.setInterlacedSourceFlag(m_interlacedSourceFlag);
    profileTierLevel.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
    profileTierLevel.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);

    if (m_profile == Profile::MAIN10 && X265_DEPTH == 8)
    {
        /* The above constraint is equal to Profile::MAIN */
        profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN, 1);
    }
    if (m_profile == Profile::MAIN)
    {
        /* A Profile::MAIN10 decoder can always decode Profile::MAIN */
        profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN10, 1);
    }
    /* XXX: should Main be marked as compatible with still picture? */

    /* XXX: may be a good idea to refactor the above into a function
     * that chooses the actual compatibility based upon options */

    sps->setPicWidthInLumaSamples(param.sourceWidth);
    sps->setPicHeightInLumaSamples(param.sourceHeight);
    sps->setConformanceWindow(m_conformanceWindow);
    sps->setMaxCUWidth(g_maxCUWidth);
    sps->setMaxCUHeight(g_maxCUHeight);
    sps->setMaxCUDepth(g_maxCUDepth);

    int minCUSize = sps->getMaxCUWidth() >> (sps->getMaxCUDepth() - g_addCUDepth);
    int log2MinCUSize = 0;
    while (minCUSize > 1)
    {
        minCUSize >>= 1;
        log2MinCUSize++;
    }

    sps->setLog2MinCodingBlockSize(log2MinCUSize);
    sps->setLog2DiffMaxMinCodingBlockSize(sps->getMaxCUDepth() - g_addCUDepth);

    sps->setPCMLog2MinSize(m_pcmLog2MinSize);
    sps->setUsePCM(m_usePCM);
    sps->setPCMLog2MaxSize(m_pcmLog2MaxSize);

    sps->setQuadtreeTULog2MaxSize(m_quadtreeTULog2MaxSize);
    sps->setQuadtreeTULog2MinSize(m_quadtreeTULog2MinSize);
    sps->setQuadtreeTUMaxDepthInter(param.tuQTMaxInterDepth);
    sps->setQuadtreeTUMaxDepthIntra(param.tuQTMaxIntraDepth);

    sps->setTMVPFlagsPresent(false);
    sps->setUseLossless(m_useLossless);

    sps->setMaxTrSize(1 << m_quadtreeTULog2MaxSize);

    int i;

    for (i = 0; i < g_maxCUDepth - g_addCUDepth; i++)
    {
        sps->setAMPAcc(i, param.bEnableAMP);
    }

    sps->setUseAMP(param.bEnableAMP);

    for (i = g_maxCUDepth - g_addCUDepth; i < g_maxCUDepth; i++)
    {
        sps->setAMPAcc(i, 0);
    }

    sps->setBitDepthY(X265_DEPTH);
    sps->setBitDepthC(X265_DEPTH);

    sps->setQpBDOffsetY(6 * (X265_DEPTH - 8));
    sps->setQpBDOffsetC(6 * (X265_DEPTH - 8));

    sps->setUseSAO(param.bEnableSAO);

    // TODO: hard-code these values in SPS code
    sps->setMaxTLayers(1);
    sps->setTemporalIdNestingFlag(true);
    for (i = 0; i < sps->getMaxTLayers(); i++)
    {
        sps->setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        sps->setNumReorderPics(m_numReorderPics[i], i);
    }

    // TODO: it is recommended for this to match the input bit depth
    sps->setPCMBitDepthLuma(X265_DEPTH);
    sps->setPCMBitDepthChroma(X265_DEPTH);

    sps->setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);

    sps->setScalingListFlag((m_useScalingListId == 0) ? 0 : 1);

    sps->setUseStrongIntraSmoothing(param.bEnableStrongIntraSmoothing);

    sps->setVuiParametersPresentFlag(getVuiParametersPresentFlag());
    if (sps->getVuiParametersPresentFlag())
    {
        TComVUI* vui = sps->getVuiParameters();
        vui->setAspectRatioInfoPresentFlag(getAspectRatioIdc() != -1);
        vui->setAspectRatioIdc(getAspectRatioIdc());
        vui->setSarWidth(getSarWidth());
        vui->setSarHeight(getSarHeight());
        vui->setOverscanInfoPresentFlag(getOverscanInfoPresentFlag());
        vui->setOverscanAppropriateFlag(getOverscanAppropriateFlag());
        vui->setVideoSignalTypePresentFlag(getVideoSignalTypePresentFlag());
        vui->setVideoFormat(getVideoFormat());
        vui->setVideoFullRangeFlag(getVideoFullRangeFlag());
        vui->setColourDescriptionPresentFlag(getColourDescriptionPresentFlag());
        vui->setColourPrimaries(getColourPrimaries());
        vui->setTransferCharacteristics(getTransferCharacteristics());
        vui->setMatrixCoefficients(getMatrixCoefficients());
        vui->setChromaLocInfoPresentFlag(getChromaLocInfoPresentFlag());
        vui->setChromaSampleLocTypeTopField(getChromaSampleLocTypeTopField());
        vui->setChromaSampleLocTypeBottomField(getChromaSampleLocTypeBottomField());
        vui->setNeutralChromaIndicationFlag(getNeutralChromaIndicationFlag());
        vui->setDefaultDisplayWindow(getDefaultDisplayWindow());
        vui->setFrameFieldInfoPresentFlag(getFrameFieldInfoPresentFlag());
        vui->setFieldSeqFlag(false);
        vui->setHrdParametersPresentFlag(false);
        vui->getTimingInfo()->setPocProportionalToTimingFlag(getPocProportionalToTimingFlag());
        vui->getTimingInfo()->setNumTicksPocDiffOneMinus1(getNumTicksPocDiffOneMinus1());
        vui->setBitstreamRestrictionFlag(getBitstreamRestrictionFlag());
        vui->setMotionVectorsOverPicBoundariesFlag(getMotionVectorsOverPicBoundariesFlag());
        vui->setMinSpatialSegmentationIdc(getMinSpatialSegmentationIdc());
        vui->setMaxBytesPerPicDenom(getMaxBytesPerPicDenom());
        vui->setMaxBitsPerMinCuDenom(getMaxBitsPerMinCuDenom());
        vui->setLog2MaxMvLengthHorizontal(getLog2MaxMvLengthHorizontal());
        vui->setLog2MaxMvLengthVertical(getLog2MaxMvLengthVertical());
    }

    /* set the VPS profile information */
    *getVPS()->getPTL() = *sps->getPTL();
    getVPS()->getTimingInfo()->setTimingInfoPresentFlag(false);
}

void TEncTop::xInitPPS(TComPPS *pps)
{
    pps->setConstrainedIntraPred(param.bEnableConstrainedIntra);
    bool bUseDQP = (getMaxCuDQPDepth() > 0) ? true : false;

    int lowestQP = -(6 * (X265_DEPTH - 8)); //m_cSPS.getQpBDOffsetY();

    if (getUseLossless())
    {
        if ((getMaxCuDQPDepth() == 0) && (param.rc.qp == lowestQP))
        {
            bUseDQP = false;
        }
        else
        {
            bUseDQP = true;
        }
    }

    if (bUseDQP)
    {
        pps->setUseDQP(true);
        pps->setMaxCuDQPDepth(m_maxCuDQPDepth);
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUWidth() >> (pps->getMaxCuDQPDepth()));
    }
    else
    {
        pps->setUseDQP(false);
        pps->setMaxCuDQPDepth(0);
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUWidth() >> (pps->getMaxCuDQPDepth()));
    }

    pps->setChromaCbQpOffset(param.cbQpOffset);
    pps->setChromaCrQpOffset(param.crQpOffset);

    pps->setEntropyCodingSyncEnabledFlag(param.bEnableWavefront);
    pps->setUseWP(param.bEnableWeightedPred);
    pps->setWPBiPred(param.bEnableWeightedBiPred);
    pps->setOutputFlagPresentFlag(false);
    pps->setSignHideFlag(param.bEnableSignHiding);
    pps->setDeblockingFilterControlPresentFlag(!param.bEnableLoopFilter);
    pps->setDeblockingFilterOverrideEnabledFlag(!m_loopFilterOffsetInPPS);
    pps->setPicDisableDeblockingFilterFlag(!param.bEnableLoopFilter);
    pps->setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    pps->setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);

    pps->setNumRefIdxL0DefaultActive(1);
    pps->setNumRefIdxL1DefaultActive(1);

    pps->setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
    pps->setUseTransformSkip(param.bEnableTransformSkip);
    pps->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

//! \}
