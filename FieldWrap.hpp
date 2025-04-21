#pragma once

#include <daisy_field.h>

using namespace daisy;

class FieldWrap {
public:
  FieldWrap() {}

  void Init(AudioHandle::AudioCallback cb) {
    field_.Init();
    field_.SetAudioBlockSize(32);
    field_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    field_.StartAdc();
    field_.StartAudio(cb);
    // zero LEDs
    field_.led_driver.SwapBuffersAndTransmit();
  }

  /**
   * DISPLAY
   */
  void ClearDisplay() { field_.display.Fill(false); }
  void UpdateDisplay() { field_.display.Update(); }

  /**
   * Prints the CPU usage on screen
   *
   * @param usage CPU usage percentage as float
   * @param x X position as uint8_t
   * @param y Y position as uint8_t
   */
  void PrintCPU(float usage, uint8_t x, uint8_t y) {
    FixedCapStr<8> cpu("CPU:");
    cpu.AppendInt(static_cast<int>(usage));
    cpu.Append("%");
    field_.display.SetCursor(x, y);
    field_.display.WriteString(cpu, Font_6x8, true);
  }

  /**
   * LEDs
   */

  void ToggleKeyLed(uint8_t i) {
    keyLedsStates_[i] = !keyLedsStates_[i];
    keysLedsChanged_ = true;
  }

  void BlinkKeyLed(uint8_t i) {
    ToggleKeyLed(i);
    keyLedsBlinking[i] = true;
    blinking_ = true;
  }

  /**
   * Processes and applies current status of LEDs
   *
   * @param stepTime current step time to calculate blinking time
   */
  void ProcessLeds(uint32_t stepTime) {
    if (stepTime >= blinkingTime_ and blinking_) {
      for (size_t i = 0; i < 16; i++) {
        if (keyLedsBlinking[i]) {
          ToggleKeyLed(i);
          keyLedsBlinking[i] = false;
        }
      }
      blinking_ = false;
    }
    // only apply changes if there were any
    if (keysLedsChanged_) {
      for (size_t i = 0; i < 16; i++) {
        field_.led_driver.SetLed(keyLeds_[i],
                                 static_cast<float>(keyLedsStates_[i]));
      }
      field_.led_driver.SwapBuffersAndTransmit();
      keysLedsChanged_ = false;
    }
  }

  /**
   * CONTROLS
   */

  void ProcessAllControls() {
    field_.ProcessAllControls();
    // process knobs, check for tolerance
    for (size_t i = 0; i < 8; i++) {
      float tempKnobValue = field_.knob[i].Process();
      if (tempKnobValue < 0.0f) {
        tempKnobValue = 0.0f;
      }
      if (fabsf(tempKnobValue - knobValues_[i]) > knobTolerance_) {
        knobValues_[i] = tempKnobValue;
        knobChanged_[i] = true;
        return;
      }
      knobChanged_[i] = false;
    }
  }

  // keys

  bool KeyboardRisingEdge(uint8_t i) {
    if (field_.KeyboardRisingEdge(i)) {
      currentTime_ = System::GetNow();
      if (currentTime_ - lastDebounceTime_ >= debounceDelay_) {
        lastDebounceTime_ = currentTime_;
        return true;
      }
    }
    return false;
  }

  char GetKeyGroup(uint8_t key) {
    if (key >= 8 and key <= 15) {
      return 'A';
    }
    return 'B';
  }

  // switches

  bool SwitchRisingEdge(uint8_t i) {
    if (i == 1) {
      return field_.GetSwitch(DaisyField::SW_1)->RisingEdge();
    } else {
      return field_.GetSwitch(DaisyField::SW_2)->RisingEdge();
    }
  }

  // knobs

  float ScaleKnob(int i, float minOutput, float maxOutput) {
    return (((knobValues_[i] - minKnob_) / (maxKnob_ - minKnob_)) *
            (maxOutput - minOutput)) +
           minOutput;
  }

  bool DidKnobChange(uint8_t i) { return knobChanged_[i]; }
  float GetKnobValue(uint8_t i) { return knobValues_[i]; }
  float GetKnobValueInHertz(uint8_t i) {
    // notes from 21 to 108 (A0 to C8)
    uint8_t note = static_cast<int>(ScaleKnob(i, 21, 108));
    // 440 * 2^((note - 69)/12)
    return 440.0f * pow(2.0, (static_cast<float>(note) - 69.0) / 12.0);
  }

  // getter for passthrough
  daisy::DaisyField &Field() { return field_; }

private:
  DaisyField field_;

  /**
   * LEDs
   */

  size_t keyLeds_[16] = {
      DaisyField::LED_KEY_A1, DaisyField::LED_KEY_A2, DaisyField::LED_KEY_A3,
      DaisyField::LED_KEY_A4, DaisyField::LED_KEY_A5, DaisyField::LED_KEY_A6,
      DaisyField::LED_KEY_A7, DaisyField::LED_KEY_A8, DaisyField::LED_KEY_B1,
      DaisyField::LED_KEY_B2, DaisyField::LED_KEY_B3, DaisyField::LED_KEY_B4,
      DaisyField::LED_KEY_B5, DaisyField::LED_KEY_B6, DaisyField::LED_KEY_B7,
      DaisyField::LED_KEY_B8};

  bool keyLedsStates_[16];
  bool keyLedsBlinking[16];
  // this is so we apply changes only if there were any
  bool keysLedsChanged_ = false;
  // how long to blink LEDs for, in AudioCallback iterations
  uint8_t blinkingTime_ = 100;
  bool blinking_ = false;

  /**
   * CONTROLS
   */

  // keys
  uint32_t currentTime_;
  uint32_t lastDebounceTime_;
  uint8_t debounceDelay_ = 32;
  // knobs
  const float knobTolerance_ = 0.001f;
  const float minKnob_ = 0.000396f;
  const float maxKnob_ = 0.968734f;
  float knobValues_[8];
  bool knobChanged_[8];
};