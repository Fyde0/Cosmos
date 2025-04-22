#pragma once

#include "utilities.hpp"

class Filter {
public:
  Filter() {}
  ~Filter() {}

  void Init(float sr);
  float Process(float in);

  void SetFreq(float freq);
  void SetQ(float q);

  float GetFreq();
  float GetQ();

private:
  const float minFreq_ = 20.0f;
  const float maxFreq_ = 20000.0f;
  const float minQ_ = 0.2f;
  const float maxQ_ = 5.0f;
  float sr_, freqIndex_, qIndex_, out_;

  float x[3]{};
  float y[3]{};

  float b0, ib0;
  float w0, cosw0, alpha;

  // filter coefficients lookup table
  // lookup table size
  static constexpr int coeffFreqSteps_ = 512;
  static constexpr int coeffQSteps_ = 32;
  // table struct
  struct FilterCoeffs {
    float a0, a1, a2;
    float b1, b2;
  };
  // table
  static FilterCoeffs coeffTable_[coeffQSteps_][coeffFreqSteps_];
  // generate lookup table
  void InitLookupTable();
  // get coefficients from index
  FilterCoeffs GetNearestCoeffs(float freqIndex, float qIndex);
};
