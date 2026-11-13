#include "input/InputDriver.h"
#include "misc/lv_timer_private.h"

#include <doctest/doctest.h>

namespace
{
class InputDriverPeriodProbe final : public InputDriver
{
  public:
    static void configure(lv_indev_t *indev) { configureReadPeriod(indev); }
};
} // namespace

TEST_CASE("input read period override changes only the input device timer")
{
    if (!lv_is_initialized())
        lv_init();

    lv_indev_t *indev = lv_indev_create();
    REQUIRE(indev != nullptr);
    lv_timer_t *readTimer = lv_indev_get_read_timer(indev);
    REQUIRE(readTimer != nullptr);
    const uint32_t displayDefaultPeriod = readTimer->period;

    InputDriverPeriodProbe::configure(indev);

#if defined(MESHFLOW_INDEV_READ_PERIOD_MS) && MESHFLOW_INDEV_READ_PERIOD_MS > 0
    CHECK(readTimer->period == MESHFLOW_INDEV_READ_PERIOD_MS);
    CHECK(readTimer->period < displayDefaultPeriod);
#else
    CHECK(readTimer->period == displayDefaultPeriod);
#endif

    lv_indev_delete(indev);
}
