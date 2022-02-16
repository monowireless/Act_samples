/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::ring_buffer             : ring_buffer
 *  -    vector<basic_string>            dynamic allocation, intended to use allocate once manner.
 *  -    fixed_vector<fixed_string>      fully fixed allocation.
 */


#include <TWELITE>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

#include "common.hpp"

using namespace eastl;

// allocation size
const size_t N_RING_STR = 16;
const size_t N_RING_FSTR = 4;

using mystr = basic_string<char>;
using myfstr = fixed_string<char, 16, false>;

// using vector and basic_string
ring_buffer< mystr, vector<mystr> > rbuff_vstr;

// using fixed_vector and fixed_string
//   note: must reserve additional one block. (N_RING_FSTR+1)
ring_buffer< myfstr, fixed_vector<myfstr, N_RING_FSTR+1, false> > rbuff_vfstr;
// ring_buffer< myfstr, vector<myfstr> > rbuff_vfstr; // if using vector.

void test_ring_buffer() {    
    check_heap(false); // init internal vars

    static bool b_init_str = false;
    static bool b_init_fstr = false;

    // dynamic allocation, but use as allocate once manner.
    {
        Serial << crlf << "!!!vector, basic_string";
        if (!b_init_str) {
            Serial << crlf << format("Check single str(sizeof(mystr)=%d) alloc:", sizeof(mystr));
            mystr tmp;
            check_heap(); // here, no heap allocation.

            Serial << crlf << "Create ringbuffer:";
            mwx::pnew(rbuff_vstr, N_RING_STR); // 204bytes allocated in HEAP (12bytes for ring_buffer, 12x16bytes for vector<string> instance with empty string)
            check_heap();

            // reserve string buffer dynamically.
            Serial << crlf << "Allocate Strings:";
            auto&& it = rbuff_vstr.get_container().begin();
            auto&& itEnd = rbuff_vstr.get_container().end();
            for(; it != itEnd; ++it) {
                (*it).reserve(32);
                check_heap();
            }
            b_init_str = true;
        }

        auto& r = rbuff_vstr;

        if (!r.full()) r.push_front() = "hello,";
        if (!r.full()) r.push_front() = "I";
        if (!r.full()) r.push_front() = "am";
        if (!r.full()) r.push_front() = "a";
        if (!r.full()) r.push_front() = "super";
        if (!r.full()) r.push_front() = "hero!";

        Serial << crlf;
        while(!r.empty()) {
            auto&& e = r.back();
            Serial << e.c_str() << ' ';
            r.pop_back();
        }
        check_heap(); // no heap allocation from putting string data by push_front() to displaying data by back(), pop_back().
    }

    // fully fixed allocation
    {
        Serial << crlf << "!!!fixed_vector, fixed_string";

        if (!b_init_fstr) {
            Serial << crlf << format("Init fixed ring_buffer(siz=%d)", sizeof(rbuff_vfstr));
            mwx::pnew(rbuff_vfstr, N_RING_FSTR);
            check_heap();
            b_init_fstr = true;
        }

        auto& r = rbuff_vfstr;

        if (!r.full()) r.push_front() = "hello,";
        if (!r.full()) r.push_front() = "my";
        if (!r.full()) r.push_front() = "world";
        if (!r.full()) r.push_front() = "is";
        if (!r.full()) r.push_front() = "too messy.";

        Serial << crlf;
        while(!r.empty()) {
            auto&& e = r.back();
            Serial << e.c_str() << ' ';
            r.pop_back();
        }
        Serial << crlf << format("rbuff_fstr.size() = %d", r.size());
    }
}