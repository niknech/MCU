#include "rc_status.h"

void RC_Update(RC_Struct_t* rc)
{
  rc->voltage = (float) rc->voltage_raw / 4096.f * 3.3f * 4.98f;
  rc->current = ((float) rc->current_raw / 4096.f * 3.3f) / 10.f;
}

void RC_PrintStatus(RC_Struct_t* rc)
{

}
