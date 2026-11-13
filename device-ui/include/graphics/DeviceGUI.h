#pragma once

#include <stdint.h>

class DisplayDriverConfig;
class DisplayDriver;
class InputDriver;
class IClientBase;

/**
 * @brief DeviceGUI - this class hierarchy implements the view part of MVC
 *
 */
class DeviceGUI
{
  public:
    DeviceGUI(const DisplayDriverConfig *cfg, DisplayDriver *driver);
    virtual ~DeviceGUI();
    virtual void init(IClientBase *client);
    virtual void task_handler(void);
    virtual void prepareSleep(void);
    virtual bool sleep(int16_t pin) { return false; }
    virtual void wakeUp(void);
#ifdef ELECROW_ThinkNode_M9
    static bool m9WakeArmed();
    static bool suppressM9Key(uint32_t key = 0);
    static void noteM9ScreenLock();
    static void noteM9ScreenWake();
#endif
    virtual bool prepareShutdown(uint32_t timeoutMs) { return true; }
    virtual void toggleDisplay(void);
    virtual void triggerHeartbeat(void) {}
    // called to inform view to do screen blanking
    virtual void blankScreen(bool enable) {}
    // called when display (driver) is doing screen blanking
    virtual void screenSaving(bool enabled) {};
    // return true if screen view is locked
    virtual bool isScreenLocked(void) { return false; };
    virtual void manualScreenLock(void) { screenSaving(true); }
    virtual void manualScreenUnlock(void) { screenSaving(false); }
    virtual bool isManualScreenLockActive(void) { return false; }

    // M9 keyboard gesture entry points. DeviceGUI keeps the active view so the
    // board-specific input driver can lock without depending on a concrete view.
    static void lockFromM9Keyboard(void);
    static void unlockFromM9Keyboard(void);
    static bool isManuallyLocked(void);

    DisplayDriver *getDisplayDriver(void) const { return displaydriver; }
    InputDriver *getInputDriver(void) const { return inputdriver; }

  protected:
    DisplayDriver *displaydriver;
    InputDriver *inputdriver;

  private:
    static DeviceGUI *activeGUI;
};
