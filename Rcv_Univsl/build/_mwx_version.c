#include <stdint.h>
extern uint32_t INTRCT_USER_APP_ID;
void MWX_Set_User_App_Ver() {
  INTRCT_USER_APP_ID = (VERSION_MAIN << 16) | (VERSION_SUB << 8) | VERSION_VAR; }
