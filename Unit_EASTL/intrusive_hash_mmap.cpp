#include <TWELITE>

#include <EASTL/utility.h>
#include <EASTL/intrusive_hash_map.h>
#include <EASTL/fixed_vector.h>

using namespace eastl;

/// THE KEY IS IN NUMERIC
typedef char HKEY; // hash key

class wid : public intrusive_hash_node_key<HKEY>
{
    using SUP = intrusive_hash_node_key; // alias
public:
    int _value;
    wid(HKEY key = 0, uint32_t value = 0) : _value(value)  { SUP::mKey = key; }
};

/// CODE EXAMPLE IF KEY IS NON-VALUE CLASS
// some defintions are required:
// - compare operator == between keys.
// - calculate the unique number from key class.

// the key : 
//  - default constructor is required.
struct HKC {
    int _value;
    HKC(int v = 0) : _value(v) {}
};
// compare operator == between keys:
bool operator == (const HKC& lhs, const HKC& rhs) { return lhs._value == rhs._value; }


// the unique number from key class :
//   - this value will be used to calculate a hash value.
struct HKC_Hash {
    size_t operator()(const HKC& hkc) const { return (size_t)hkc._value; }
};

// the node:
class widC : public intrusive_hash_node_key<HKC>
{
    using SUP = intrusive_hash_node_key; // alias
public:
    int _value;
    widC(HKC key = {0}, uint32_t value = 0) : _value(value) { SUP::mKey = key; }
};

/// TEST CODES
void test_instrusive_hash_mmap() {
    Serial << crlf << "--- NUMERIC KEY ---";
    {
        const unsigned N = 8;
        fixed_vector<wid, N, false> v;
        intrusive_hash_multimap<HKEY, wid, 7> m;
        v.resize(N);

        v[0].mKey = 'a'; v[0]._value = 0;
        v[1].mKey = 'b'; v[1]._value = 1;
        v[2].mKey = 'c'; v[2]._value = 2;
        v[3].mKey = 'b'; v[3]._value = 3;
        v[4].mKey = 'd'; v[4]._value = 4;
        v[5].mKey = 'e'; v[5]._value = 5;
        v[6].mKey = 'b'; v[6]._value = 6;
        v[7].mKey = 'f'; v[7]._value = 7;

        Serial << crlf << format("SIZE: vect=%d mmap=%d", sizeof(v), sizeof(m));

        Serial << crlf << "Vect: ";
        for(int i = 0; i < N; i++) {
            if (!(i & 0x3)) Serial << crlf;
            Serial << format(" v[%d]=(%c,%d)", i, v[i].mKey, v[i]._value);
            
            m.insert(v[i]);
        }

        Serial << crlf << "MMAP: ";
        for (auto &&x : m) {
            Serial << '(' << x.mKey << ':' << x._value << ')';
        }

        {
            char k = 'b';
            
            Serial << crlf << format("'%c'->%d:", k, m.count(k));
            auto p = m.equal_range(k);

            for(auto&& q = p.first; q != p.second; q++) {
                Serial << " (" << q->mKey << ',' << q->_value << ')';
            }
        }

        m.remove(v[3]);
        Serial << crlf << "Remove v[3]: ";
        for (auto &&x : m) {
            Serial << '(' << x.mKey << ':' << x._value << ')';
        }

        Serial << crlf << "Remove odd: ";
        for (auto &&x : m) {
            if (x._value & 1) m.remove(x);
        }
        for (auto &&x : m) {
            Serial << '(' << x.mKey << ':' << x._value << ')';
        }
    }

    Serial << crlf << "--- CUSTOM KEY ---";
    {
        widC a(HKC(1), 'a');
        widC b(HKC(2), 'b');
        widC c(HKC(1), 'c');
        widC d(HKC(4), 'd');
        widC e(HKC(5), 'e');

        intrusive_hash_multimap<HKC, widC, 7, HKC_Hash> m;

        m.insert(a);
        m.insert(b);
        m.insert(c);
        m.insert(d);
        m.insert(e);

        Serial << crlf << "Inserted Items:";

        for(auto&& x : m) {
            Serial << '(' << int(x.mKey._value) << ',' << char(x._value) << ')';
        }

        Serial << crlf << format("Count of HKC(1): %d", m.count(HKC(1)));
    }
}
