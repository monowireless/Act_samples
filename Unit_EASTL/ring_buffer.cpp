/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#include <TWELITE>
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

using namespace eastl;

using mystr = basic_string<char>;

ring_buffer< mystr, vector<mystr> > debugLogText;


#ifdef JN516x
extern "C" uint32_t u32HeapStart; 
void check_heap(bool init=false) {
    static uint32_t u32HeapAdrLast;
    if (init) u32HeapAdrLast = u32HeapStart;

    Serial << '<' << format("%d", u32HeapStart - u32HeapAdrLast) << '>';
    u32HeapAdrLast = u32HeapStart;
}
#else     
void check_heap() {
}
#endif

void test_ring_buffer() {
    check_heap(false); // init internal vars

    Serial << crlf << format("Check single str(sizeof(mystr)=%d) alloc:", sizeof(mystr));
    mystr tmp;
    check_heap(); // here, no heap allocation.

    Serial << crlf << "Create ringbuffer:";
    mwx::pnew(debugLogText, 16); // 204bytes allocated in HEAP (12bytes for ring_buffer, 12x16bytes for vector<string> instance with empty string)
    check_heap();

    Serial << crlf << "Allocate Strings:";

    vector<mystr>::iterator it = debugLogText.get_container().begin();
    vector<mystr>::iterator itEnd = itEnd = debugLogText.get_container().end();
    for(; it != itEnd; ++it) {
        Serial << '{';
        (*it).reserve(32);
        check_heap();
        Serial << '}';
    }

    debugLogText.push_front() = "hello,";
    debugLogText.push_front() = "I";
    debugLogText.push_front() = "am";
    debugLogText.push_front() = "a";
    debugLogText.push_front() = "super";
    debugLogText.push_front() = "hero!";
    
    Serial << crlf;
    while(!debugLogText.empty()) {
        Serial << debugLogText.back().c_str() << ' ';
        debugLogText.pop_back();
    }
    check_heap(); // no heap allocation from putting string data by push_front() to displaying data by back(), pop_back().
}