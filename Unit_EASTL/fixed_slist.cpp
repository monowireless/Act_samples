/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::fixed_slist        : one way link list
 *  - eastl::pair               : pair two types
 */

#include <TWELITE>
#include <EASTL/fixed_slist.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

/* Note: 
 * The fixed_list makes list structure in linear memory model with limited number of
 * entries. The insert operation takes O(n) order.
 * 
 * If you want to allocate an object other than unique_ptr<> (e.g. in stack or global),
 * see the example of `fixed_string'.
 * 
 * The ::make_pair<> is defined as std::make_pair<> in the using_mwx_defs.hpp.
 */

using namespace eastl;
using tdata = pair<uint8_t, void (*)()>; // data is pair of uint8_t and function ptr.

static const unsigned N_ELE_SMALL = 3;
static const unsigned N_ELE_LARGE = 30;
using tlst3 = fixed_slist<tdata, N_ELE_SMALL, false>; // list data type (3 entries)
using tlst30 = fixed_slist<tdata, N_ELE_LARGE, false>; // list data type (30 entries)

static unique_ptr<tlst3> uq_cnt1;

static void func0() { Serial << "func0"; }
static void func1() { Serial << "func1"; }
static void func2() { Serial << "func2"; }
static void func3() { Serial << "func3"; }

void test_fixed_slist() {
    int bytes_per_entry =  (sizeof(tlst30)-sizeof(tlst3)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tlst3) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(tdata), bytes_header, bytes_per_entry);

    if (!uq_cnt1) uq_cnt1.reset(new tlst3()); // create own object (reserved in HEAP area)
    Serial << crlf << format("The fixed_slist object created at %08x."
            , (void*)&(*uq_cnt1), sizeof(tlst3));

    Serial << crlf << "Perform insert Operation.";
    if(!uq_cnt1->full()) uq_cnt1->insert(uq_cnt1->begin(), eastl::make_pair('0', func0));
    if(!uq_cnt1->full()) uq_cnt1->insert(uq_cnt1->begin(), eastl::make_pair('1', func1));
    if(!uq_cnt1->full()) uq_cnt1->insert(uq_cnt1->begin(), eastl::make_pair('2', func2));
    if(!uq_cnt1->full()) uq_cnt1->insert(uq_cnt1->begin(), eastl::make_pair('3', func3));

    Serial << crlf << "Show all entries.";
    for (auto &x : *uq_cnt1) {
        Serial << crlf << x.first << ':';
        x.second(); // call the function stored in the second.
    }

    Serial << crlf << "Find entry and erase it.";
    for(auto p = uq_cnt1->begin(); p != uq_cnt1->end(); p++) {
        if (p->first == '1') {
            Serial << crlf << format("..found %c:func#%08x", p->first, p->second);

            uq_cnt1->erase(p);
            Serial << crlf << "..erased";
            break;
        }
    }

    Serial << crlf << "Show all entries.";
    for (auto &x : *uq_cnt1) {
        Serial << crlf << x.first << ':';
        x.second(); // call the function stored in the second.
    }
}