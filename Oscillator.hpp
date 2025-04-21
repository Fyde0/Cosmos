#pragma once
#include "utilities.hpp"

class Oscillator {
public:
  Oscillator() {}
  ~Oscillator() {}

  // LAST to make it easier for checks
  enum { MODE_SIN, MODE_TRI, MODE_SAW, MODE_LAST };

  void Init(float sr) {
    sr_ = sr;
    freq_ = 440.0f;
    amp_ = 0.5f;
    phase_ = 0.0f;
    phaseInc_ = 0.0f;
    mode_ = MODE_SIN;
    params_[0] = 0.0f;
    params_[1] = 0.0f;
    params_[2] = 0.0f;

    calcPhaseInc();
    calcSineVars();
  }

  inline void SetFreq(float f) {
    freq_ = f;
    calcPhaseInc();
    calcSineVars();
  }

  inline void SetAmp(float a) { amp_ = a; }

  inline void SetParam(uint8_t param, float value) { params_[param] = value; }

  inline void SetMode(uint8_t mode) {
    // check if mode number is not outside the list
    mode_ = mode < MODE_LAST ? mode : MODE_SIN;
  }

  void Process(float *out1, float *out2) {
    switch (mode_) {

    case MODE_SIN:
      *out1 = b1 * y1 - y2;
      y2 = y1;
      y1 = *out1;
      *out2 = *out1;
      break;
      // c++ sinf uses a bunch more CPU
      // *out1 = sinf(phase_ * TWOPI_F);
      // *out2 = *out1;
      // break;

    case MODE_TRI:
      *out1 = (2.0f * phase_) - 1.0f;
      // absolute value of saw = triangle
      *out1 = 2.0f * (fabsf(*out1) - 0.5f);
      *out2 = *out1;
      break;

    case MODE_SAW:
      *out1 = (2.0f * phase_) - 1.0f;
      *out1 -= polyBLEP(phase_, phaseInc_);
      *out2 = *out1;
      break;

    default:
      *out1 = 0.0f;
      *out2 = 0.0f;
      break;
    }

    phase_ += phaseInc_;
    if (phase_ > 1.0f) {
      phase_ -= 1.0f; // why -? wrap maybe?
    }

    *out1 = *out1 * amp_;
    *out2 = *out2 * amp_;
  }

private:
  uint8_t mode_;
  float sr_, freq_, amp_, phase_, phaseInc_;
  float params_[3];

  inline void calcPhaseInc() { phaseInc_ = freq_ * (1.0f / sr_); }

  float w, y1, y2, b1;
  void calcSineVars() {
    // from musicdsp, 9-fast-sine-wave-calculation.html
    // must use 2pi here, but not in other waves for some reason
    w = freq_ * (TWOPI_F / sr_);
    y1 = sin(0.0f - w);
    y2 = sin(0.0f - 2 * w);
    b1 = 2.0f * cos(w);
  }

  float t, dt;
  float polyBLEP(float phase, float phaseInc) {
    // t is usually divided by 2pi because
    // it usually goes from 0 to 2pi, but here it
    // goes from 0 to 1, I guess?
    // It doesn't work if I use 2pi
    t = phase;
    dt = phaseInc;
    // beginning of wave
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0f; // adds some sort of smoothing?
    } // don't really understand
    // end of wave
    else if (t > 1.0f - dt) {
      t = (t - 1.0f) / dt;
      return t * t + t + t + 1.0f;
    } else {
      return 0.0f;
    }
  }
};
