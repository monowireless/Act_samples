/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/**
 * intrusive コンテナの特徴：
 * 
 * - コンテナ要素にリンクを維持するためのポインタを保持します。
 *   eastl::intrusive_list_node を継承します。
 * - 通常のコンテナではオブジェクトのメモリ管理を行いますが、
 *   intrusive コンテナには、各要素そのものが格納されたように見えます。
 *   実際にはリンクを維持するポインタの操作によりコンテナに格納された
 *   ように見えます。
 * - コンテナ要素オブジェクトのメモリ管理はコンテナとは独立して行います。
 *   例えばスタック領域で確保したオブジェクトを格納したintrusiveコンテナ
 *   をスコープ外で参照すると破壊的な結果となります。
 * - メモリ効率の高いコンテナです。
 */


#include <TWELITE>
#include <EASTL/intrusive_list.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/linked_ptr.h>

using namespace eastl;

// the sizeof(IntNode) will be 12bytes
struct IntNode : public eastl::intrusive_list_node { 
    int mX;
    IntNode(int x = 0) : mX(x) { }
        // no need to call super class's constructor eastl::intrusive_list_node()
};

// define operators for sorting
inline bool operator==(const IntNode& a, const IntNode& b) { return a.mX == b.mX; }
inline bool operator<(const IntNode& a, const IntNode& b) { return a.mX < b.mX; }
inline bool operator>(const IntNode& a, const IntNode& b) { return a.mX > b.mX; }

using tilist = intrusive_list<IntNode>;

void print_node(IntNode& n) {
    Serial << format("(%d)", n.mX, (void*)&n, sizeof(IntNode));

#if 0
    Serial << format("node(%d)@%08x,%dby", n.mX, (void*)&n, sizeof(IntNode));

    // dump node memory information
    //   the member data in eastl::intrusive_list_node is not necessary to
    //   be initialized when constructed.
    uint8_t*p = (uint8_t*)&n;
    Serial << '<';
    for (size_t i; i < sizeof(IntNode); i++) {
        Serial << format("%02x", *p++);
        Serial.flush();
    }
    Serial << '>';
#endif
}

void print_list(tilist& l, const char*msg="") {
    Serial << crlf << '!' << msg << ':';
    for(auto&&x : l) {
        Serial << ' ';
        print_node(x);
    }
}

IntNode nodeA;
IntNode nodeB;
IntNode nodeC;
IntNode nodeD;
IntNode nodeE;

tilist intList;
void test_intrusive_list() {
    Serial << format("SIZE: tilist=%d IntNode=%d", sizeof(tilist), sizeof(IntNode));

    // construct global boject
    mwx::pnew(nodeA, 5);
    mwx::pnew(nodeB, 1);
    mwx::pnew(nodeC, 4);
    mwx::pnew(nodeD, 9);
    mwx::pnew(nodeE, 2);

    mwx::pnew(intList);

    auto& v = intList; // alias

    print_list(v, "init");
    v.push_front(nodeA);
    print_list(v, "push_back(nodeA)");
    v.push_front(nodeB);
    print_list(v, "push_back(nodeB)");
    v.remove(nodeA);
    print_list(v, "remove(nodeA)");

    v.push_front(nodeC);
    print_list(v, "push_back(nodeC)");
    v.push_front(nodeD);
    print_list(v, "push_back(nodeD)");
    v.push_front(nodeE);
    print_list(v, "push_back(nodeE)");

    v.sort();
    print_list(v, "sorted");

    v.sort(eastl::greater<tilist::value_type>());
    print_list(v, "sorted reverse");
}