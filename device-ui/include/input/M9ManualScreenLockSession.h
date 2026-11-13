#pragma once

// Testable owner for the exact view and input group suspended by the M9
// manual lock gesture. Timeout blanking remains a separate state.
template <typename Screen, typename InputGroup> class M9ManualScreenLockSession
{
  public:
    bool begin(Screen *screen, InputGroup *inputGroup)
    {
        if (locked || !screen)
            return false;

        savedScreen = screen;
        savedInputGroup = inputGroup;
        locked = true;
        return true;
    }

    bool end(Screen *&screen, InputGroup *&inputGroup)
    {
        if (!locked)
            return false;

        screen = savedScreen;
        inputGroup = savedInputGroup;
        savedScreen = nullptr;
        savedInputGroup = nullptr;
        locked = false;
        return true;
    }

    bool isActive() const { return locked; }

  private:
    Screen *savedScreen = nullptr;
    InputGroup *savedInputGroup = nullptr;
    bool locked = false;
};
