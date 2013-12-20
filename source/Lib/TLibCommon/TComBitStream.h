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

/** \file     TComBitStream.h
    \brief    class for handling bitstream (header)
*/

#ifndef X265_COMBITSTREAM_H
#define X265_COMBITSTREAM_H

#include <stdint.h>
#include <vector>
#include <stdio.h>
#include <assert.h>
#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// pure virtual class for basic bit handling
class TComBitIf
{
public:

    virtual void        writeAlignOne() {}

    virtual void        writeAlignZero() {}

    virtual void        write(UInt uiBits, UInt uiNumberOfBits)  = 0;
    virtual void        resetBits()                              = 0;
    virtual UInt getNumberOfWrittenBits() const = 0;
    virtual ~TComBitIf() {}
};

/**
 * Model of a writable bitstream that accumulates bits to produce a
 * bytestream.
 */
class TComOutputBitstream : public TComBitIf
{
    /**
     * FIFO for storage of bytes.  Use:
     *  - fifo.push_back(x) to append words
     *  - fifo.clear() to empty the FIFO
     *  - &fifo.front() to get a pointer to the data array.
     *    NB, this pointer is only valid until the next push_back()/clear()
     */
    std::vector<uint8_t> *m_fifo;

    UInt m_num_held_bits; /// number of bits not flushed to bytestream.
    UChar m_held_bits; /// the bits held and not flushed to bytestream.
    /// this value is always msb-aligned, bigendian.

public:

    // create / destroy
    TComOutputBitstream();
    ~TComOutputBitstream();

    // interface for encoding

    /**
     * append uiNumberOfBits least significant bits of uiBits to
     * the current bitstream
     */
    void        write(UInt uiBits, UInt uiNumberOfBits);

    /** insert one bits until the bitstream is byte-aligned */
    void        writeAlignOne();

    /** insert zero bits until the bitstream is byte-aligned */
    void        writeAlignZero();

    /** this function should never be called */
    void resetBits() { assert(0); }

    // utility functions

    /**
     * Return a pointer to the start of the byte-stream buffer.
     * Pointer is valid until the next write/flush/reset call.
     * NB, data is arranged such that subsequent bytes in the
     * bytestream are stored in ascending addresses.
     */
    char* getByteStream() const;

    /**
     * Return the number of valid bytes available from  getByteStream()
     */
    UInt getByteStreamLength();

    /**
     * Reset all internal state.
     */
    void clear();

    /**
     * returns the number of bits that need to be written to
     * achieve byte alignment.
     */
    int getNumBitsUntilByteAligned() { return (8 - m_num_held_bits) & 0x7; }

    /**
     * Return the number of bits that have been written since the last clear()
     */
    UInt getNumberOfWrittenBits() const { return UInt(m_fifo->size()) * 8 + m_num_held_bits; }

    /**
     * Return a reference to the internal fifo
     */
    std::vector<uint8_t>& getFIFO() { return *m_fifo; }

    UChar getHeldBits()          { return m_held_bits; }

    /** Return a reference to the internal fifo */
    std::vector<uint8_t>& getFIFO() const { return *m_fifo; }

    void          addSubstream(TComOutputBitstream* pcSubstream);
    void writeByteAlignment();

    //! returns the number of start code emulations contained in the current buffer
    int countStartCodeEmulations();
};
}
//! \}

#endif // ifndef X265_COMBITSTREAM_H
