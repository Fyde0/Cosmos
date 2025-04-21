#include "Clock.hpp"
#include "FieldWrap.hpp"
#include "Sequencer.hpp"
#include "daisy_field.h"

#define MAIN_DELAY 5 // ms, main loop iteration time (separate from audio)
#define DISPLAY_UPDATE_DELAY 20 // update display every x main iterations
#define LEDS_UPDATE_DELAY 2     // update LEDs every x main iterations

using namespace daisy;

FieldWrap hw;

Clock clock;
Sequencer seq1;
Sequencer seq2;

/**
 * AUDIO CALLBACK
 */

float cpuUsage = 0.f;
uint32_t stepTime = 0;
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  // for CPU %
  uint32_t start = System::GetTick();

  stepTime++;

  for (size_t i = 0; i < size; i++) {

    if (clock.Process()) {

      // sequencers
      seq1.Advance();
      seq2.Advance();
      // seq2 resets seq1
      if (seq2.IsCurrentStepActive()) {
        seq1.SetCurrentStep(0);
      }
      // seq1 = group A = 7 to 15, makes no sense
      hw.BlinkKeyLed(seq1.GetCurrentStep() + 8);
      hw.BlinkKeyLed(seq2.GetCurrentStep());

      // start step
      stepTime = 0;
    }
  }

  cpuUsage += 0.03f * (((System::GetTick() - start) / 200.f) - cpuUsage);
}

/**
 * MAIN
 */

int main(void) {

  // Init stuff
  hw.Init(AudioCallback);

  clock.Init(2, hw.Field().AudioSampleRate());
  seq1.Init(8);
  seq2.Init(8);

  // main loop iterations
  uint8_t mainCount = 0;
  while (1) {
    // count main loop iterations
    ++mainCount;

    /**
     * CONTROLS
     */

    hw.ProcessAllControls();

    for (size_t i = 0; i < 16; ++i) {
      if (hw.KeyboardRisingEdge(i)) {
        if (hw.GetKeyGroup(i) == 'A') {
          seq1.ToggleStep(i - 8);
        }
        if (hw.GetKeyGroup(i) == 'B') {
          seq2.ToggleStep(i);
        }
        hw.ToggleKeyLed(i);
      }
    }

    // only update screen every x iterations
    if (mainCount % DISPLAY_UPDATE_DELAY == 0) {

      hw.ClearDisplay();

      hw.PrintCPU(cpuUsage, 0, 0);

      FixedCapStr<8> step("Step:");
      step.AppendInt(seq1.GetCurrentStep());
      hw.Field().display.SetCursor(0, 10);
      hw.Field().display.WriteString(step, Font_6x8, true);

      // FixedCapStr<16> time("???:");
      // time.AppendInt(mainCount);
      // hw.Field().display.SetCursor(0, 20);
      // hw.Field().display.WriteString(time, Font_6x8, true);

      // hw.Field().display.SetCursor(0, 20);
      // hw.Field().display.WriteString(seq1.GetSequenceString(), Font_6x8,
      // true); hw.Field().display.SetCursor(0, 30);
      // hw.Field().display.WriteString(seq2.GetSequenceString(), Font_6x8,
      // true);

      hw.UpdateDisplay();
    }

    if (mainCount % LEDS_UPDATE_DELAY == 0) {
      hw.ProcessLeds(stepTime);
    }

    System::Delay(MAIN_DELAY);
  }
}
