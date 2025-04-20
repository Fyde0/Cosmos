// Learned, written and changed from DaisySP

#pragma once
#include "utilities.hpp"

class Clock {
public:
  Clock() {}
  ~Clock() {}

  void Init(float freq, float sr) {
    freq_ = freq;
    mult_ = 1.0f;
    phase_ = 0.0f;
    sr_ = sr;
    phaseIncr_ = calcPhaseIncr();
  };

  /**
   * Moves the clock forward
   * Tick happens when the phase reaches TWOPI
   *
   * @return bool true if tick happened
   */
  bool Process() {
    phase_ += phaseIncr_;
    if (phase_ >= TWOPI_F) {
      phase_ -= TWOPI_F;
      return true;
    }
    return false;
  };

  void SetFreq(float freq) {
    freq_ = freq;
    phaseIncr_ = calcPhaseIncr();
  };

  void SetMult(float mult) {
    mult_ = (int)round(mult);
    phaseIncr_ = calcPhaseIncr();
  };

  /**
   * Sets phase to the end so that the next process will be a reset
   */
  void SetPhaseToEnd() { phase_ = TWOPI_F - phaseIncr_; };

private:
  float freq_, mult_, phase_, sr_, phaseIncr_;

  // calculates phase increment per sample
  float calcPhaseIncr() { return (TWOPI_F * freq_ * mult_) / sr_; };
};
