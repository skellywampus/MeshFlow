#pragma once
#include <stdint.h>

#if defined(MESHFLOW_PERF_STATS)
#include <Arduino.h>
#include <esp_heap_caps.h>
#endif

// Opt-in, fixed-cost counters for on-device UI profiling. Keep disabled in
// normal builds; enabling the macro reports rolling 5-second UI/flush samples.
class MeshFlowPerfStats
{
  public:
    static void recordLoop(uint32_t durationUs)
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        ++s.loopCount;
        s.loopTotalUs += durationUs;
        if (durationUs > s.loopMaxUs)
            s.loopMaxUs = durationUs;
#else
        (void)durationUs;
#endif
    }

    static void recordFlush(uint32_t durationUs, uint32_t pixels)
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        ++s.flushCount;
        s.flushTotalUs += durationUs;
        s.flushPixels += pixels;
        if (durationUs > s.flushMaxUs)
            s.flushMaxUs = durationUs;
        if (s.inputPending) {
            const uint32_t latencyUs = (uint32_t)(micros() - s.inputAtUs);
            if (latencyUs <= 500000U) {
                recordTimed(s.inputCount, s.inputTotalUs, s.inputMaxUs, latencyUs);
            }
            s.inputPending = false;
        }
        if (s.keyboardReadPending) {
            const uint32_t latencyUs = (uint32_t)(micros() - s.keyboardReadAtUs);
            if (latencyUs <= 500000U) {
                recordTimed(s.keyboardReadCount, s.keyboardReadTotalUs, s.keyboardReadMaxUs, latencyUs);
            }
            s.keyboardReadPending = false;
        }
#else
        (void)durationUs;
        (void)pixels;
#endif
    }

    // Call when an M9 key reaches the screen-level handler. The next flush
    // callback estimates event-to-render submission latency, not panel completion.
    static void markInputEvent()
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        s.inputAtUs = micros();
        s.inputPending = true;
#endif
    }

    // Call after the M9 controller returns a raw key byte, before mapping and policy dispatch.
    static void markKeyboardRead()
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        s.keyboardReadAtUs = micros();
        s.keyboardReadPending = true;
#endif
    }

    static void recordViewSetup(uint32_t durationUs)
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        recordTimed(s.viewSetupCount, s.viewSetupTotalUs, s.viewSetupMaxUs, durationUs);
#else
        (void)durationUs;
#endif
    }

    static void recordNodeListSync(uint32_t durationUs)
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        recordTimed(s.nodeListSyncCount, s.nodeListSyncTotalUs, s.nodeListSyncMaxUs, durationUs);
#else
        (void)durationUs;
#endif
    }

    static void recordChatRender(uint32_t durationUs)
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        recordTimed(s.chatRenderCount, s.chatRenderTotalUs, s.chatRenderMaxUs, durationUs);
#else
        (void)durationUs;
#endif
    }

    static void reportIfDue()
    {
#if defined(MESHFLOW_PERF_STATS)
        auto &s = state();
        const uint32_t now = millis();
        if (!s.reportStarted) {
            s.reportStarted = true;
            s.reportAt = now;
            return;
        }
        if ((uint32_t)(now - s.reportAt) < 5000)
            return;

        const uint32_t reportWindowMs = (uint32_t)(now - s.reportAt);
        const uint32_t internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        const uint32_t internalLargest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        const uint32_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        const uint32_t psramLargest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        Serial.printf("MeshFlow perf/5s: loop n=%u avg=%u us max=%u us; transition n=%u avg=%u us max=%u us; key-read-to-flush n=%u avg=%u us max=%u us; LVGL-key-to-flush n=%u avg=%u us max=%u us; node-sync n=%u avg=%u us max=%u us; chat-render n=%u avg=%u us max=%u us; flush-callback n=%u avg=%u us max=%u us rate=%u/s pixels=%u; heap internal=%u largest=%u PSRAM=%u largest=%u\n",
                      (unsigned)s.loopCount, average(s.loopTotalUs, s.loopCount), (unsigned)s.loopMaxUs,
                      (unsigned)s.viewSetupCount, average(s.viewSetupTotalUs, s.viewSetupCount), (unsigned)s.viewSetupMaxUs,
                      (unsigned)s.keyboardReadCount, average(s.keyboardReadTotalUs, s.keyboardReadCount),
                      (unsigned)s.keyboardReadMaxUs,
                      (unsigned)s.inputCount, average(s.inputTotalUs, s.inputCount), (unsigned)s.inputMaxUs,
                      (unsigned)s.nodeListSyncCount, average(s.nodeListSyncTotalUs, s.nodeListSyncCount), (unsigned)s.nodeListSyncMaxUs,
                      (unsigned)s.chatRenderCount, average(s.chatRenderTotalUs, s.chatRenderCount), (unsigned)s.chatRenderMaxUs,
                      (unsigned)s.flushCount, average(s.flushTotalUs, s.flushCount), (unsigned)s.flushMaxUs,
                      (unsigned)(s.flushCount * 1000U / reportWindowMs), (unsigned)s.flushPixels,
                      (unsigned)internalFree, (unsigned)internalLargest, (unsigned)psramFree, (unsigned)psramLargest);
        s.loopCount = s.loopTotalUs = s.loopMaxUs = 0;
        s.viewSetupCount = s.viewSetupTotalUs = s.viewSetupMaxUs = 0;
        s.inputCount = s.inputTotalUs = s.inputMaxUs = 0;
        s.keyboardReadCount = s.keyboardReadTotalUs = s.keyboardReadMaxUs = 0;
        s.nodeListSyncCount = s.nodeListSyncTotalUs = s.nodeListSyncMaxUs = 0;
        s.chatRenderCount = s.chatRenderTotalUs = s.chatRenderMaxUs = 0;
        s.flushCount = s.flushTotalUs = s.flushMaxUs = s.flushPixels = 0;
        s.reportAt = now;
#endif
    }

  private:
#if defined(MESHFLOW_PERF_STATS)
    struct Counters {
        uint32_t loopCount = 0, loopTotalUs = 0, loopMaxUs = 0;
        uint32_t viewSetupCount = 0, viewSetupTotalUs = 0, viewSetupMaxUs = 0;
        uint32_t inputCount = 0, inputTotalUs = 0, inputMaxUs = 0, inputAtUs = 0;
        uint32_t keyboardReadCount = 0, keyboardReadTotalUs = 0, keyboardReadMaxUs = 0, keyboardReadAtUs = 0;
        uint32_t nodeListSyncCount = 0, nodeListSyncTotalUs = 0, nodeListSyncMaxUs = 0;
        uint32_t chatRenderCount = 0, chatRenderTotalUs = 0, chatRenderMaxUs = 0;
        uint32_t flushCount = 0, flushTotalUs = 0, flushMaxUs = 0, flushPixels = 0;
        uint32_t reportAt = 0;
        bool reportStarted = false, inputPending = false, keyboardReadPending = false;
    };

    static Counters &state()
    {
        static Counters counters;
        return counters;
    }

    static uint32_t average(uint32_t total, uint32_t count) { return count ? total / count : 0; }
    static void recordTimed(uint32_t &count, uint32_t &total, uint32_t &max, uint32_t durationUs)
    {
        ++count;
        total += durationUs;
        if (durationUs > max)
            max = durationUs;
    }
#endif

};
