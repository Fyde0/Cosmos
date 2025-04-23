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
    sequenceHertz_.resize(steps_, 440.f);
  }

  void Advance() {
    // this way is better than % apparently
    if (++currentStep_ > steps_ - 1) {
      currentStep_ = 0;
    }
  }

  void SetCurrentStep(uint8_t step) { currentStep_ = step; }
  void SetNote(uint8_t step, uint8_t note) {
    sequenceNote_[step] = quant_.QuantizeNote(note);
    sequenceHertz_[step] = quant_.NoteToHertz(note, true);
  }

  uint8_t GetCurrentStep() const { return currentStep_; }
  float GetCurrentNoteHertz() const { return sequenceHertz_[currentStep_]; }

  const char *StepToName(uint8_t step) {
    return quant_.NoteToName(sequenceNote_[step]);
  }

private:
  uint8_t steps_;
  Quantizer quant_;
  uint8_t currentStep_;
  std::vector<float> sequenceNote_;
  std::vector<float> sequenceHertz_;
};