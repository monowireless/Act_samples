#include <TWELITE>

#ifdef JN516x
extern "C" uint32_t u32HeapStart; 
static uint32_t Get_u32HeapStart() {
	return u32HeapStart;
}
#else
static uint32_t Get_u32HeapStart() {
	int *p = new int;
	uint32_t addr = (uint32_t)p;
	delete p;
	return addr;
}
#endif

size_t check_heap(bool init=false) {
    static uint32_t u32HeapAdrLast = 0;
    if (init && u32HeapAdrLast == 0) {
        u32HeapAdrLast = Get_u32HeapStart();
        return 0;
    }

    uint32_t start = Get_u32HeapStart();
    size_t d = start - u32HeapAdrLast;
    Serial << '<' << format("%d", d) << '>';
    u32HeapAdrLast = start;

    return d;
}
