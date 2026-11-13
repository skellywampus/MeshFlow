#include "input/InputDriver.h"

#ifndef MESHFLOW_INDEV_READ_PERIOD_MS
#define MESHFLOW_INDEV_READ_PERIOD_MS 0
#endif

InputDriver *InputDriver::driver = nullptr;
lv_indev_t *InputDriver::keyboard = nullptr;
lv_indev_t *InputDriver::pointer = nullptr;
lv_indev_t *InputDriver::encoder = nullptr;
lv_indev_t *InputDriver::button = nullptr;
lv_group_t *InputDriver::inputGroup = nullptr;

std::string InputDriver::keyboardDevice = "none";
std::string InputDriver::pointerDevice = "none";

void InputDriver::configureReadPeriod(lv_indev_t *indev)
{
#if MESHFLOW_INDEV_READ_PERIOD_MS > 0
    // Keep input polling independent from the display refresh timer so a board
    // can scan physical controls more often without scheduling extra redraws.
    if (indev) {
        if (lv_timer_t *readTimer = lv_indev_get_read_timer(indev))
            lv_timer_set_period(readTimer, MESHFLOW_INDEV_READ_PERIOD_MS);
    }
#else
    (void)indev;
#endif
}

InputDriver *InputDriver::instance(void)
{
    if (!driver)
        driver = new InputDriver;
    return driver;
}

InputDriver::~InputDriver(void)
{
    if (keyboard)
        releaseKeyboardDevice();
    if (pointer)
        releasePointerDevice();
}
