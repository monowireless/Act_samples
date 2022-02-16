/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* fixed_set is not recommended because of big usage in memory */
/* fixed_set は使用メモリ量が多いため使用は推奨しない */

#include <TWELITE>
#include <EASTL/fixed_set.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/sort.h>

using namespace eastl;

static const unsigned N_ELE_SMALL = 4;
static const unsigned N_ELE_LARGE = 30;
using type_ele = uint64_t;
using tcnt = fixed_set<type_ele, N_ELE_SMALL, false>; // list data type (3 entries)
using tcnt_large = fixed_set<type_ele, N_ELE_LARGE, false>; // list data type (30 entries)
//using tcnt = fixed_multiset<type_ele, N_ELE_SMALL, false>; // list data type (3 entries)
//using tcnt_large = fixed_multiset<type_ele, N_ELE_LARGE, false>; // list data type (30 entries)

static unique_ptr<tcnt> uq_cnt1;

void test_fixed_set() {
    int bytes_per_entry =  (sizeof(tcnt_large)-sizeof(tcnt)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tcnt) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("tcnt ELE=%d/MEM=%d, tcnt_large ELE=%d/MEM=%d", N_ELE_SMALL, sizeof(tcnt), N_ELE_LARGE, sizeof(tcnt_large));
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(type_ele), bytes_header, bytes_per_entry);
    
    if (!uq_cnt1) uq_cnt1.reset(new tcnt()); // create own object (reserved in HEAP area)
    Serial << crlf << format("The fixed_set object created at %08x."
            , (void*)&(*uq_cnt1), sizeof(tcnt));

    auto& v = *uq_cnt1;

    Serial << crlf << "Perform insert Operation.";
    if(v.size() < v.max_size()) v.insert(4);
    if(v.size() < v.max_size()) v.insert(2);
    if(v.size() < v.max_size()) v.insert(3);
    if(v.size() < v.max_size()) v.insert(4);
    if(v.size() < v.max_size()) v.insert(1);
    
    Serial << crlf << "Show all entries.";
    for (auto &x : v) {
        Serial << crlf << int(x) << ':';
    }
    
    int key = 3;
    auto it = v.find(key);
    Serial << crlf << ((it != v.end()) ? "found " : "not found ") << key;

    Serial << crlf << "Erase an entry.";
    v.erase(it);
    Serial << crlf << "Show all entries.";
    for (auto &x : *uq_cnt1) {
        Serial << crlf << int(x) << ':';
    }
    
    key = 5;
    it = v.find(5);
    Serial << crlf << ((it != v.end()) ? "found " : "not found ") << key;

#if 0
    // try sorting
    typedef eastl::less<type_ele>      CompareFunction;
    eastl::bubble_sort(v.begin(), v.end(), CompareFunction());
    for (auto &x : v) {
        Serial << crlf << int(x) << ':';
    }
#endif
}