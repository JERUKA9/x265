/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <min.chen@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
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

#include "mbdstharness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace x265;

struct DctConf_t
{
    const char *name;
    int width;
};

const DctConf_t DctConf_infos[] =
{
    { "dst4x4\t",    4 },
    { "dct4x4\t",    4 },
    { "dct8x8\t",    8 },
    { "dct16x16",   16 },
    { "dct32x32",   32 },
};

const DctConf_t IDctConf_infos[] =
{
    { "idst4x4\t",    4 },
    { "idct4x4\t",    4 },
    { "idct8x8\t",    8 },
    { "idct16x16",   16 },
    { "idct32x32",   32 },
};

MBDstHarness::MBDstHarness()
{
    mbuf1 = (short*)X265_MALLOC(short, mb_t_size);
    mbufdct = (short*)X265_MALLOC(short, mb_t_size);
    mbufidct = (int*)X265_MALLOC(int,   mb_t_size);

    mbuf2 = (short*)X265_MALLOC(short, mem_cmp_size);
    mbuf3 = (short*)X265_MALLOC(short, mem_cmp_size);
    mbuf4 = (short*)X265_MALLOC(short, mem_cmp_size);

    mintbuf1 = (int*)X265_MALLOC(int, mb_t_size);
    mintbuf2 = (int*)X265_MALLOC(int, mb_t_size);
    mintbuf3 = (int*)X265_MALLOC(int, mem_cmp_size);
    mintbuf4 = (int*)X265_MALLOC(int, mem_cmp_size);
    mintbuf5 = (int*)X265_MALLOC(int, mem_cmp_size);
    mintbuf6 = (int*)X265_MALLOC(int, mem_cmp_size);
    mintbuf7 = (int*)X265_MALLOC(int, mem_cmp_size);
    mintbuf8 = (int*)X265_MALLOC(int, mem_cmp_size);

    if (!mbuf1 || !mbuf2 || !mbuf3 || !mbuf4 || !mbufdct)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    if (!mintbuf1 || !mintbuf2 || !mintbuf3 || !mintbuf4 || !mintbuf5 || !mintbuf6 || !mintbuf7 || !mintbuf8)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        exit(1);
    }

    const int idct_max = (1 << (BIT_DEPTH + 4)) - 1;
    for (int i = 0; i < mb_t_size; i++)
    {
        mbuf1[i] = rand() & PIXEL_MAX;
        mbufdct[i] = (rand() & PIXEL_MAX) - (rand() & PIXEL_MAX);
        mbufidct[i] = (rand() & idct_max);
    }

    for (int i = 0; i < mb_t_size; i++)
    {
        mintbuf1[i] = rand() & PIXEL_MAX;
        mintbuf2[i] = rand() & PIXEL_MAX;
    }

#if _DEBUG
    memset(mbuf2, 0, mem_cmp_size);
    memset(mbuf3, 0, mem_cmp_size);
    memset(mbuf4, 0, mem_cmp_size);
    memset(mbufidct, 0, mb_t_size);

    memset(mintbuf3, 0, mem_cmp_size);
    memset(mintbuf4, 0, mem_cmp_size);
    memset(mintbuf5, 0, mem_cmp_size);
    memset(mintbuf6, 0, mem_cmp_size);
    memset(mintbuf7, 0, mem_cmp_size);
    memset(mintbuf8, 0, mem_cmp_size);
#endif // if _DEBUG
}

MBDstHarness::~MBDstHarness()
{
    X265_FREE(mbuf1);
    X265_FREE(mbuf2);
    X265_FREE(mbuf3);
    X265_FREE(mbuf4);
    X265_FREE(mbufdct);
    X265_FREE(mbufidct);

    X265_FREE(mintbuf1);
    X265_FREE(mintbuf2);
    X265_FREE(mintbuf3);
    X265_FREE(mintbuf4);
    X265_FREE(mintbuf5);
    X265_FREE(mintbuf6);
    X265_FREE(mintbuf7);
    X265_FREE(mintbuf8);
}

bool MBDstHarness::check_dct_primitive(dct_t ref, dct_t opt, int width)
{
    int j = 0;
    int cmp_size = sizeof(int) * width * width;

    for (int i = 0; i <= 100; i++)
    {
        ref(mbufdct + j, mintbuf1, width);
        opt(mbufdct + j, mintbuf2, width);

        if (memcmp(mintbuf1, mintbuf2, cmp_size))
        {
#if _DEBUG
            // redo for debug
            ref(mbufdct + j, mintbuf1, width);
            opt(mbufdct + j, mintbuf2, width);
#endif
            return false;
        }

        j += 16;
#if _DEBUG
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::check_idct_primitive(idct_t ref, idct_t opt, int width)
{
    int j = 0;
    int cmp_size = sizeof(short) * width * width;

    for (int i = 0; i <= 100; i++)
    {
        ref(mbufidct + j, mbuf2, width);
        opt(mbufidct + j, mbuf3, width);

        if (memcmp(mbuf2, mbuf3, cmp_size))
        {
#if _DEBUG
            // redo for debug
            ref(mbufidct + j, mbuf2, width);
            opt(mbufidct + j, mbuf3, width);
#endif
            return false;
        }

        j += 16;
#if _DEBUG
        memset(mbuf2, 0xCD, mem_cmp_size);
        memset(mbuf3, 0xCD, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::check_dequant_primitive(dequant_t ref, dequant_t opt)
{
    int j = 0;

    for (int i = 0; i <= 5; i++)
    {
        int width = (rand() % 4 + 1) * 4;

        if (width == 12)
        {
            width = 32;
        }
        int height = width;

        int scale = rand() % 58;
        int per = scale / 6;
        int rem = scale % 6;

        bool useScalingList = (scale % 2 == 0) ? false : true;

        uint32_t log2TrSize = (rand() % 4) + 2;

        int cmp_size = sizeof(int) * height * width;

        opt(mintbuf1 + j, mintbuf3, width, height, per, rem, useScalingList, log2TrSize, mintbuf2 + j);
        ref(mintbuf1 + j, mintbuf4, width, height, per, rem, useScalingList, log2TrSize, mintbuf2 + j);

        if (memcmp(mintbuf3, mintbuf4, cmp_size))
            return false;

        j += 16;
#if _DEBUG
        memset(mintbuf3, 0, mem_cmp_size);
        memset(mintbuf4, 0, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::check_quant_primitive(quant_t ref, quant_t opt)
{
    int j = 0;

    // fill again to avoid error Q value
    for (int i = 0; i < mb_t_size; i++)
    {
        mintbuf1[i] = rand() & PIXEL_MAX;
        mintbuf2[i] = rand() & PIXEL_MAX;
    }

    for (int i = 0; i <= 5; i++)
    {
        int width = (rand() % 4 + 1) * 4;

        if (width == 12)
        {
            width = 32;
        }
        int height = width;

        uint32_t optReturnValue = 0;
        uint32_t refReturnValue = 0;

        int bits = rand() % 32;
        int valueToAdd = rand() % (32 * 1024);
        int cmp_size = sizeof(int) * height * width;
        int numCoeff = height * width;
        int optLastPos = -1, refLastPos = -1;

        refReturnValue = ref(mintbuf1 + j, mintbuf2 + j, mintbuf5, mintbuf6, bits, valueToAdd, numCoeff, &refLastPos);
        optReturnValue = opt(mintbuf1 + j, mintbuf2 + j, mintbuf3, mintbuf4, bits, valueToAdd, numCoeff, &optLastPos);

        if (memcmp(mintbuf3, mintbuf5, cmp_size))
            return false;

        if (memcmp(mintbuf4, mintbuf6, cmp_size))
            return false;

        if (optReturnValue != refReturnValue)
            return false;

        if (optLastPos != refLastPos)
            return false;

        j += 16;

#if _DEBUG
        memset(mintbuf3, 0, mem_cmp_size);
        memset(mintbuf4, 0, mem_cmp_size);
        memset(mintbuf5, 0, mem_cmp_size);
        memset(mintbuf6, 0, mem_cmp_size);
#endif
    }

    return true;
}

bool MBDstHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int i = 0; i < NUM_DCTS; i++)
    {
        if (opt.dct[i])
        {
            if (!check_dct_primitive(ref.dct[i], opt.dct[i], DctConf_infos[i].width))
            {
                printf("\n%s failed\n", DctConf_infos[i].name);
                return false;
            }
        }
    }

    for (int i = 0; i < NUM_IDCTS; i++)
    {
        if (opt.idct[i])
        {
            if (!check_idct_primitive(ref.idct[i], opt.idct[i], IDctConf_infos[i].width))
            {
                printf("%s failed\n", IDctConf_infos[i].name);
                return false;
            }
        }
    }

    if (opt.dequant)
    {
        if (!check_dequant_primitive(ref.dequant, opt.dequant))
        {
            printf("dequant: Failed!\n");
            return false;
        }
    }

    if (opt.quant)
    {
        if (!check_quant_primitive(ref.quant, opt.quant))
        {
            printf("quant: Failed!\n");
            return false;
        }
    }

    return true;
}

void MBDstHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int value = 0; value < NUM_DCTS; value++)
    {
        if (opt.dct[value])
        {
            printf("%s\t", DctConf_infos[value].name);
            REPORT_SPEEDUP(opt.dct[value], ref.dct[value], mbuf1, mintbuf1, DctConf_infos[value].width);
        }
    }

    for (int value = 0; value < NUM_IDCTS; value++)
    {
        if (opt.idct[value])
        {
            printf("%s\t", IDctConf_infos[value].name);
            REPORT_SPEEDUP(opt.idct[value], ref.idct[value], mbufidct, mbuf2, IDctConf_infos[value].width);
        }
    }

    if (opt.dequant)
    {
        printf("dequant\t\t");
        REPORT_SPEEDUP(opt.dequant, ref.dequant, mintbuf1, mintbuf3, 32, 32, 5, 2, false, 5, mintbuf2);
    }

    if (opt.quant)
    {
        printf("quant\t\t");
        int dummy = -1;
        REPORT_SPEEDUP(opt.quant, ref.quant, mintbuf1, mintbuf2, mintbuf3, mintbuf4, 23, 23785, 32 * 32, &dummy);
    }
}
