#include "graphics/common/M9PowerGuards.h"
#include <vector>
#ifdef M9_GUARD_STANDALONE
#include <cassert>
#include <cstdio>
static unsigned checks = 0;
#define CHECK(value) do { ++checks; assert(value); } while (false)
#else
#include <doctest/doctest.h>
#endif

namespace {
struct GPIOModel {
    enum { Disabled, Edge, Low };
    M9WakePinState state{Edge, 3, false};
    bool usb = false, failArm = false, failRestore = false, pending = false, transitionOnArm = false;
    std::vector<int> calls;
    M9WakePinState snapshot() {return state;}
    bool powered() {return usb;}
    void mask() {calls.push_back(1);state.interrupts=0;}
    bool armWake() {
        calls.push_back(2);
        CHECK(state.interrupts==0);
        state.type=Low;state.wake=true;
        if (transitionOnArm) {usb=!usb;pending=true;}
        return !failArm;
    }
    bool restore(const M9WakePinState &saved) {
        calls.push_back(3);
        CHECK(state.interrupts==0);
        state.wake=false;
        state.type=saved.type;
        pending=false;
        if (failRestore) return false;
        state.wake=saved.wake;
        state.interrupts=saved.interrupts;
        return true;
    }
};

void wakeRestoration() {
    // Both normal ANYEDGE and an originally disabled IRQ must be preserved.
    // Exercise completed/rejected calls and partial wake-setup failure alike.
    for (unsigned irq : {0u, 3u}) for (bool before : {false,true})
        for (bool after : {false,true}) for (bool fail : {false,true}) {
            GPIOModel gpio;gpio.state.interrupts=irq;gpio.usb=before;gpio.failArm=fail;
            M9PowerWakeGuard guard;
            CHECK(guard.arm(gpio)==!fail);
            gpio.usb=after;gpio.pending=true;
            guard.restore(gpio);
            CHECK(gpio.state.type==GPIOModel::Edge);
            CHECK(gpio.state.interrupts==irq);
            CHECK(!gpio.state.wake);
            CHECK(gpio.pending==fail);
            // A transition after an already-failed arm is handled by the normal
            // restored edge handler, not incorrectly replayed as a sleep edge.
            CHECK((guard.takeChanged()==(!fail && before!=after)));
            CHECK(!guard.takeChanged());
            size_t count=gpio.calls.size();guard.restore(gpio);
            CHECK(gpio.calls.size()==count);
            CHECK((gpio.calls[0]==1 && gpio.calls[1]==2));
        }
    GPIOModel gpio;gpio.state={GPIOModel::Low, 3, true};
    M9PowerWakeGuard guard;CHECK(guard.arm(gpio));
    guard.restore(gpio);
    CHECK((gpio.state.type==GPIOModel::Low && gpio.state.wake && gpio.state.interrupts==3));
    for (unsigned i=0;i<1000;++i) {
        gpio.state={GPIOModel::Edge, 3, false};gpio.usb=false;
        CHECK(guard.arm(gpio));gpio.usb=true;guard.restore(gpio);
        CHECK(guard.takeChanged());CHECK(gpio.state.type==GPIOModel::Edge);
    }
    CHECK(guard.arm(gpio));gpio.failRestore=true;guard.restore(gpio);
    CHECK(guard.faulted());CHECK(gpio.state.interrupts==0);
    GPIOModel failed;failed.failArm=true;failed.transitionOnArm=true;
    M9PowerWakeGuard failedGuard;CHECK(!failedGuard.arm(failed));
    CHECK(failedGuard.takeChanged());CHECK(!failed.pending);
    CHECK((failed.state.type==GPIOModel::Edge && failed.state.interrupts==3));
}

void playbackLifetime() {
    unsigned owners=0;
    auto acquire=[&] {++owners;};auto release=[&] {CHECK(owners>0);--owners;};
    M9PlaybackLease ringtone, systemTone;
    for (unsigned i=0;i<1000;++i) {
        ringtone.begin(acquire);ringtone.begin(acquire);CHECK(owners==1);
        systemTone.begin(acquire);CHECK(owners==2);
        ringtone.end(release);ringtone.end(release);CHECK(owners==1);
        systemTone.end(release);CHECK(owners==0);
        CHECK((!ringtone.active() && !systemTone.active()));
    }
    // Immediate begin/end is the failed/empty-song path; no owner can leak.
    ringtone.begin(acquire);ringtone.end(release);CHECK(owners==0);
}
}

#ifdef M9_GUARD_STANDALONE
int main() {
    wakeRestoration();playbackLifetime();
    std::printf("PASS: GPIO restoration and audio lifecycle guards, %u checks\n", checks);
}
#else
TEST_CASE("M9 power wake restores original IRQ state across USB transitions") {wakeRestoration();}
TEST_CASE("M9 playback leases release on completion cancellation and failed start") {playbackLifetime();}
#endif
