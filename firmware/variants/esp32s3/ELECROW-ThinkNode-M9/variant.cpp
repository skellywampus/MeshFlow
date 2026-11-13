#include "variant.h"
#include "Arduino.h"
#include "SPILock.h"
#include "Wire.h"
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"
#endif

#define KBD_ADDR_M9_VERSION1_0 0x6C
#define KBD_ADDR_M9_VERSION1_1 0x6D

int m9Version = 0;

void earlyInitVariant()
{
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
    // CPU software resets retain GPIO configuration. Sanitize only our wake
    // pins before Power/input/radio can install the shared GPIO ISR service.
    const int wakePins[] = {EXT_PWR_DETECT, KB_INT, LR1110_IRQ_PIN};
    for (int pin : wakePins) {
        gpio_intr_disable((gpio_num_t)pin);
        gpio_wakeup_disable((gpio_num_t)pin);
        gpio_set_intr_type((gpio_num_t)pin, GPIO_INTR_DISABLE);
        if (pin < 32) gpio_ll_clear_intr_status(&GPIO, 1UL << pin);
        else gpio_ll_clear_intr_status_high(&GPIO, 1UL << (pin - 32));
    }
#endif
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, !GPS_EN_ACTIVE);
    pinMode(GPS_SLEEP_INT, OUTPUT);
    digitalWrite(GPS_SLEEP_INT, HIGH);
    delay(100);
}

void initVariant()
{
    // determine M9 version
    Wire.begin(SDA, SCL);
    Wire.beginTransmission(KBD_ADDR_M9_VERSION1_0);
    if (Wire.endTransmission() == 0) {
        m9Version = 1;
    }
    Wire.beginTransmission(KBD_ADDR_M9_VERSION1_1);
    if (Wire.endTransmission() == 0) {
        m9Version = 2;
    }

    if (m9Version > 0) {
        // configure keyboard long-press time
        const uint16_t ms = 700;
        Wire.beginTransmission(m9Version == 1 ? KBD_ADDR_M9_VERSION1_0 : KBD_ADDR_M9_VERSION1_1);
        Wire.write(0x03);
        Wire.write((ms >> 8) & 0xFF);
        Wire.write(ms & 0xFF);
        Wire.endTransmission();
    }
    Wire.end();
}

void variant_shutdown()
{
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, !GPS_EN_ACTIVE);
    uint64_t gpioMask = (1ULL << KB_INT);
    gpio_pulldown_en((gpio_num_t)KB_INT);
    esp_sleep_enable_ext1_wakeup(gpioMask, ESP_EXT1_WAKEUP_ANY_HIGH);
}
