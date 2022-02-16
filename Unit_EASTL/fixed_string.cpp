/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::fixed_string          : string class on fixed memory.
 *  - eastl::basic_string          : string class with dynamic allocation (more flexible, can use with substring)
 *  - eastl::fixed_substring       : substring class
 */

#include <TWELITE>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_substring.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

using namespace eastl;
using tstr128 = fixed_string<char, 127 + 1, false>;

static tstr128 *fs1;
static tstr128 s_str1;
static unique_ptr<tstr128> uq_str1;

template <typename T>
void print_str(const char*msg, T& str) {
    Serial << crlf;
    Serial << format("\e[31m%s\e[0m---", str.c_str());
    Serial << format("\e[32m%s\e[0m", msg);
    Serial << format(",ad=%08x,sz=%d/%d", (uint32)(void*)str.c_str(), str.size(), str.capacity());
}

/*** the setup procedure (called on boot) */
void test_fixed_string() {
    Serial << crlf << format("SIZE: tstr128=%dby, header=%dby", sizeof(tstr128), sizeof(tstr128) - 128);
    
    tstr128 fs0("hello world"); // Can hold up to a strlen of 128.
    print_str("fs0 as local", fs0);
    
    fs0 = "hola mundo";
    print_str("fs0 = ...", fs0);
    
    fs0.clear();
    print_str("fs0.clear()", fs0);
    
    // new the obj (reserved at heap)
    fs1 = new tstr128("hello");
    print_str("new fs1", *fs1);
    
    // new the obj2 (reserved at heap)
    auto fs2 = new tstr128("hello2");
    print_str("new fs2", *fs2);

    // using placement new (reserved at gloal area)
    mwx::pnew(s_str1, "hello3");
    print_str("global s_str1", s_str1);

    // using unique ptr (reserved at heap)
    if (!uq_str1) uq_str1.reset(new tstr128("hello world"));
    print_str("unique_ptr uq_str1", *uq_str1);

    // test basic_strin and substring
    Serial << crlf;
    fixed_substring<char> sstr1(uq_str1->c_str(), 2, 5); // construct substr from char*
    basic_string<char> bstr1("", 128); // construct basic_string<char> of 128bytes buffer, allocated in HEAP.
    Serial << crlf << format("<ad=%08x siz=%d/%d>", (void*)bstr1.data(), bstr1.size(), bstr1.capacity());
    bstr1.resize(0); // trunk to 0byte.
    bstr1 = "hello"; // set "hello"
    Serial << crlf << format("<ad=%08x siz=%d/%d>", (void*)bstr1.data(), bstr1.size(), bstr1.capacity());
    bstr1 += " world! + sub=" + sstr1; // add "world..."
    Serial << crlf << format("<ad=%08x siz=%d/%d>", (void*)bstr1.data(), bstr1.size(), bstr1.capacity());
    Serial << crlf << "BUFF(iter):";
    for(auto x: bstr1) Serial.putchar(x); // output whole data
    Serial << crlf << "BUFF(cstr):" << bstr1.c_str(); // output via c_str()
    Serial << crlf << format("<ad=%08x siz=%d/%d>", (void*)bstr1.data(), bstr1.size(), bstr1.capacity());
    Serial << crlf << format("bstr1@%08x, c_str()->%08x", (void*)bstr1.data(), bstr1.c_str());
            // check the c_str() address, should be same with bstr1.data(). (no allocation newly)
}
