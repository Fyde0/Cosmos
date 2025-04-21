#pragma once

// I don't even know where to start commenting this, watch this:
// https://www.youtube.com/playlist?list=PLbqhA-NKGP6Afr_KbPUuy_yIBpPR4jzWo

class Filter {
public:
  Filter() {}
  ~Filter() {}

  void Init(float sr) {
    sr_ = sr;
    freq_ = 261.626f;
    q_ = 0.2f;
    out_ = 0.0f;
    x[0] = x[1] = x[2] = 0.0;
    y[0] = y[1] = y[2] = 0.0;
    calcCoeff();
  }

  inline void SetFreq(float freq) {
    freq_ = std::min(std::max(freq, 20.0f), 20000.0f);
    calcCoeff();
  }

  inline void SetQ(float q) {
    q_ = std::min(std::max(q, 0.2f), 5.0f);
    calcCoeff();
  }

  float Process(float in) {
    x[2] = x[1];
    x[1] = x[0];
    x[0] = in;

    y[2] = y[1];
    y[1] = y[0];
    y[0] = a0 * x[0];
    y[0] += a1 * x[1];
    y[0] += a2 * x[2];
    y[0] -= b1 * y[1];
    y[0] -= b2 * y[2];

    out_ = y[0];

    // out_ = b0*in + b1*lastX1 + b2*lastX2 - a1*lastY1 - a2*lastY2;

    // lastX2 = lastX1;
    // lastX1 = in;
    // lastY2 = lastY1;
    // lastY1 = out_;

    return out_;
  }

private:
  float sr_, freq_, q_, out_;

  float a0 = 0.0;
  float a1 = 0.0;
  float a2 = 0.0;
  float b1 = 0.0;
  float b2 = 0.0;
  float x[3]{};
  float y[3]{};

  float b0, ib0;
  float w0, cosw0, alpha;
  void calcCoeff() {
    w0 = 2.0f * PI_F * (freq_ / sr_);
    cosw0 = cos(w0);
    alpha = sin(w0) / (2.0f * q_);

    b0 = 1.0f + alpha;
    ib0 = 1.0f / b0;

    a0 = ((1.0 - cosw0) / 2.0) * ib0;
    a1 = (1.0 - cosw0) * ib0;
    a2 = ((1.0 - cosw0) / 2.0) * ib0;

    b1 = (-2.0 * cosw0) * ib0;
    b2 = (1.0 - alpha) * ib0;
  }

  // float lastX1, lastX2, lastY1, lastY2;
  // float b0, b1, b2, a1, a2;
  //
  // float k, norm;
  // void calcCoeff(float frequency, float q) {
  //	k = tanf(M_PI * frequency / sr_);
  //	norm = 1.0 / (1 + k/ q + k * k);
  //
  //	b0 = k * k * norm;
  //	b1 = 2.0 * b0;
  //	b2 = b0;
  //	a1 = 2 * (k * k - 1) * norm;
  //	a2 = (1 - k / q + k * k) * norm;
  // }
};
