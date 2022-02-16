/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::vector_multimap
 *  - eastl::vector_map
 */

#include <TWELITE>
#include <EASTL/vector_map.h>
#include <EASTL/vector_multimap.h>

#include "common.hpp"

using namespace eastl;

/*
 * vector map, multimap stores key information sorted into linear vector.
 * therefore, inserting cost is much bigger.
 */

void test_vector_multimap() {
    {
        // construction : note: this will make memory leak.
        vector_multimap<char, int> m;
        Serial << crlf << format("vector_multimap (sizeof=%d):", sizeof(m));
        check_heap();
        Serial << crlf << "reserve(10): ";
        m.reserve(10);
        check_heap();

        // insert items
        Serial << crlf << "emplace items: ";
        m.emplace('c', 30);
        m.emplace('a', 10);
        m.emplace('b', 20);
        m.emplace('a', 40); // key 'a' has two or more items.
        m.emplace('d', 20);
        m.emplace('a', 60);
        check_heap();

        Serial << crlf;

        // query mumber of items with key 'a'
        auto count = m.count('a'); // count == 2
        Serial << "..count = " << int(count) << crlf;

        // Enumerate values with key 'a'
        Serial << "..";
        auto p = m.equal_range('a');
        for (auto it = p.first; it != p.second; ++it) {
            Serial << int(it->second) << ' ';
        }
        check_heap();
    }

    // check mem size of vector_map()
    {
        vector_map<char, int> m;
        Serial << crlf << format("vector_map (sizeof=%d):", sizeof(m));
        m.reserve(10);
        check_heap();
        
        // insert items
        Serial << crlf << "emplace items: ";
        m.insert({
            {'c', 30},
            {'a', 10},
            {'b', 20},
            {'z', 40},
            {'d', 20},
            {'k', 60},
        });
        /* 
        m.emplace('c', 30);
        m.emplace('a', 10);
        m.emplace('b', 20);
        m.emplace('z', 40); // key 'a' has two or more items.
        m.emplace('d', 20);
        m.emplace('k', 60);
        */
        Serial << crlf << "..";
        for(auto&& x: m) Serial << char(x.first) << '/' << int(x.second) << ' ';
        check_heap();

        // find 'a'
        auto&& x = m.find('a');
        if(x != m.end()) {
            Serial << crlf << format("..found %c->%d", x->first, x->second);
        }
    }
}