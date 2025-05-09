#pragma once

#include <vector>

class TriggerSequencer {
public:
  TriggerSequencer() {}
  ~TriggerSequencer() {}

  void Init(uint8_t steps) {
    steps_ = steps;
    currentStep_ = 0;
    sequence_.resize(steps_, false);
  }

  void Advance() {
    // this way is better than % apparently
    if (++currentStep_ > steps_ - 1) {
      currentStep_ = 0;
    }
  }

  void ToggleStep(uint8_t step) { sequence_[step] = !sequence_[step]; }

  void SetCurrentStep(uint8_t step) { currentStep_ = step; }

  uint8_t GetCurrentStep() const { return currentStep_; }
  bool IsStepActive(uint8_t step) { return sequence_[step]; }
  bool IsCurrentStepActive() const { return sequence_[currentStep_]; }

private:
  uint8_t steps_;
  uint8_t currentStep_;
  std::vector<bool> sequence_;
};