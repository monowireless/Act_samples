/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::fixed_list        : bi-directional link list
 *  - eastl::pair              : pair two types
 */

#include <TWELITE>
#include <EASTL/fixed_list.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/sort.h>

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

// define function using in element data type
static void func0() { Serial << "func0"; }
static void func1() { Serial << "func1"; }
static void func2() { Serial << "func2"; }
static void func3() { Serial << "func3"; }

// element data type
using tdata = eastl::pair<uint8_t, void (*)()>; // data is pair of uint8_t and function ptr.
// define operators for sorting
inline bool operator==(const tdata& a, const tdata& b) { return a.first == b.first; }
inline bool operator<(const tdata& a, const tdata& b) { return a.first < b.first; }
inline bool operator>(const tdata& a, const tdata& b) { return a.first > b.first; }

// container
static const unsigned N_ELE_SMALL = 3;
static const unsigned N_ELE_LARGE = 30;
using tlst3 = fixed_list<tdata, N_ELE_SMALL, false>; // list data type (3 entries)
using tlst30 = fixed_list<tdata, N_ELE_LARGE, false>; // list data type (30 entries)
// unique pointer of container
static unique_ptr<tlst3> uq_cnt1;

template <typename T>
void show_all(T& v, const char *msg) {
    Serial << crlf << msg << '|';
    for (auto &x : v) {
        Serial << x.first << ':';
        x.second(); // call the function stored in the second.
        Serial << "  ";
    }
}

//inline bool operator<(const SetWidget& a, const SetWidget& b)
//    { return a.mX < b.mX; }
void test_fixed_list() {
    // check container size information
    int bytes_per_entry =  (sizeof(tlst30)-sizeof(tlst3)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tlst3) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(tdata), bytes_header, bytes_per_entry);

    // create an container object.
    if (!uq_cnt1) uq_cnt1.reset(new tlst3()); // create own object (reserved in HEAP area)
    Serial << crlf << format("The fixed_list object created at %08x."
            , (void*)&(*uq_cnt1), sizeof(tlst3));

    auto& v = *uq_cnt1; // alias as `v'

    Serial << crlf << "Perform insert Operation.";
    if(!v.full()) v.insert(v.begin(), eastl::make_pair('K', func0));
    if(!v.full()) v.insert(v.begin(), eastl::make_pair('a', func1));
    if(!v.full()) v.insert(v.begin(), eastl::make_pair('0', func2));
    if(!v.full()) v.insert(v.begin(), eastl::make_pair('X', func3));

    show_all(v, "show all entries");

    Serial << crlf << "Find entry and erase it.";
    for(auto p = v.begin(); p != v.end(); p++) {
        if (p->first == '1') {
            Serial << crlf << format("..found %c:func#%08x", p->first, p->second);

            v.erase(p);
            Serial << crlf << "..erased";
            break;
        }
    }

    show_all(v, "show all entries");

    // try sorting
    typedef eastl::less<tlst3::value_type>      CF_less;
    eastl::bubble_sort(v.begin(), v.end(), CF_less());
    show_all(v, "sorted");
    
    typedef eastl::greater<tlst3::value_type>   CF_greater;
    eastl::bubble_sort(v.begin(), v.end(), CF_greater());
    show_all(v, "sorted(rev)");
}