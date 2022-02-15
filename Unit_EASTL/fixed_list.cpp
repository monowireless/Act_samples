#include <TWELITE>
#include <EASTL/fixed_list.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

/* Note: 
 * The fixed_list makes list structure in linear memory model with limited number of
 * entries. The insert operation takes O(n) order.
 * 
 * If you want to allocate an object other than unique_ptr<> (e.g. in stack or global),
 * see the example of `fixed_string'.
 */

using namespace eastl;
using tdata = pair<uint8_t, void (*)()>; // data is pair of uint8_t and function ptr.
using tlst3 = fixed_list<tdata, 3, false>; // list data
using tlst30 = fixed_list<tdata, 30, false>; // list data
static unique_ptr<tlst3> uq_lst1;

void func0() { Serial << "func0"; }
void func1() { Serial << "func1"; }
void func2() { Serial << "func2"; }
void func3() { Serial << "func3"; }

void test_fixed_list() {
    Serial << "\e[H\e[2J";
    delay(50); // wait for screen clean up
    Serial << "*** fixed_list ***";

    Serial << crlf << format("sizeof tdata = %d", sizeof(tdata));
    Serial << crlf << format("sizeof tlst3 = %d", sizeof(tlst3));
    Serial << crlf << format("sizeof tlst30 = %d", sizeof(tlst30));

    int bytes_per_entry =  (sizeof(tlst30)-sizeof(tlst3)) / 27;
    int bytes_header = sizeof(tlst3) - 3*bytes_per_entry;
    Serial << crlf << format("header=%d, %dbytes/entry", bytes_header, bytes_per_entry);

    uq_lst1.reset(new tlst3()); // create own object (reserved in HEAP area)
    Serial << crlf << format("The fixed_list object created at %08x."
            , (void*)&(*uq_lst1), sizeof(tlst3));

    Serial << crlf << "Perform insert Operation.";
    if(!uq_lst1->full()) uq_lst1->insert(uq_lst1->begin(), eastl::make_pair('0', func0));
    if(!uq_lst1->full()) uq_lst1->insert(uq_lst1->begin(), eastl::make_pair('1', func1));
    if(!uq_lst1->full()) uq_lst1->insert(uq_lst1->begin(), eastl::make_pair('2', func2));
    if(!uq_lst1->full()) uq_lst1->insert(uq_lst1->begin(), eastl::make_pair('3', func3));

    Serial << crlf << "Show all entries.";
    for (auto &x : *uq_lst1) {
        Serial << crlf << x.first << ':';
        x.second(); // call the function stored in the second.
    }

    Serial << crlf << "Find entry and erase it.";
    for(auto p = uq_lst1->begin(); p != uq_lst1->end(); p++) {
        if (p->first == '1') {
            Serial << crlf << format("..found %c:func#%08x", p->first, p->second);

            uq_lst1->erase(p);
            Serial << crlf << "..erased";
            break;
        }
    }

    Serial << crlf << "Show all entries.";
    for (auto &x : *uq_lst1) {
        Serial << crlf << x.first << ':';
        x.second(); // call the function stored in the second.
    }
}