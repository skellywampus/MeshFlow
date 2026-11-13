#if defined(ELECROW_ThinkNode_M9) && defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <esp32-hal-cpu.h>
#include "graphics/common/M9PowerStatistics.h"
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
#include "graphics/common/M9AutoLightSleep.h"
#endif
#endif
#include "graphics/DeviceGUI.h"
#include "graphics/MeshFlowPerfStats.h"
#include "graphics/driver/DisplayDriver.h"
#include "graphics/driver/DisplayDriverConfig.h"
#include "input/I2CKeyboardScanner.h"
#include "input/InputDriver.h"
#include "input/M9ScreenLockPolicy.h"
#include <thread>

#include "input/I2CKeyboardInputDriver.h"
#ifdef ELECROW_ThinkNode_M9
#include "graphics/view/TFT/TFTView_320x240.h"
#endif
static I2CKeyboardInputDriver *keyboardDriver = nullptr;
#ifdef ELECROW_ThinkNode_M9
#include <Arduino.h>
#include <atomic>
static DeviceGUI *m9Gui = nullptr;
static std::atomic<uint32_t> m9LockAt{0};
static std::atomic<uint32_t> m9WakeAt{0};
#endif

#if LV_USE_LIBINPUT
#include "input/LinuxInputDriver.h"
static LinuxInputDriver *linuxInputDriver = nullptr;
#else
#if defined(INPUTDRIVER_ENCODER_TYPE)
#include "input/EncoderInputDriver.h"
static EncoderInputDriver *encoderDriver = nullptr;
#endif
#if defined(INPUTDRIVER_MATRIX_TYPE)
#include "input/KeyMatrixInputDriver.h"
static KeyMatrixInputDriver *keyMatrixDriver = nullptr;
#endif
#if defined(INPUTDRIVER_BUTTON_TYPE)
#include "input/ButtonInputDriver.h"
static ButtonInputDriver *buttonDriver = nullptr;
#endif
#if defined(INPUTDRIVER_DIRECTIONALPAD_TYPE)
#include "input/DirectionalPadInputDriver.h"
static DirectionalPadInputDriver *padDriver = nullptr;
#endif
#endif
#include "lvgl.h"
#include "ui.h"
#include "util/ILog.h"

DeviceGUI *DeviceGUI::activeGUI = nullptr;

DeviceGUI::DeviceGUI(const DisplayDriverConfig *cfg, DisplayDriver *driver) : displaydriver(driver), inputdriver(nullptr)
{
#ifdef ELECROW_ThinkNode_M9
    m9Gui = this;
#endif
    activeGUI = this;

#if LV_USE_LIBINPUT
    if (cfg) {
        linuxInputDriver = new LinuxInputDriver(cfg->keyboard(), cfg->pointer());
        inputdriver = linuxInputDriver;
    }
#endif
#if defined(INPUTDRIVER_ENCODER_TYPE)
    encoderDriver = new EncoderInputDriver;
#endif
#if defined(INPUTDRIVER_MATRIX_TYPE)
    keyMatrixDriver = new KeyMatrixInputDriver;
#endif
#if defined(INPUTDRIVER_BUTTON_TYPE)
    buttonDriver = new ButtonInputDriver;
#endif
#if defined(INPUTDRIVER_DIRECTIONALPAD_TYPE)
    padDriver = new DirectionalPadInputDriver;
#endif

    if (!inputdriver)
        inputdriver = InputDriver::instance();
}

void DeviceGUI::init(IClientBase *client)
{
    ILOG_DEBUG("Display driver init...");
    displaydriver->init(this);

    ILOG_DEBUG("Input driver init...");
    I2CKeyboardScanner scanner;
    keyboardDriver = scanner.scan();
    if (keyboardDriver)
        keyboardDriver->init();
#if LV_USE_LIBINPUT
    if (linuxInputDriver)
        linuxInputDriver->init();
#endif
#if defined(INPUTDRIVER_ENCODER_TYPE)
    if (encoderDriver)
        encoderDriver->init();
#endif
#if defined(INPUTDRIVER_MATRIX_TYPE)
    if (keyMatrixDriver)
        keyMatrixDriver->init();
#endif
#if defined(INPUTDRIVER_BUTTON_TYPE)
    if (buttonDriver)
        buttonDriver->init();
#endif
#if defined(INPUTDRIVER_DIRECTIONALPAD_TYPE)
    if (padDriver)
        padDriver->init();
#endif

    displaydriver->printConfig();
}

void DeviceGUI::toggleDisplay(void)
{
    displaydriver->toggleDisplay();
}

/**
 * Linux: measure how long it takes to call displaydriver->task_handler().
 * Then tell the lvgl library how long it took via lv_tick_inc().
 */
void DeviceGUI::task_handler(void)
{
#if defined(ARCH_PORTDUINO)
    int ms = 10;
    auto start = std::chrono::high_resolution_clock::now();
    displaydriver->task_handler();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    if (duration.count() < ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms - duration.count()));
        lv_tick_inc(ms);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        lv_tick_inc(duration.count() + 1);
    }
#else
#if defined(MESHFLOW_PERF_STATS)
    const uint32_t perfStartUs = micros();
#endif
    displaydriver->task_handler();
#if defined(ELECROW_ThinkNode_M9) && defined(ARDUINO_ARCH_ESP32)
    // Keep input, model updates and protocol timers alive, but stop invisible flushes.
    static bool refreshPaused = false;
    const bool asleep = displaydriver->isPowersaving();
    if (asleep != refreshPaused) {
        auto *refresh = lv_display_get_refr_timer(lv_display_get_default());
        if (asleep) lv_timer_pause(refresh);
        else {
            lv_timer_resume(refresh);
            lv_obj_invalidate(lv_screen_active());
        }
        refreshPaused = asleep;
    }
    // Keep APB at 80 MHz and full CPU speed whenever Wi-Fi or the UI needs it.
    const uint32_t target = asleep && WiFi.getMode() == WIFI_OFF ? 80 : 240;
    static uint32_t applied = 0;
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
    meshflow_m9_pm_ui(asleep, WiFi.getMode() != WIFI_OFF);
    applied = target;
#else
    if (applied != target && setCpuFrequencyMhz(target)) applied = target;
#endif
    static uint32_t recorded = UINT32_MAX;
    static bool recordedOn = true;
    if (recorded != applied || recordedOn == asleep) {
        meshflow_power_state(getCpuFrequencyMhz(), !asleep);
        recorded = applied;
        recordedOn = !asleep;
    }
#endif
#if defined(MESHFLOW_PERF_STATS)
    MeshFlowPerfStats::recordLoop((uint32_t)(micros() - perfStartUs));
    MeshFlowPerfStats::reportIfDue();
#endif
#endif
};

DeviceGUI::~DeviceGUI()
{
#ifdef ELECROW_ThinkNode_M9
    if (m9Gui == this)
        m9Gui = nullptr;
#endif
    if (activeGUI == this)
        activeGUI = nullptr;
    delete inputdriver;
}

#ifdef ELECROW_ThinkNode_M9
bool DeviceGUI::m9WakeArmed()
{
    return millis() - m9LockAt.load() >= 500U;
}

void DeviceGUI::noteM9ScreenLock() { m9LockAt.store(millis()); }
void DeviceGUI::noteM9ScreenWake()
{
    m9WakeAt.store(millis());
    if (m9Gui && m9Gui->displaydriver)
        m9Gui->displaydriver->forceWakeup();
}

bool DeviceGUI::suppressM9Key(uint32_t key)
{
    const uint32_t wakeAt = m9WakeAt.load();
    auto *view = TFTView_320x240::currentInstance();
    return (view && view->isScreenLocked()) ||
           (wakeAt && (millis() - wakeAt < 500U || (key == 0xA3 && millis() - wakeAt < 2000U)));
}
#endif

void DeviceGUI::lockFromM9Keyboard(void)
{
    if (!activeGUI || !activeGUI->displaydriver || activeGUI->isManualScreenLockActive())
        return;

    if (M9ScreenLockPolicy::shouldRestoreBeforeManualLock(activeGUI->isManualScreenLockActive(), activeGUI->isScreenLocked())) {
        // Restore the LVGL screen state without waking the panel. The manual
        // lock immediately replaces it with the blank lock screen, so waking
        // here would flash the display and spend power for a frame that the
        // user cannot interact with.
        activeGUI->screenSaving(false);
    }

    ILOG_INFO("M9 manual screen lock requested");
    activeGUI->manualScreenLock();
    if (activeGUI->isManualScreenLockActive())
        activeGUI->displaydriver->forceScreenSleep();
}

void DeviceGUI::unlockFromM9Keyboard(void)
{
    if (activeGUI && activeGUI->displaydriver && activeGUI->isManualScreenLockActive()) {
        ILOG_INFO("M9 manual screen unlock requested");
        activeGUI->displaydriver->forceScreenWake();
        activeGUI->manualScreenUnlock();
    }
}

bool DeviceGUI::isManuallyLocked(void)
{
    return activeGUI && activeGUI->isManualScreenLockActive();
}

void DeviceGUI::prepareSleep(void)
{
    if (inputdriver)
        inputdriver->prepareSleep();
}

void DeviceGUI::wakeUp(void)
{
    if (inputdriver)
        inputdriver->wakeUp();
}
