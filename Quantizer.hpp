#pragma once

#include <map>
#include <string>

class Quantizer {
public:
  Quantizer() {}
  ~Quantizer() {}

  void Init() {
    qKey_ = 3; // C
    qScale_ = 3;
  }

  void SetKey(uint8_t key) { qKey_ = key; }

  void SetScale(uint8_t scale) { qScale_ = scale; }

  uint8_t QuantizeNote(uint8_t note) {
    while (true) {
      for (uint8_t increment : qScales[qScale_]) {
        // given note - current key modulo 12
        if ((note - qKey_) % 12 == increment) {
          return note;
        }
      }
      note++;
    }
  }

  float NoteToHertz(uint8_t note, bool quant = false) {
    if (quant) {
      note = QuantizeNote(note);
    }
    // midi note to hertz, +21 because I start from A0 (note 21)
    return 440.0f * pow(2.0f, (note + 21.0f - 69.0f) / 12.0f);
  }

  const char *NoteToName(uint8_t note) { return notes[note % 12]; }

private:
  // midi notes from 21 to 108
  //                 0  to 87
  // freq = 440⋅2^(n−69)/12
  // A  A# B  C  C# D  D# E  F  F# G  G#
  // 0  1  2  3  4  5  6  7  8  9  10 11
  uint8_t qKey_, qScale_;

  const char *notes[12] = {"A ", "A#", "B ", "C ", "C#", "D ",
                           "D#", "E ", "F ", "F#", "G ", "G#"};

  std::map<int, std::array<int, 12>> qScales = {
      // I'm sure there's a better way to do this
      {0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}}, // Chromatic
      {1, {0, 2, 4, 5, 7, 9, 11, 0, 0, 0, 0, 0}},  // Ionian (Natural Major)
      {2, {0, 2, 4, 7, 9, 0, 0, 0, 0, 0, 0, 0}},   // Pentatonic Major
      {3, {0, 2, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0}},  // Aeolian (Natural Minor)
      {4, {0, 2, 3, 5, 7, 8, 11, 0, 0, 0, 0, 0}},  // Harmonic Minor
      {5, {0, 2, 3, 5, 7, 9, 11, 0, 0, 0, 0, 0}},  // Melodic Minor
      {6, {0, 3, 5, 7, 10, 0, 0, 0, 0, 0, 0, 0}},  // Pentatonic Minor
      {7, {0, 2, 3, 5, 7, 9, 10, 0, 0, 0, 0, 0}},  // Dorian
      {8, {0, 1, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0}},  // Phrygian
      {9, {0, 2, 4, 6, 7, 9, 11, 0, 0, 0, 0, 0}},  // Lydian
      {10, {0, 2, 4, 5, 7, 9, 10, 0, 0, 0, 0, 0}}, // Mixolydian
      {11, {0, 1, 3, 5, 6, 8, 10, 0, 0, 0, 0, 0}}, // Locrian
                                                   // Fifth, I, IV, V chords?
  };
};
