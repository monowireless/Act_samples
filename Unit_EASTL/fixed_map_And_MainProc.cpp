/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: The main procedure of example code of EASTL.
 *       This file is also an example of eastl::fixed_map<> used as
 *       key bindings.
 */

#include <TWELITE>

#include <EASTL/fixed_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

#include <EASTL/sort.h>

#include "common.hpp"

using namespace eastl;

static const unsigned N_ELE_SMALL = 10;
static const unsigned N_ELE_LARGE = 40;

// used for value entitiy of map.
struct FuncDef {
    void (*func)();
    const char *msg_help;
};
using tmap = fixed_map<char, FuncDef, N_ELE_SMALL>;
using tmap_large = fixed_map<char, FuncDef, N_ELE_LARGE>;

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
        Serial << crlf << format(" %c : %s", x, v_map[x].msg_help);
#if 0
        // if there is possibility that key is not found in map container.
        auto&&e = v_map.find(x);
        if (e != v_map.end()) {
            Serial << crlf << format(" %c : %s", x, e->second.msg_help);
        }
#endif
    }
}


/*** the setup procedure (called on boot) */
void setup() {
    // create map table for keybind
    mwx::pnew(v_map);
    mwx::pnew(v_key);

    // alias to m
    auto& m = v_map;
    auto& v = v_key;

    // add keybind (allows put all entries as an initializer_list argument.)
    m.insert({
        { '1', {test_fixed_string,       "fixed_string"} },
        { '2', {test_fixed_set,          "fixed_set"} },
        { '3', {test_fixed_list,         "fixed_list"} },
        { '4', {test_fixed_slist,        "fixed_slist"} },
        { '5', {test_fixed_vector,       "fixed_vector"} },
        { '6', {test_intrusive_list,     "intrusive_list"} },
        { '7', {test_intrusive_hash_set, "intrusive_hash_set"} },
    });

    // put key into v_key
    for(auto&&x : m) {
        v.push_back(x.first);
    }
    sort(v.begin(), v.end(), eastl::greater<char>()); // sort in reverse order

    // show help
    help();

    // show size info
    Serial << crlf << crlf << "*** fixed_map is used for keybindings ***";
    int bytes_per_entry =  (sizeof(tmap_large)-sizeof(tmap)) / (N_ELE_LARGE - N_ELE_SMALL);
    int bytes_header = sizeof(tmap) - N_ELE_SMALL*bytes_per_entry;
    Serial << crlf << format("SIZE: ele=%dby, header=%dby, %dby/entry", sizeof(char) + sizeof(FuncDef), bytes_header, bytes_per_entry);
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
            Serial << "*** " << i->second.msg_help << " ***" << crlf;

            // execute test!
            i->second.func();
        } else switch(c) {
            case 0xd: help(); break;
            case '!': the_twelite.reset_system(); break;
            default: break;
        }
    }
}
