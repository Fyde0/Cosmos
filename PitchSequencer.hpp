#pragma once

#include "Quantizer.hpp"
#include <vector>

class PitchSequencer {
public:
  PitchSequencer() {}
  ~PitchSequencer() {}

  void Init(uint8_t steps) {
    steps_ = steps;
    quant_.Init();
    currentStep_ = 0;
    transpose_ = 0;
  }

  void Advance() {
    // this way is better than % apparently
    if (++currentStep_ > steps_ - 1) {
      currentStep_ = 0;
    }
  }

  void SetCurrentStep(uint8_t step) { currentStep_ = step; }
  void SetNote(uint8_t step, uint8_t note) { sequenceNote_[step] = note; }
  void SetTranspose(int8_t transpose) { transpose_ = transpose; }

  uint8_t GetCurrentStep() const { return currentStep_; }
  float GetCurrentNoteHertz() {
    return quant_.NoteToHertz(sequenceNote_[currentStep_] + transpose_);
  }
  int8_t GetTranspose() const { return transpose_; }

  const char *StepToName(uint8_t step) {
    return quant_.NoteToName(sequenceNote_[step] + transpose_);
  }

private:
  uint8_t steps_;
  Quantizer quant_;
  uint8_t currentStep_;
  std::vector<uint8_t> sequenceNote_;
  int8_t transpose_;
};