/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
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

#ifndef _INTRAPREDHARNESS_H_1
#define _INTRAPREDHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class IntraPredHarness : public TestHarness
{
protected:

    pixel *pixel_buff;
    pixel *pixel_out_c;
    pixel *pixel_out_vec;
    pixel *pixel_out_33_c;
    pixel *pixel_out_33_vec;

    pixel *IP_vec_output_p, *IP_C_output_p;

    static const int ip_t_size = 4 * 65 * 65 * 100;
    static const int out_size = 64 * FENC_STRIDE;
    static const int out_size_33 = 33 * 64 * FENC_STRIDE;

    bool check_dc_primitive(intra_dc_t ref, intra_dc_t opt);
    bool check_planar_primitive(intra_planar_t ref, intra_planar_t opt);
    bool check_angular_primitive(intra_ang_t ref, intra_ang_t opt);
    bool check_allangs_primitive(const intra_allangs_t ref[], const intra_allangs_t opt[]);

public:

    IntraPredHarness();

    virtual ~IntraPredHarness();

    const char *getName() const { return "intrapred"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _INTRAPREDHARNESS_H_1
