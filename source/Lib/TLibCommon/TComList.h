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

/** \file     TComList.h
    \brief    general list class (header)
*/

#ifndef X265_TCOMLIST_H
#define X265_TCOMLIST_H

#include "CommonDef.h"

#include <list>
#include <assert.h>
#include <cstdlib>

namespace x265 {
// private namespace


//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// list template
template<class C>
class TComList : public std::list<C>
{
public:

    typedef typename std::list<C>::iterator TComIterator;

    TComList& operator +=(const TComList& rcTComList)
    {
        if (!rcTComList.empty())
        {
            insert(this->end(), rcTComList.begin(), rcTComList.end());
        }
        return *this;
    } // leszek

    C popBack()
    {
        C cT = this->back();

        this->pop_back();
        return cT;
    }

    C popFront()
    {
        C cT = this->front();

        this->pop_front();
        return cT;
    }

    void pushBack(const C& rcT)
    {
        /*assert( sizeof(C) == 4);*/
        if (rcT != NULL)
        {
            this->push_back(rcT);
        }
    }

    void pushFront(const C& rcT)
    {
        /*assert( sizeof(C) == 4);*/
        if (rcT != NULL)
        {
            this->push_front(rcT);
        }
    }

    TComIterator find(const C& rcT) // leszek
    {
        return find(this->begin(), this->end(), rcT);
    }
};
}
//! \}

#endif // ifndef X265_TCOMLIST_H
