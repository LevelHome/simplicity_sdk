#include <em_device.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(_SILICON_LABS_32B_SERIES_2)
#include <em_rmu.h>

bool sl_btctrl_hci_reset_reason_is_sys_reset(void)
{
  uint32_t reset_cause;
  reset_cause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();
  return reset_cause == EMU_RSTCAUSE_SYSREQ;
}

#elif defined(_SILICON_LABS_32B_SERIES_3)
#include <sl_hal_emu.h>

bool sl_btctrl_hci_reset_reason_is_sys_reset(void)
{
  uint32_t reset_cause;
  reset_cause = sl_hal_emu_get_reset_cause();
  sl_hal_emu_clear_reset_cause();
  return reset_cause == EMU_RSTCAUSE_SYSREQ;
}

#else
#error "Unsupported platform"
#endif

void sl_btctrl_hci_reset(void)
{
  CORE_ResetSystem();
}
