#include "init.h"

#include "address.h"
#include "datalink.h"


void init_bacnet() {
  /* setup my info */
  Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
  address_init();
  dlenv_init();
  atexit(datalink_cleanup);
}