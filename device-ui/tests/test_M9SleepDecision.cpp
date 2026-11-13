#include "graphics/common/M9SleepDecision.h"
#include <doctest/doctest.h>
#include <vector>
#include <string>

// Exercise the real veto/dedup/accounting helpers used by the IDF callbacks.
// These checks do not substitute for observing GPIO wake and RF receive on M9.
TEST_CASE("M9 idle sleep requires all safety conditions") {
    M9SleepConditions good{true,true,false,false,false,true};
    CHECK(m9SleepBlocks(good)==0);
    auto c=good;c.ready=false;CHECK(m9SleepBlocks(c)==1);
    c=good;c.screenOff=false;CHECK(m9SleepBlocks(c)==2);
    c=good;c.gpsPowered=true;CHECK(m9SleepBlocks(c)==4);
    c=good;c.wifiActive=true;CHECK(m9SleepBlocks(c)==8);
    c=good;c.spiBusy=true;CHECK(m9SleepBlocks(c)==16);
    c=good;c.radioArmed=false;CHECK(m9SleepBlocks(c)==32);
    c=good;c.usbPowered=true;CHECK(m9SleepBlocks(c)==64);
    c=good;c.serialActive=true;CHECK(m9SleepBlocks(c)==128);
}
TEST_CASE("M9 TX and fallback ISR/replay claim once per radio arm") {
    bool armed=false;
    CHECK_FALSE(m9ClaimRadioIrq(armed));
    for(unsigned cycle=0;cycle<1000;++cycle) {
        armed=true;
        CHECK(m9ClaimRadioIrq(armed)); // ordinary ISR or replay wins
        CHECK_FALSE(m9ClaimRadioIrq(armed)); // the other path must not dispatch again
        CHECK_FALSE(armed);
    }
}
TEST_CASE("M9 rejected sleeps never count as successful entries or time") {
    M9SleepStats s{};
    m9RecordSleepReturn(s,false,1900,true,2);
    CHECK(s.failures==1);CHECK(s.entries==0);CHECK(s.successCallUs==0);CHECK(s.gpioWakes==0);
    m9RecordSleepReturn(s,true,12000,true,1);
    m9RecordSleepReturn(s,true,8000,true,2);
    m9RecordSleepReturn(s,true,4000,false,0);
    CHECK(s.entries==3);CHECK(s.failures==1);
    CHECK(s.successCallUs==24000);CHECK(s.eligibleCallUs==20000);
    CHECK(s.timerWakes==1);CHECK(s.gpioWakes==1);CHECK(s.otherWakes==1);
}

// A TX rearm wakes a higher-priority worker which can install RX immediately.
// Detaching after that wake would remove the new handler and strand the next RX.
TEST_CASE("M9 idle wake preserves fast RX and detaches TX before rearm") {
    std::vector<std::string> events;
    bool claimed=false;
    auto run=[&](bool tx,bool fast) {
        m9DispatchIdleWake(tx,fast,
            [&]{events.push_back("claim");return !claimed && (claimed=true);},
            [&]{events.push_back("detach old");},
            [&]{events.push_back("rearm next");},
            [&]{events.push_back("fast readout");},
            [&](bool tx){events.push_back(tx ? "notify TX" : "notify RX");});
    };
    run(false,true);
    CHECK(events==std::vector<std::string>{"fast readout"});
    CHECK_FALSE(claimed);
    events.clear();run(true,true);
    CHECK(events==std::vector<std::string>{"claim","detach old","rearm next","notify TX"});
    events.clear();run(true,true);
    CHECK(events==std::vector<std::string>{"claim"});
    claimed=false;events.clear();run(false,false);
    CHECK(events==std::vector<std::string>{"claim","detach old","notify RX"});
}
