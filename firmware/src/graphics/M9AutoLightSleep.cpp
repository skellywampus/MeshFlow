#if defined(MESHFLOW_M9_AUTO_LIGHT_SLEEP) && defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "configuration.h"
#include "graphics/common/M9AutoLightSleep.h"
#include "graphics/common/M9SleepDecision.h"
#include "graphics/common/M9PowerGuards.h"
#include "Power.h"
#include "main.h"
#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"
#include "mesh/RadioLibInterface.h"
#include "mesh/NodeDB.h"
#include "esp_pm.h"
#include "esp_private/pm_impl.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern "C" void meshflow_m9_keyboard_wake();
namespace {
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
StaticSemaphore_t policyStorage;
SemaphoreHandle_t policyMutex = xSemaphoreCreateMutexStatic(&policyStorage);
esp_pm_lock_handle_t gate = nullptr, cpu = nullptr, bus = nullptr;
M9SleepStats stats{};
bool ready = false, gateHeld = true, cpuHeld = false, radioArmed = false;
bool pinsArmed = false, gpsSeen = false;
bool wifiSleepReady = false;
bool powerWakePending = false;
uint32_t audioUsers = 0;
esp_err_t wakeConfigError = ESP_OK;
M9PowerWakeGuard powerWake;
struct PowerWakeGPIO {
    static constexpr gpio_num_t pin = (gpio_num_t)EXT_PWR_DETECT;
    M9WakePinState snapshot() {
        return {GPIO.pin[pin].int_type, GPIO.pin[pin].int_ena, bool(GPIO.pin[pin].wakeup_enable)};
    }
    bool powered() { return gpio_get_level(pin) == EXT_PWR_DETECT_VALUE; }
    void mask() { gpio_intr_disable(pin); }
    bool armWake() { return gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL) == ESP_OK; }
    bool restore(const M9WakePinState &saved) {
        esp_err_t err = gpio_wakeup_disable(pin);
        if (gpio_set_intr_type(pin, (gpio_int_type_t)saved.type) != ESP_OK) err=ESP_FAIL;
        if (saved.wake && gpio_wakeup_enable(pin, (gpio_int_type_t)saved.type) != ESP_OK) err=ESP_FAIL;
        // Clear the old level latch while masked. Reconcile a missed transition
        // in the normal power worker, never by repeatedly firing the old ISR.
        gpio_ll_clear_intr_status(&GPIO, 1UL << EXT_PWR_DETECT);
        if (err == ESP_OK) GPIO.pin[pin].int_ena = saved.interrupts;
        return err == ESP_OK;
    }
} powerGPIO;
uint32_t configuredMax = 0;
uint64_t eligibleAt = 0;
void account() {
    uint64_t now = esp_timer_get_time();
    if (eligibleAt) stats.eligibleUs += now-eligibleAt;
    eligibleAt = ready && stats.screenOff && !stats.gpsOn && (!stats.wifiOn || wifiSleepReady) && !stats.usbOn && !audioUsers && !moduleConfig.serial.enabled ? now : 0;
}
// Called only from normal task context, with policyMutex held; never hold mux
// while entering an IDF PM API (the idle path takes those locks in reverse order).
void apply() {
    bool hold = !stats.screenOff || stats.gpsOn || (stats.wifiOn && !wifiSleepReady) || stats.usbOn || audioUsers || !ready;
    if (hold && !gateHeld) { esp_pm_lock_acquire(gate); gateHeld = true; }
    uint32_t mhz = !stats.screenOff || stats.wifiOn || audioUsers ? 240 : 80;
    if (configuredMax != mhz) {
        esp_pm_config_t cfg{};
        cfg.min_freq_mhz = 80; cfg.max_freq_mhz = mhz; cfg.light_sleep_enable = ready;
        esp_err_t err=esp_pm_configure(&cfg);
        if (err != ESP_OK) {
            if (!gateHeld) {esp_pm_lock_acquire(gate);gateHeld=true;}
            portENTER_CRITICAL(&mux); stats.error=err; ready=false; stats.initialized=0; account(); portEXIT_CRITICAL(&mux);
            return;
        }
        configuredMax=mhz;
    }
    bool fast=!stats.screenOff || stats.wifiOn || audioUsers;
    if (fast && !cpuHeld) {esp_pm_lock_acquire(cpu);cpuHeld=true;}
    if (!fast && cpuHeld) {esp_pm_lock_release(cpu);cpuHeld=false;}
    if (!hold && gateHeld) {esp_pm_lock_release(gate);gateHeld=false;}
}
bool IRAM_ATTR skipSleep() {
    portENTER_CRITICAL_ISR(&mux);
    bool skip=audioUsers || wakeConfigError != ESP_OK || m9SleepBlocks({ready, bool(stats.screenOff), bool(stats.gpsOn), bool(stats.wifiOn) && !wifiSleepReady, stats.busUsers!=0, radioArmed, gpio_get_level((gpio_num_t)EXT_PWR_DETECT)==EXT_PWR_DETECT_VALUE, moduleConfig.serial.enabled})!=0;
    bool pending=!skip && gpio_get_level((gpio_num_t)LR1110_IRQ_PIN);
    if (pending) {++stats.pendingSkips;skip=true;}
    portEXIT_CRITICAL_ISR(&mux);
    // A missed edge must not wait for a periodic poll while vetoing sleep.
    if (pending && RadioLibInterface::instance) RadioLibInterface::instance->notifyLightSleepIrq();
    return skip;
}
esp_err_t IRAM_ATTR enterSleep(int64_t, void *) {
    // The veto is skipSleep, not the entry callback's return value: IDF 5.5
    // logs entry-callback errors but does not use them to cancel the sleep.
    pinsArmed=true;
    if (!powerWake.arm(powerGPIO)) wakeConfigError=ESP_FAIL;
    gpio_intr_disable((gpio_num_t)LR1110_IRQ_PIN);
    gpio_intr_disable((gpio_num_t)KB_INT);
    if (gpio_wakeup_enable((gpio_num_t)LR1110_IRQ_PIN,GPIO_INTR_HIGH_LEVEL)!=ESP_OK) wakeConfigError=ESP_FAIL;
    if (gpio_wakeup_enable((gpio_num_t)KB_INT,GPIO_INTR_HIGH_LEVEL)!=ESP_OK) wakeConfigError=ESP_FAIL;
    return ESP_OK;
}
esp_err_t IRAM_ATTR exitSleep(int64_t, void *) {
    if (!pinsArmed) return ESP_OK;
    pinsArmed=false;
    bool radioPending=gpio_get_level((gpio_num_t)LR1110_IRQ_PIN);
    bool keyPending=gpio_get_level((gpio_num_t)KB_INT);
    gpio_wakeup_disable((gpio_num_t)LR1110_IRQ_PIN);
    gpio_wakeup_disable((gpio_num_t)KB_INT);
    powerWake.restore(powerGPIO);
    if (powerWake.faulted()) wakeConfigError=ESP_FAIL;
    portENTER_CRITICAL_ISR(&mux);
    powerWakePending = powerWake.takeChanged() || powerWakePending;
    if (wakeConfigError != ESP_OK) stats.error=wakeConfigError;
    portEXIT_CRITICAL_ISR(&mux);
    gpio_set_intr_type((gpio_num_t)LR1110_IRQ_PIN,GPIO_INTR_POSEDGE);
    gpio_set_intr_type((gpio_num_t)KB_INT,GPIO_INTR_POSEDGE);
    portENTER_CRITICAL_ISR(&mux);bool attached=radioArmed;portEXIT_CRITICAL_ISR(&mux);
    if(attached)gpio_intr_enable((gpio_num_t)LR1110_IRQ_PIN);
    gpio_intr_enable((gpio_num_t)KB_INT);
    // Recheck after rearming: an edge may have arrived while CPU IRQs were masked.
    // The shared claim makes a replay and the ordinary ISR mutually exclusive.
    if ((radioPending || gpio_get_level((gpio_num_t)LR1110_IRQ_PIN)) && RadioLibInterface::instance)
        RadioLibInterface::instance->notifyLightSleepIrq();
    // Do not infer a key from a generic GPIO wake (radio/USB can also wake).
    // If a short wake pulse has ended, the existing 20 ms UI poll still drains
    // the controller's latched event and consumes the first press for display wake.
    if (keyPending || gpio_get_level((gpio_num_t)KB_INT))
        meshflow_m9_keyboard_wake();
    return ESP_OK;
}
bool initialize() {
    if (!gpsSeen) stats.gpsOn=gpio_get_level((gpio_num_t)PIN_GPS_EN)==GPS_EN_ACTIVE;
    stats.usbOn=gpio_get_level((gpio_num_t)EXT_PWR_DETECT)==EXT_PWR_DETECT_VALUE;
    esp_err_t err=esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,0,"m9Policy",&gate);
    if (err==ESP_OK) err=esp_pm_lock_acquire(gate);
    if (err==ESP_OK) err=esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX,0,"m9UI",&cpu);
    esp_pm_lock_handle_t createdBus=nullptr;
    if (err==ESP_OK) err=esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,0,"m9SPI",&createdBus);
    portENTER_CRITICAL(&mux);bus=createdBus;portEXIT_CRITICAL(&mux);
    if (err==ESP_OK) err=esp_sleep_enable_gpio_wakeup();
    if (err==ESP_OK) err=esp_pm_register_skip_light_sleep_callback(skipSleep);
    esp_pm_sleep_cbs_register_config_t cb{};cb.enter_cb=enterSleep;cb.exit_cb=exitSleep;
    if (err==ESP_OK) err=esp_pm_light_sleep_register_cbs(&cb);
    portENTER_CRITICAL(&mux);stats.error=err;ready=err==ESP_OK;stats.initialized=ready;account();portEXIT_CRITICAL(&mux);
    return ready;
}
}
extern "C" void meshflow_m9_wifi_sleep_ready(bool enabled) {
    xSemaphoreTake(policyMutex,portMAX_DELAY);
    portENTER_CRITICAL(&mux);account();wifiSleepReady=enabled;account();portEXIT_CRITICAL(&mux);
    if (gate && cpu && bus) apply();
    xSemaphoreGive(policyMutex);
}
extern "C" void meshflow_m9_pm_ui(bool off,bool wifi) {
    portENTER_CRITICAL(&mux);
    bool powerChanged=powerWakePending;
    powerWakePending=false;
    bool unchanged=ready && stats.screenOff==uint32_t(off) && stats.wifiOn==uint32_t(wifi);
    portEXIT_CRITICAL(&mux);
    if (powerChanged) {
        meshflow_m9_pm_power_sample();
        if (power) {
            power->setIntervalFromNow(0);
            concurrency::mainDelay.interrupt();
        }
    }
    if (unchanged) return;
    xSemaphoreTake(policyMutex,portMAX_DELAY);
    if (!gate && !stats.error) initialize();
    // Acquire before publishing any active state; entry is blocked before UI work resumes.
    if ((!off || wifi) && gate && !gateHeld) {esp_pm_lock_acquire(gate);gateHeld=true;}
    portENTER_CRITICAL(&mux);account();stats.screenOff=off;stats.wifiOn=wifi;account();portEXIT_CRITICAL(&mux);
    if (gate && cpu && bus) apply();
    xSemaphoreGive(policyMutex);
}
extern "C" void meshflow_m9_pm_gps(bool active) {
    xSemaphoreTake(policyMutex,portMAX_DELAY);
    if (active && gate && !gateHeld) {esp_pm_lock_acquire(gate);gateHeld=true;}
    portENTER_CRITICAL(&mux);account();gpsSeen=true;stats.gpsOn=active;account();portEXIT_CRITICAL(&mux);
    if (gate && cpu && bus) apply();
    xSemaphoreGive(policyMutex);
}
extern "C" void meshflow_m9_pm_audio(bool active) {
    xSemaphoreTake(policyMutex,portMAX_DELAY);
    if (!gate && !stats.error) initialize();
    if (active && gate && !gateHeld) {esp_pm_lock_acquire(gate);gateHeld=true;}
    portENTER_CRITICAL(&mux);
    account();
    if (active) ++audioUsers;
    else if (audioUsers) --audioUsers;
    account();
    portEXIT_CRITICAL(&mux);
    if (gate && cpu && bus) apply();
    xSemaphoreGive(policyMutex);
}
extern "C" void IRAM_ATTR meshflow_m9_pm_radio_arm(bool active) {
    portENTER_CRITICAL_ISR(&mux);radioArmed=active;portEXIT_CRITICAL_ISR(&mux);
}
extern "C" bool IRAM_ATTR meshflow_m9_pm_claim_radio() {
    portENTER_CRITICAL_ISR(&mux);bool claimed=m9ClaimRadioIrq(radioArmed);portEXIT_CRITICAL_ISR(&mux);return claimed;
}
extern "C" void meshflow_m9_pm_power_sample() {
    bool powered=gpio_get_level((gpio_num_t)EXT_PWR_DETECT)==EXT_PWR_DETECT_VALUE;
    xSemaphoreTake(policyMutex,portMAX_DELAY);
    if(powered && gate && !gateHeld) {esp_pm_lock_acquire(gate);gateHeld=true;}
    portENTER_CRITICAL(&mux);account();stats.usbOn=powered;account();portEXIT_CRITICAL(&mux);
    if(gate && cpu && bus)apply();
    xSemaphoreGive(policyMutex);
}
extern "C" bool meshflow_m9_pm_bus_acquire() {
    portENTER_CRITICAL(&mux);++stats.busUsers;auto handle=bus;portEXIT_CRITICAL(&mux);
    if(handle)esp_pm_lock_acquire(handle);
    return handle!=nullptr;
}
extern "C" void meshflow_m9_pm_bus_release(bool held) {
    portENTER_CRITICAL(&mux);--stats.busUsers;portEXIT_CRITICAL(&mux);
    if(held)esp_pm_lock_release(bus);
}
extern "C" void meshflow_m9_pm_snapshot(M9SleepStats *out) {
    portENTER_CRITICAL(&mux);account();*out=stats;out->wifiSleepReady=wifiSleepReady;portEXIT_CRITICAL(&mux);
}
extern "C" esp_err_t __real_esp_light_sleep_start();
extern "C" esp_err_t IRAM_ATTR __wrap_esp_light_sleep_start() {
    // Only the RTOS path has run our entry callback. Do not label an unrelated
    // explicit Meshtastic sleep call as automatic idle sleep.
    if (!pinsArmed) return __real_esp_light_sleep_start();
    if (wakeConfigError != ESP_OK) return wakeConfigError;
    portENTER_CRITICAL_ISR(&mux);bool eligible=eligibleAt!=0;portEXIT_CRITICAL_ISR(&mux);
    int64_t start=esp_timer_get_time();
    esp_err_t result=__real_esp_light_sleep_start();
    uint64_t elapsed=esp_timer_get_time()-start;
    portENTER_CRITICAL_ISR(&mux);
    auto cause=esp_sleep_get_wakeup_cause();
    unsigned kind=cause==ESP_SLEEP_WAKEUP_TIMER ? 1 : cause==ESP_SLEEP_WAKEUP_GPIO ? 2 : 0;
    m9RecordSleepReturn(stats,result==ESP_OK,elapsed,eligible,kind);
    portEXIT_CRITICAL_ISR(&mux);
    return result;
}
#endif
