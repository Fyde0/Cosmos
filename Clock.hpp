// Learned, written and changed from DaisySP

#pragma once
#include "utilities.hpp"

class Clock {
public:
  Clock() {}
  ~Clock() {}

  void Init(float freq, float sr) {
    freq_ = freq;
    multIndex_ = 5; // x1
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

  float GetBpm() { return freq_ * 60.f; }

  void SetFreq(float freq) {
    freq_ = freq;
    phaseIncr_ = calcPhaseIncr();
  };

  void SetMult(uint8_t multIndex) {
    // clamp
    // value = (value < min) ? min : (value > max ? max : value);
    uint8_t maxValue = sizeof(mults_) / sizeof(mults_[0]) - 1; // array size
    multIndex_ =
        (multIndex < 0) ? 0 : (multIndex > maxValue ? maxValue : multIndex);
    mult_ = mults_[multIndex_];
    phaseIncr_ = calcPhaseIncr();
  };

  /**
   * Sets phase to the end so that the next process will be a reset
   */
  void SetPhaseToEnd() { phase_ = TWOPI_F - phaseIncr_; };

  /**
   * Clock multiplier character for printing on screen
   */
  const char *GetMultChar() { return multChar_[multIndex_]; }

private:
  float freq_, mult_, phase_, sr_, phaseIncr_;
  uint8_t multIndex_;
  float mults_[11] = {1.0f / 16, 1.0f / 8, 1.0f / 4, 1.0f / 3, 1.0f / 2, 1.0f,
                      2.0f,      3.0f,     4.0f,     8.0f,     16.0f};
  const char *multChar_[11] = {"/16", "/8", "/4", "/3", "/2", "",
                               "x2",  "x3", "x4", "x8", "x16"};

  // calculates phase increment per sample
  float calcPhaseIncr() { return (TWOPI_F * freq_ * mult_) / sr_; };
};
