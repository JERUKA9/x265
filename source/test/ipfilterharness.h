/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Devaki <deepthidevaki@multicorewareinc.com>,
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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

#ifndef _IPFILTERHARNESS_H_1
#define _IPFILTERHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class IPFilterHarness : public TestHarness
{
protected:

    pixel *pixel_buff;
    short *short_buff;

    pixel *IPF_vec_output_p, *IPF_C_output_p;
    short *IPF_vec_output_s, *IPF_C_output_s;

    int ipf_t_size;

    bool check_IPFilter_primitive(ipfilter_pp_t ref, ipfilter_pp_t opt);
    bool check_IPFilter_primitive(ipfilter_ps_t ref, ipfilter_ps_t opt);
    bool check_IPFilter_primitive(ipfilter_sp_t ref, ipfilter_sp_t opt);
    bool check_IPFilter_primitive(ipfilter_p2s_t ref, ipfilter_p2s_t opt);
    bool check_IPFilter_primitive(ipfilter_s2p_t ref, ipfilter_s2p_t opt);
    bool check_filterHMultiplaneWghtd(filterHwghtd_t ref, filterHwghtd_t opt);
    bool check_filterVMultiplaneWghtd(filterVwghtd_t ref, filterVwghtd_t opt);

public:

    IPFilterHarness();

    virtual ~IPFilterHarness();

    const char *getName() const { return "interp"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _FILTERHARNESS_H_1
