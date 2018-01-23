#ifndef _ESPFC_MIXER_H_
#define _ESPFC_MIXER_H_

#include "Model.h"
#include "EscDriver.h"

namespace Espfc {

class Mixer
{
  public:
    Mixer(Model& model): _model(model) {}

    int begin()
    {
      for(size_t i = 0; i < OUTPUT_CHANNELS; ++i)
      {
        _driver.attach(i, _model.config.pin[i + PIN_OUTPUT_0], _model.state.outputDisarmed[i]);
        _model.logger.info().log(F("OUTPUT PIN")).log(i).logln(_model.config.pin[i + PIN_OUTPUT_0]);
      }
      _driver.begin((EscProtocol)_model.config.output.protocol, _model.config.output.async, _model.config.output.rate);
      _model.logger.info().log(F("OUTPUT CONF")).log(_model.config.output.protocol).log(_model.config.output.async).logln(_model.config.output.rate);
      return 1;
    }

    int update()
    {
      _model.state.stats.start(COUNTER_MIXER);
      switch(_model.config.mixerType)
      {
        case FRAME_QUAD_X:
          updateQuadX();
          break;
        case FRAME_BALANCE_ROBOT:
          updateBalancingRobot();
          break;
      }
      _model.state.stats.end(COUNTER_MIXER);
      return 1;
    }

  private:
    void updateBalancingRobot()
    {
      float p = _model.state.output[AXIS_PITCH];
      float y = _model.state.output[AXIS_YAW] * (_model.config.yawReverse ? -1 : 1);
      float out[2];
      out[0] = p + y;
      out[1] = p - y;
      writeOutput(out, 2);
    }

    void updateQuadX()
    {
      float r = _model.state.output[AXIS_ROLL];
      float p = _model.state.output[AXIS_PITCH];
      float y = _model.state.output[AXIS_YAW] * (_model.config.yawReverse ? -1 : 1);
      float t = _model.state.output[AXIS_THRUST];

      float out[OUTPUT_CHANNELS];
      for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
      {
        out[i] = 0;
      }

      // TODO: use mix table
      out[0] = -r + p + y;
      out[1] = -r - p - y;
      out[2] =  r + p - y;
      out[3] =  r - p + y;

      // airmode logic
      if(_model.isActive(MODE_AIRMODE))
      {
        float min = 0.f, max = 0.f;
        for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
        {
          max = std::max(max, out[i]);
          min = std::min(min, out[i]);
        }
        float range = (max - min) * 0.5f;
        if(range > 1.f)
        {
          for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
          {
            out[i] /= range;
          }
          t = 0.f;
        }
        else
        {
          t = Math::bound(t, -1.f + range, 1.f - range);
        }
      }

      for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
      {
        out[i] += t;
      }
      writeOutput(out, 4);
    }

    void writeOutput(float * out, size_t axes)
    {
      bool stop = _stop();
      for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
      {
        if(i >= axes || stop)
        {
          _model.state.outputUs[i] = _model.state.outputDisarmed[i];
        }
        else
        {
          float v = Math::bound(out[i], -1.f, 1.f);
          const OutputChannelConfig& och = _model.config.output.channel[i];
          if(och.servo)
          {
            _model.state.outputUs[i] = (int16_t)Math::map3(v, -1.f, 0.f, 1.f, och.reverse ? och.max : och.min, och.neutral, och.reverse ? och.min : och.max);
          }
          else
          {
            _model.state.outputUs[i] = (int16_t)Math::map(v, -1.f, 1.f, _model.config.output.minThrottle, _model.config.output.maxThrottle);
          }
        }
      }
      _write();
    }

    void _write()
    {
      for(size_t i = 0; i < OUTPUT_CHANNELS; i++)
      {
        _driver.write(i, _model.state.outputUs[i]);
      }
      _driver.apply();
    }

    bool _stop(void)
    {
      if(!_model.isActive(MODE_ARMED)) return true;
      if(_model.config.mixerType == FRAME_QUAD_X && _model.isActive(FEATURE_MOTOR_STOP) && _model.state.inputUs[AXIS_THRUST] < _model.config.input.minCheck) return true;
      return false;
      //return !_model.isActive(MODE_ARMED) || (_model.isActive(FEATURE_MOTOR_STOP) && _model.state.inputUs[AXIS_THRUST] < _model.config.inputMinCheck);
    }

    Model& _model;
    EscDriver _driver;
};

}

#endif
