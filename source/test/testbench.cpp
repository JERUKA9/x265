/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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
#include "pixelharness.h"
#include "mbdstharness.h"
#include "ipfilterharness.h"
#include "intrapredharness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace x265;

static const char *CpuType[] =
{
    "",
    "",
    "SSE2",
    "SSE3",
    "SSSE3",
    "SSE4.1",
    "SSE4.2",
    "AVX",
    "AVX2",
    0
};

extern int instrset_detect();

int main(int argc, char *argv[])
{
    int cpuid = instrset_detect(); // Detect supported instruction set
    const char *testname = 0;
    int cpuid_user = -1;

    for (int i = 1; i < argc - 1; i += 2)
    {
        if (!strcmp(argv[i], "--cpuid"))
        {
            cpuid_user = atoi(argv[i + 1]);
        }
        if (!strcmp(argv[i], "--test"))
        {
            testname = argv[i + 1];
            printf("Testing only harnesses that match name <%s>\n", testname);
        }
    }

    int seed = (int)time(NULL);
    const char *bpp[] = { "8bpp", "16bpp" };
    printf("Using random seed %X %s\n", seed, bpp[HIGH_BIT_DEPTH]);
    srand(seed);

    PixelHarness  HPixel;
    MBDstHarness  HMBDist;
    IPFilterHarness HIPFilter;
    IntraPredHarness HIPred;

    // To disable classes of tests, simply comment them out in this list
    TestHarness *harness[] =
    {
        &HPixel,
        &HMBDist,
        &HIPFilter,
        &HIPred
    };

    EncoderPrimitives cprim;
    memset(&cprim, 0, sizeof(EncoderPrimitives));
    Setup_C_Primitives(cprim);

    int cpuid_low = 2;
    int cpuid_high = cpuid;

    if (cpuid_user >= 0)
    {
        cpuid_low = cpuid_high = cpuid_user;
    }
    for (int i = cpuid_low; i <= cpuid_high; i++)
    {
#if ENABLE_VECTOR_PRIMITIVES
        EncoderPrimitives vecprim;
        memset(&vecprim, 0, sizeof(vecprim));
        Setup_Vector_Primitives(vecprim, 1 << i);
        printf("Testing intrinsic primitives: %s (%d)\n", CpuType[i], i);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
                continue;
            if (!harness[h]->testCorrectness(cprim, vecprim))
            {
                fprintf(stderr, "\nx265: intrinsic primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }

#endif // if ENABLE_VECTOR_PRIMITIVES

#if ENABLE_ASM_PRIMITIVES
        EncoderPrimitives asmprim;
        memset(&asmprim, 0, sizeof(asmprim));
        Setup_Assembly_Primitives(asmprim, 1 << i);
        printf("Testing assembly primitives: %s (%d)\n", CpuType[i], i);
        for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
        {
            if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
                continue;
            /* Here it should be asmprim and not vecprim. Right? */
            if (!harness[h]->testCorrectness(cprim, asmprim))
            {
                fprintf(stderr, "\nx265: asm primitive has failed. Go and fix that Right Now!\n");
                return -1;
            }
        }

#endif // if ENABLE_ASM_PRIMITIVES
    }

    /******************* Cycle count for all primitives **********************/

    EncoderPrimitives optprim;
    memset(&optprim, 0, sizeof(optprim));
    for (int i = 2; i < cpuid; i++)
    {
#if ENABLE_VECTOR_PRIMITIVES
        Setup_Vector_Primitives(optprim, 1 << i);
#endif
#if ENABLE_ASM_PRIMITIVES
        Setup_Assembly_Primitives(optprim, 1 << i);
#endif
    }

    printf("\nTest performance improvement with full optimizations\n");

    for (size_t h = 0; h < sizeof(harness) / sizeof(TestHarness*); h++)
    {
        if (testname && strncmp(testname, harness[h]->getName(), strlen(testname)))
            continue;
        printf("== %s primitives ==\n", harness[h]->getName());
        harness[h]->measureSpeed(cprim, optprim);
    }

    printf("\n");
    return 0;
}
