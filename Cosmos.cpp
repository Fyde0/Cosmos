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

float cpuUsage = 0.f;
uint32_t stepTime = 0;
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  // for CPU %
  uint32_t start = System::GetTick();

  stepTime++;

  for (size_t i = 0; i < size; i++) {

    if (clock.Process()) {
      seq1.Advance();
      hw.BlinkKeyLed(seq1.GetCurrentStep());
      stepTime = 0;
    }
  }

  cpuUsage += 0.03f * (((System::GetTick() - start) / 200.f) - cpuUsage);
}

int main(void) {
  hw.Init(AudioCallback);

  clock.Init(2, hw.Field().AudioSampleRate());
  seq1.Init(8);
  seq1.ToggleStep(0);
  hw.ToggleKeyLed(0);

  // main loop iterations
  uint8_t mainCount = 0;
  while (1) {
    // count main loop iterations
    ++mainCount;

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

      hw.UpdateDisplay();
    }

    if (mainCount % LEDS_UPDATE_DELAY == 0) {
      hw.ProcessLeds(stepTime);
    }

    System::Delay(MAIN_DELAY);
  }
}
