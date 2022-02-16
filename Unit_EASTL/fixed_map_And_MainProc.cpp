/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: The main procedure of example code of EASTL.
 *       This file is also an example of the followings:
 *        - fixed_map
 *        - fixed_vector
 *        - intrusive_hash_map
 *        - sort (with custom compare function object)
 */

/* USE_IHM: using intrustive_hash_map is used */
//#define USE_IHM // use intrusive_hash_map

/* includes */
#include <TWELITE>

#include <EASTL/fixed_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

#include <EASTL/sort.h>
#ifdef USE_IHM
#include <EASTL/internal/intrusive_hashtable.h>
#include <EASTL/intrusive_hash_map.h>
#endif

#include <string.h>
#include "common.hpp"

/* namespace */
using namespace eastl;

static const unsigned N_BUCKET_CT = 7;  // hash bucket size in intrustive_hash_map.
static const unsigned N_ELE_SMALL = 10; // max entry number
static const unsigned N_ELE_LARGE = 40; // just for calculating element's size in containers.

// used for value entitiy of map.
struct FuncDef {
    void (*func)();
    const char *msg_help;
};

#ifdef USE_IHM
// for intrusive map values, element object needs to be inherited from intrusive_hash_node_key<key_type>
//   this class will embed key and value information.
class WidFuncDef : public intrusive_hash_node_key<char>
{
    using SUP = intrusive_hash_node_key; // alias
    const FuncDef *_x; // function and label
public:
    WidFuncDef(char key = 0, const FuncDef *x = nullptr) : _x(x)  { SUP::mKey = key; } // not sure why intrusive_hash_node_key<char>::mKey(key) causes an error.
    void func() { _x->func(); }
    const char* msg_help() { return _x->msg_help; }
    const FuncDef* data() { return _x; }
};

// type definitions and key/value access functions
using tmap = eastl::intrusive_hash_map<char, WidFuncDef, N_BUCKET_CT>;       // more bucket_count, more spreading(efficient) and more memory usage.
template <typename T> const FuncDef& map_value(T i) { return *(i->data()); } // access function to get value part by iterator
tmap::key_type map_key(tmap::value_type ref) { return ref.mKey; }            // access function to get key part by iterator
#else
// type definitions and key/value access functions
using tmap = fixed_map<char, const FuncDef*, N_ELE_SMALL>;
using tmap_large = fixed_map<char, const FuncDef*, N_ELE_LARGE>;
template <typename T> const FuncDef& map_value(T i) { return *(const FuncDef*)(i->second); } // access function to get value part by iterator
tmap::key_type map_key(tmap::reference& ref) { return ref.first; }    // access function to get key part by iterator
#endif

#ifdef USE_IHM
// Func and label tables
#define DEF_FuncDef(n) static const FuncDef fd_##n = { test_##n, #n }; \
                       static WidFuncDef wid_##n;
// e.g.) expand DEF_FuncDef(fixed_xxx)
//       -> static const FuncDef fd_fixed_xxx = { test_fixed_xxx, "fixed_xxx" }
//          static WidFuncDef wid_fixed_xxx;
#else
// Func and label tables
#define DEF_FuncDef(n) static const FuncDef fd_##n = { test_##n, #n }
// e.g.) expand DEF_FuncDef(fixed_xxx)
//       -> static const FuncDef fd_fixed_xxx = { test_fixed_xxx, "fixed_xxx" }
#endif

/* defines test functions and label string in const(ROM) */
DEF_FuncDef(fixed_string);
DEF_FuncDef(fixed_set);
DEF_FuncDef(fixed_list);
DEF_FuncDef(fixed_slist);
DEF_FuncDef(fixed_vector);
DEF_FuncDef(intrusive_list);
DEF_FuncDef(intrusive_hash_set);
DEF_FuncDef(ring_buffer);
DEF_FuncDef(vector_multimap);

// unique_ptr: for replacement of global definition.
//   Embedded compiler environment does not support global object initialization.
static tmap v_map;

// to sort keys
static eastl::fixed_vector<char, N_ELE_SMALL> v_key;
struct VPCompare {
    bool operator()(char lhs, char rhs) const {
        const char *str_l = map_value(v_map.find(lhs)).msg_help;
        const char *str_r = map_value(v_map.find(rhs)).msg_help;
        return strcmp(str_l, str_r) < 0;
    }
};

// clear serial terminal
void clear_screen() {
    // clear screen and show the test title.
    Serial << "\e[H\e[2J";
    delay(50); // wait for screen clean up
}

// show help, just listing of all map entries.
void help() {
    clear_screen();
    Serial      << "!!!Unit_EASTL"
        << crlf << "!sample codes using EASTL.";
    
    for (auto&&x : v_key) {
        // if there is possibility that key is not found in map container.
        auto&&e = v_map.find(x);
        if (e != v_map.end()) {
            Serial << crlf << format(" %c : %s", x, map_value(e).msg_help);
        }        
    }
}

/*** the setup procedure (called on boot) */
void setup() {
    // construnt map table for keybind by placement new.
    mwx::pnew(v_map); // key and value map
    mwx::pnew(v_key); // key listing (intended to be sorted)

    // alias to m and v
    auto& m = v_map;
    auto& v = v_key;

    // add keybind (allows put all entries as an initializer_list argument.)
#ifdef USE_IHM
    #define IHM_INS(key, base) m.insert(*mwx::pnew(wid_##base, key, &fd_##base))
    // IHM_INS('1', fd_fixed_xxx) 
    //   -> m.insert(*mwx::pnew(wid_fixed_xxx, '1', &fd_fixed_xxx));
    
    IHM_INS('t', fixed_string);
    IHM_INS('s', fixed_set);
    IHM_INS('l', fixed_list);
    IHM_INS('m', fixed_slist);
    IHM_INS('v', fixed_vector);
    IHM_INS('L', intrusive_list);
    IHM_INS('S', intrusive_hash_set);
    IHM_INS('r', ring_buffer);
    IHM_INS('V', vector_multimap);
#else
    m.insert({
        { 't', &fd_fixed_string },
        { 's', &fd_fixed_set },
        { 'l', &fd_fixed_list },
        { 'm', &fd_fixed_slist },
        { 'v', &fd_fixed_vector },
        { 'L', &fd_intrusive_list },
        { 'S', &fd_intrusive_hash_set },
        { 'r', &fd_ring_buffer },
        { 'V', &fd_vector_multimap },
    });
#endif

    // put key into v_key
    for(auto&&x : m) v.push_back(map_key(x));
    sort(v.begin(), v.end()
                , VPCompare()               // sort by label string
                // , eastl::greater<char>() // sort by key in reverse order
        ); 

    // show help
    help();

    // show size info of the container.
#ifdef USE_IHM
    Serial << crlf << crlf << "*** intrusive_hash_map is used for keybindings ***";
    Serial << crlf << format("SIZE: ele=%dby, container=%d with bucketsz=%dby", sizeof(WidFuncDef), sizeof(tmap), m.bucket_count());
#else
    Serial << crlf << crlf << "*** fixed_map is used for keybindings ***";
    int bytes_per_entry =  (sizeof(tmap_large)-sizeof(tmap)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tmap) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(char) + sizeof(FuncDef), bytes_header, bytes_per_entry);
#endif
}

/*** the loop procedure (called every event) */
void loop() {
    if (Serial.available()) {
        int c = Serial.read();

        // alias to m
        auto& m = v_map;

        // search key
        auto&& i = m.find(char(c));
        if (i != m.end()) {
            clear_screen();
            Serial << "*** " << map_value(i).msg_help << " ***" << crlf;
            map_value(i).func(); // execute test!
        } else switch(c) {
            case 0xd: help(); break;
            case '!': the_twelite.reset_system(); break;
            default: break;
        }
    }
}
