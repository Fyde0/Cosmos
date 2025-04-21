#pragma once

#include <algorithm>

class Envelope {
public:
  Envelope() {}
  ~Envelope() {}

  void Init(float sr) {
    sr_ = sr;
    stageTime_ = 0.0f;
    stageTimeInc_ = 1.0f / sr_; // samples in one second
    stage_ = 0;
    attack_ = 0.1f; // seconds
    decay_ = 1.0f;  // seconds
    curve_ = 0.0f;
    scale_ = 1.0f;
    out_ = 0.0f;
  }

  // 0.001sec to 10sec
  inline void SetAttack(float attack) {
    attack_ = std::min(std::max(attack, 0.001f), 10.0f);
  }

  inline void SetDecay(float decay) {
    decay_ = std::min(std::max(decay, 0.001f), 10.0f);
  }

  inline void Trigger() {
    if (out_ == 0.0f) {
      stageTime_ = 0.0f;
    } else {
      // retriggers, to avoid click
      stageTime_ = out_ * attack_;
    }
    stage_ = 1;
  }

  float Process() {
    // attack
    if (stage_ == 1) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / attack_;
      // TODO apply curve here
      // end of attack, go to decay
      if (out_ >= 1.0f) {
        stageTime_ = 0.0f;
        stage_ = 2;
      }
    }

    if (stage_ == 2) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / decay_;
      out_ = 1.0f - out_;
      // TODO apply curve here
      // end of decay, stop
      if (out_ <= 0.0001f) {
        out_ = 0.0f;
        stage_ = 0;
      }
    }
    // TODO apply scale here
    return out_;
  }

private:
  uint8_t stage; // OFF 0, ATTACK 1, DECAY 2
  float sr_, stageTime_, stageTimeInc_, stage_, attack_, decay_, curve_, scale_,
      out_;
};
