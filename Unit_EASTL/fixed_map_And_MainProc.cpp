/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: The main procedure of example code of EASTL.
 *       This file is also an example of eastl::fixed_map<> used as
 *       key bindings.
 */

/* USE_IHM: using intrustive_hash_map is used */
#define USE_IHM // use intrusive_hash_map

#include <TWELITE>

#include <EASTL/fixed_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

#include <EASTL/sort.h>
#ifdef USE_IHM
#include <EASTL/intrusive_hash_map.h>
#endif

#include "common.hpp"

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
    const FuncDef *_x;
public:
    WidFuncDef(char key = 0, const FuncDef *x = nullptr) : _x(x) {
        mKey = key;
        if (key) {
            Serial << format("<C:k=%c,v=%s>", key, x->msg_help ? x->msg_help : "");
            Serial.flush();
        }
    }
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
const FuncDef& map_value(tmap::iterator& i) { return *(const FuncDef*)(i->second); } // access function to get value part by iterator
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

// unique_ptr: for replacement of global definition.
//   Embedded compiler environment does not support global object initialization.
static tmap v_map;

// to sort keys
static eastl::fixed_vector<char, N_ELE_SMALL> v_key;

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
    //    -> mwx::pnew(wid_fixed_string, '1', &fd_fixed_string);
    //       m.insert(wid_fixed_string);
    
    IHM_INS('1', fixed_string);
    IHM_INS('2', fixed_set);
    IHM_INS('3', fixed_list);
    IHM_INS('4', fixed_slist);
    IHM_INS('5', fixed_vector);
    IHM_INS('6', intrusive_list);
    IHM_INS('7', intrusive_hash_set);
#else
    m.insert({
        { '1', &fd_fixed_string },
        { '2', &fd_fixed_set },
        { '3', &fd_fixed_list },
        { '4', &fd_fixed_slist },
        { '5', &fd_fixed_vector },
        { '6', &fd_intrusive_list },
        { '7', &fd_intrusive_hash_set },
    });
#endif

    // put key into v_key
    for(auto&&x : m) v.push_back(map_key(x));
    sort(v.begin(), v.end(), eastl::greater<char>()); // sort in reverse order (as a code example.)

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
