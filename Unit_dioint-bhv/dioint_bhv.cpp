// mwx header
#include "dioint_bhv.hpp"

/*****************************************************************/
// MUST DEFINE CLASS NAME HERE
#define __MWX_APP_CLASS_NAME MyApp
#include "_mwx_cbs_cpphead.hpp"
/*****************************************************************/
const uint8_t PIN_DO1 = 16;

/***
 * Timer 0 のハンドラー (Timer0 を begin()したとき)
 */
/* 使ってないのでコメントアウト
MWX_TIMER_INT(0, uint32_t arg, uint8_t& handled) {
}
*/


/***
 * 割り込みハンドラ (PIN_DI1)
 */
MWX_DIO_INT(BRD_APPTWELITE::PIN_DI1, uint32_t arg, uint8_t& handled) {
    Serial << '1';
    handled = false;
}

/***
 * イベント (PIN_DI1)
 */
MWX_DIO_EVENT(BRD_APPTWELITE::PIN_DI1, uint32_t arg) {
}

/***
 * 割り込みハンドラ (PIN_DI2)
 */
MWX_DIO_INT(BRD_APPTWELITE::PIN_DI2, uint32_t arg, uint8_t& handled) {
    Serial << '2';
    handled = true; // no event happen
}

#if 0
/***
 * イベント (PIN_DI2)
 */
MWX_DIO_EVENT(BRD_APPTWELITE::PIN_DI2, uint32_t arg) {
}
#endif


/*****************************************************************/
// common procedure (DO NOT REMOVE)
#include "_mwx_cbs_cpptail.cpp"
// MUST UNDEF CLASS NAME HERE
#undef __MWX_APP_CLASS_NAME
/*****************************************************************/