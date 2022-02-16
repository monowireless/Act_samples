#include <TWELITE>

#ifdef JN516x
/* check HEAP head pointer */
extern "C" uint32_t u32HeapStart; 
size_t check_heap(bool init=false) {
    static uint32_t u32HeapAdrLast = 0;
    if (init && u32HeapAdrLast == 0) {
        u32HeapAdrLast = u32HeapStart;
        return 0;
    }

    size_t d = u32HeapStart - u32HeapAdrLast;
    Serial << '<' << format("%d", d) << '>';
    u32HeapAdrLast = u32HeapStart;

    return d;
}
#else     
void check_heap() {
}
#endif