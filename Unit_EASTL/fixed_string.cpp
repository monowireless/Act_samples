#include <TWELITE>
#include <EASTL/fixed_string.h>
#include <EASTL/unique_ptr.h>

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
    Serial << "\e[H\e[2J";
    delay(50); // wait for screen clean up
    Serial << "*** fixed_string ***";

    Serial << crlf << format("sizeof(tstr128)=%d, header=%d", sizeof(tstr128), sizeof(tstr128) - 128);
    
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
    uq_str1.reset(new tstr128("hello4"));
    print_str("unique_ptr uq_str1", *uq_str1);

}
