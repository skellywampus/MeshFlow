// Compile-time requirements for the M9 automatic-idle-sleep integration.
#if defined(MESHFLOW_M9_AUTO_LIGHT_SLEEP) && defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "sdkconfig.h"
#include "esp_pm.h"
#ifndef CONFIG_PM_ENABLE
#error "M9 automatic light sleep requires rebuilt PM-enabled SDK libraries"
#endif
#ifndef CONFIG_FREERTOS_USE_TICKLESS_IDLE
#error "M9 automatic light sleep requires FreeRTOS tickless idle"
#endif
#ifndef CONFIG_PM_LIGHT_SLEEP_CALLBACKS
#error "M9 radio wake coordination requires PM callbacks"
#endif
#if CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP || CONFIG_PM_SLP_DISABLE_GPIO
#error "M9 GPIO42 radio wake requires the digital GPIO peripheral to remain available"
#endif
#if !CONFIG_BT_CTRL_MODEM_SLEEP || !CONFIG_BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP
#error "M9 BLE preservation requires controller modem sleep and retained main XTAL"
#endif
static_assert(CONFIG_PM_ENABLE == 1 && CONFIG_FREERTOS_USE_TICKLESS_IDLE == 1,
              "SDK settings must enable PM and tickless idle together");
#endif
