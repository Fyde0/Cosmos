#pragma once

#include <vector>

class PitchSequencer {
public:
  PitchSequencer() {}
  ~PitchSequencer() {}

  void Init(uint8_t steps) {
    steps_ = steps;
    currentStep_ = 0;
    sequence_.resize(steps_, 440.f);
  }

  void Advance() {
    // this way is better than % apparently
    if (++currentStep_ > steps_ - 1) {
      currentStep_ = 0;
    }
  }

  void SetCurrentStep(uint8_t step) { currentStep_ = step; }
  void SetPitch(uint8_t step, float pitch) { sequence_[step] = pitch; }

  uint8_t GetCurrentStep() const { return currentStep_; }
  float GetCurrentPitch() const { return sequence_[currentStep_]; }

private:
  uint8_t steps_;
  uint8_t currentStep_;
  std::vector<float> sequence_;
};