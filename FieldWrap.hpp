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
};