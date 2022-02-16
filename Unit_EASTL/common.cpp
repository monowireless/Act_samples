#include <TWELITE>

#ifdef JN516x
/* check HEAP head pointer */
extern "C" uint32_t u32HeapStart; 
void check_heap(bool init=false) {
    static uint32_t u32HeapAdrLast = 0;
    if (init && u32HeapAdrLast == 0) u32HeapAdrLast = u32HeapStart;

    Serial << '<' << format("%d", u32HeapStart - u32HeapAdrLast) << '>';
    u32HeapAdrLast = u32HeapStart;
}
#else     
void check_heap() {
}
#endif