#include "Device/InputPWM.h"
#include <Arduino.h>
#include "Utils/MemoryHelper.h"

namespace Espfc {

namespace Device {

void InputPWM::begin(uint8_t* pin, int mode)
{
    for(int i=0;i<sizeof(_pin);i++){
        if(_pin[i] != -1)
        {
            detachInterrupt(_pin[i]);
            _pin[i] = -1;
        }
    }
    _pin = {};

    for(int i=0;i<sizeof(pin);i++){
        if(pin[i] != -1)
        {
            _pin[i] = pin[i];
            _channel = 0;
            _last_tick = micros();

            for(size_t i = 0; i < CHANNELS; i++){
                _channels[i] = (i == 2) ? 1000 : 1500; // throttle
            }

            pinMode(_pin[i], INPUT);
            
        #if defined(UNIT_TEST)
            // no mock available
        #elif defined(ARCH_RP2040)
            attachInterruptParam(_pin, InputPWM::handle_isr, (PinStatus)mode, this);
        #else
            attachInterruptArg(_pin[i], InputPWM::handle_isr, this, mode);
        #endif
        }
    }
}

InputStatus FAST_CODE_ATTR InputPWM::update()
{
  if(_new_data)
  {
    _new_data = false;
    return INPUT_RECEIVED;
  }
  return INPUT_IDLE;
}

uint16_t FAST_CODE_ATTR InputPWM::get(uint8_t i) const
{
  return _channels[i];
}

void FAST_CODE_ATTR InputPWM::get(uint16_t * data, size_t len) const
{
  const uint16_t * src = const_cast<const uint16_t *>(_channels);
  while(len--)
  {
    *data++ = *src++;
  }
}

size_t InputPWM::getChannelCount() const { return CHANNELS; }

bool InputPWM::needAverage() const { return true; }

void IRAM_ATTR InputPWM::handle()
{
    uint8_t current_time = micros();
    for(int i = 0;i<sizeof(_pin);i++){
        if (digitalRead(_pin[i])) {
            if (_last_channel[i] == 0) { 
                _last_channel[i] = 1; _timer[i] = current_time; 
            } 
        } else if (_last_channel[i] == 1) {
            _last_channel[i] = 0; 
            _channels[i] = current_time - _timer[i]; 
        } 
    }
}

void IRAM_ATTR InputPWM::handle_isr(void* args)
{
  if(args) reinterpret_cast<InputPWM*>(args)->handle();
}

}

}
