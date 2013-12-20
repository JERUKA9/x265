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

bool hasXOP(void); // instr_detect.cpp

/* The #if logic here must match the file lists in CMakeLists.txt */
#if defined(__INTEL_COMPILER)
#define HAVE_SSE3
#define HAVE_SSSE3
#define HAVE_SSE4
#define HAVE_AVX2
#elif defined(__GNUC__)
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
#define HAVE_SSE3
#define HAVE_SSSE3
#define HAVE_SSE4
#endif
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
#define HAVE_AVX2
#endif
#elif defined(_MSC_VER)
#define HAVE_SSE3
#define HAVE_SSSE3
#define HAVE_SSE4
#if _MSC_VER >= 1700 // VC11
#define HAVE_AVX2
#endif
#endif

namespace x265 {
// private x265 namespace

void Setup_Vec_BlockCopyPrimitives_sse3(EncoderPrimitives&);
void Setup_Vec_BlockCopyPrimitives_avx2(EncoderPrimitives&);

void Setup_Vec_DCTPrimitives_sse3(EncoderPrimitives&);
void Setup_Vec_DCTPrimitives_ssse3(EncoderPrimitives&);
void Setup_Vec_DCTPrimitives_sse41(EncoderPrimitives&);

void Setup_Vec_IPredPrimitives_sse3(EncoderPrimitives&);
void Setup_Vec_IPredPrimitives_sse41(EncoderPrimitives&);

void Setup_Vec_IPFilterPrimitives_ssse3(EncoderPrimitives&);
void Setup_Vec_IPFilterPrimitives_sse41(EncoderPrimitives&);

void Setup_Vec_PixelPrimitives_sse3(EncoderPrimitives&);
void Setup_Vec_PixelPrimitives_sse41(EncoderPrimitives&);
void Setup_Vec_PixelPrimitives_xop(EncoderPrimitives&);
void Setup_Vec_PixelPrimitives_avx2(EncoderPrimitives&);

/* Use primitives for the best available vector architecture */
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuMask)
{
#ifdef HAVE_SSE3
    if (cpuMask & (1 << X265_CPU_LEVEL_SSE3))
    {
        Setup_Vec_PixelPrimitives_sse3(p);
        Setup_Vec_DCTPrimitives_sse3(p);
        Setup_Vec_IPredPrimitives_sse3(p);
        Setup_Vec_BlockCopyPrimitives_sse3(p);
    }
#endif
#ifdef HAVE_SSSE3
    if (cpuMask & (1 << X265_CPU_LEVEL_SSSE3))
    {
        Setup_Vec_IPFilterPrimitives_ssse3(p);
        Setup_Vec_DCTPrimitives_ssse3(p);
    }
#endif
#ifdef HAVE_SSE4
    if (cpuMask & (1 << X265_CPU_LEVEL_SSE41))
    {
        Setup_Vec_PixelPrimitives_sse41(p);
        Setup_Vec_IPredPrimitives_sse41(p);
        Setup_Vec_IPFilterPrimitives_sse41(p);
        Setup_Vec_DCTPrimitives_sse41(p);
    }
#endif
#ifdef HAVE_AVX2
    if (cpuMask & (1 << X265_CPU_LEVEL_AVX2))
    {
        Setup_Vec_PixelPrimitives_avx2(p);
        Setup_Vec_BlockCopyPrimitives_avx2(p);
    }
#endif
}
}
