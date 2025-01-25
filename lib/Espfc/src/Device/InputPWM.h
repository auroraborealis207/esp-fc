#pragma once

#include <cstdint>
#include <cstddef>
#include "InputDevice.h"

namespace Espfc {

enum PWMMode {
  PWM_MODE_NORMAL   = 0x01, // RISING edge
  PWM_MODE_INVERTED = 0x02  // FALLING edge
};

namespace Device {

class InputPWM: public InputDevice
{
  public:
    void begin(uint8_t* pin, int mode = PWM_MODE_NORMAL);
    InputStatus update() override;
    uint16_t get(uint8_t i) const override;
    void get(uint16_t * data, size_t len) const override;
    size_t getChannelCount() const override;
    bool needAverage() const override;

  private:
    static const size_t CHANNELS = 16;
    static const uint32_t BROKEN_LINK_US = 100000UL; // 100ms

    void handle();
    static void handle_isr(void* args);

    volatile uint16_t _channels[CHANNELS];
    volatile uint32_t _last_tick;
    volatile uint8_t  _channel;
    volatile uint8_t* _last_channel;
    volatile uint8_t* _timer;

    volatile bool     _new_data;
    uint8_t* _pin;
};

}

}
