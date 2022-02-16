/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::fixed_vector          : vector with fixed memory
 *  - eastl::sort                  : sort
 *  - eastl::less, eastl::greater  : compare func obj
 *  - eastl::bin_search_i          : binary search by iterator
 */

#include <TWELITE>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>

#include <EASTL/sort.h>
#include <EASTL/algorithm.h>

static const unsigned N_ELE_SMALL = 4;
static const unsigned N_ELE_LARGE = 40;
using namespace eastl;
using tvct4 = fixed_vector<uint16_t, N_ELE_SMALL, false>; // list data type (4 entries)
using tvct40 = fixed_vector<uint16_t, N_ELE_LARGE, false>; // list data type (40 entries)
static unique_ptr<tvct4> uq_vct1;

template <typename T>
static void list_vect(T& v, const char*msg) {
    Serial << crlf;
    for(auto&&x : v) {
        Serial << int(x) << ' ';
    }
    Serial << ": " << msg;
}

void test_fixed_vector() {
    int bytes_per_entry =  (sizeof(tvct40)-sizeof(tvct4)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tvct4) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(uint16_t), bytes_header, bytes_per_entry);

    if(!uq_vct1) {
        uq_vct1.reset(new tvct4({3,1,2,4}));
        Serial << crlf << "!create object by initializer list.";
    } else {
        Serial << crlf << "!already created the object.";
    }

    if (!uq_vct1) {
        Serial << crlf << "!failded to create object.";
        return;
    }

    // set `v' as reference of the object.
    auto& v = *uq_vct1;
    Serial << crlf << format("!the object addr is %08x", (void*)&v);

    list_vect(v, "init data"); // show the vector.

    v.pop_back();
    list_vect(v, "pop_back"); // show the vector.

    v.push_back(5);
    list_vect(v, "push_back"); // show the vector.

    // access by v[]
    Serial << crlf;
    for(unsigned i = 0; i < v.size(); i++) {
        v[i]++;
        Serial << int(v[i]) << ' ';
    }
    Serial << ": access by [] operator";

#if 1
    // try sorting
    typedef eastl::less<tvct4::value_type>      CF_less;
    eastl::sort(v.begin(), v.end(), CF_less());
    list_vect(v, "sort"); // show the vector.

    {
        int n = 4;
        auto i1 = eastl::binary_search_i(v.begin(), v.end(), n, CF_less());
        if (i1 != v.end()) {
            Serial << crlf << format("bin_search: found %d at index %d", n, i1 - v.begin());
        }
    }

    typedef eastl::greater<tvct4::value_type>   CF_greater;
    eastl::sort(v.begin(), v.end(), CF_greater());
    list_vect(v, "sort reverse"); // show the vector.
    
    {
        int n = 4;
        auto i1 = eastl::binary_search_i(v.begin(), v.end(), n, CF_greater());
        if (i1 != v.end()) {
            Serial << crlf << format("bin_search: found %d at index %d", n, i1 - v.begin());
        }
    }

#endif
}
