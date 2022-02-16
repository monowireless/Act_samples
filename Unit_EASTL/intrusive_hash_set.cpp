/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/* Note: this example refers the followings:
 *  - eastl::intrusive_hash_set     : set container (intrustive hash)
 *  - eastl::intrusive_hash_node    : base class holding link information to form list.
 */

#include <TWELITE>
#include <EASTL/internal/intrusive_hashtable.h>
#include <EASTL/intrusive_hash_set.h>
#include <EASTL/intrusive_hash_map.h>
#include <EABase/eabase.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>

using namespace eastl;

struct SetWidget : public intrusive_hash_node
{
    SetWidget(int x = 0)
        : mX(x) { }
    int mX;
};

struct SWHash
{
    size_t operator()(const SetWidget& sw) const
    {
        return (size_t)sw.mX;
    }
};

template class eastl::intrusive_hash_set<SetWidget, 37, SWHash>;

// necessary for hash set.
inline bool operator==(const SetWidget& a, const SetWidget& b)
   { return a.mX == b.mX; }
    
void test_intrusive_hash_set() {
    const size_t kBucketCount = 7; // should be primer number
	typedef intrusive_hash_set<SetWidget, kBucketCount, SWHash> IHM_SW;

    // the container size (cnt=...) will vary by kBucketCount (more bucket, more memory, less collision in hash)
    Serial << crlf << format("SIZE: ele=%d cnt=%d(buckt:%d)", sizeof(SetWidget), sizeof(IHM_SW), kBucketCount);

    SetWidget nodeA(5);
    SetWidget nodeB(3);
    SetWidget nodeC(2);
    SetWidget nodeD(4);
    SetWidget nodeE(1);

    IHM_SW v;

    Serial << crlf << "Perform insert Operation.";
    v.insert(nodeA);
    v.insert(nodeB);
    v.insert(nodeC);
    v.insert(nodeD);
    v.insert(nodeE);
    
    Serial << crlf << "Show all entries.";
    for (auto &x : v) {
        Serial << crlf << int(x.mX) << ':';
    }
    
    int key = 3;
    auto it = v.find(key);
    Serial << crlf << ((it != v.end()) ? "found " : "not found ") << key;

    Serial << crlf << "Erase an entry.";
    v.erase(it);
    Serial << crlf << "Show all entries.";
    for (auto &x : v) {
        Serial << crlf << int(x.mX) << ':';
    }
    
    key = 4;
    it = v.find(key);
    Serial << crlf << ((it != v.end()) ? "found " : "not found ") << key;
}