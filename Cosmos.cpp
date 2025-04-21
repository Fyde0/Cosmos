#include "Clock.hpp"
#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Oscillator.hpp"
#include "PitchSequencer.hpp"
#include "TriggerSequencer.hpp"
#include "daisy_field.h"

#define MAIN_DELAY 5 // ms, main loop iteration time (separate from audio)
#define DISPLAY_UPDATE_DELAY 20 // update display every x main iterations
#define LEDS_UPDATE_DELAY 2     // update LEDs every x main iterations

using namespace daisy;

FieldWrap hw;

Clock clock;
TriggerSequencer seq1;
TriggerSequencer seq2;
PitchSequencer pitchSeq;
Oscillator osc;
Envelope env1;

// play/pause
bool play = false;

/**
 * AUDIO CALLBACK
 */

// audio
float out1, out2;

float cpuUsage = 0.f;
uint16_t stepTime = 0;
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  // for CPU %
  uint32_t start = System::GetTick();

  for (size_t i = 0; i < size; i++) {
    if (play) {

      if (clock.Process()) {

        // sequencers
        seq1.Advance();
        seq2.Advance();
        pitchSeq.Advance();
        // seq2 resets seq1
        if (seq2.IsCurrentStepActive()) {
          seq1.SetCurrentStep(0);
        }
        // seq1 = group A = 7 to 15, makes no sense
        hw.BlinkKeyLed(seq1.GetCurrentStep() + 8);
        hw.BlinkKeyLed(seq2.GetCurrentStep());

        // set oscillator frequency
        osc.SetFreq(pitchSeq.GetCurrentPitch());

        // start step
        stepTime = 0;

        if (seq1.IsCurrentStepActive()) {
          env1.Trigger();
        }
      }

      osc.SetAmp(env1.Process());
      osc.Process(&out1, &out2);

      out2 = out1;

      out[0][i] = out1;
      out[1][i] = out2;

    } else {
      out[0][i] = 0.f;
      out[1][i] = 0.f;
    }
  }

  stepTime++;

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
  pitchSeq.Init(8);
  osc.Init(hw.Field().AudioSampleRate());
  osc.SetMode(Oscillator::MODE_SIN);
  env1.Init(hw.Field().AudioSampleRate());

  // main loop iterations
  uint8_t mainCount = 0;
  while (1) {
    // count main loop iterations
    ++mainCount;

    /**
     * CONTROLS
     */

    hw.ProcessAllControls();

    // Keys
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

    // Switches
    // Switch 1, Play/Pause
    if (hw.SwitchRisingEdge(1)) {
      if (play) {
        play = !play; // stop
      } else {
        // reset all seqs
        // sequencer advances before step is processed,
        // so you need to set it to the last step when starting
        seq1.SetCurrentStep(7);
        seq2.SetCurrentStep(7);
        pitchSeq.SetCurrentStep(7);
        // set phase to end so that you don't have to wait for the next tick
        clock.SetPhaseToEnd();
        stepTime = 0;
        play = !play;
      }
    }

    // Knobs
    for (size_t i = 0; i < 8; i++) {
      if (hw.DidKnobChange(i)) {
        pitchSeq.SetPitch(i, hw.GetKnobValueInHertz(i));
      }
    }

    /**
     * UI
     */

    // only update screen every x iterations
    if (mainCount % DISPLAY_UPDATE_DELAY == 0) {

      hw.ClearDisplay();

      hw.PrintCPU(cpuUsage, 86, 0);

      // FixedCapStr<8> step("Step:");
      // step.AppendInt(seq1.GetCurrentStep());
      // hw.Field().display.SetCursor(0, 10);
      // hw.Field().display.WriteString(step, Font_6x8, true);

      // FixedCapStr<32> var("");
      // var.AppendFloat(hw.GetKnobValue(0));
      // hw.Field().display.SetCursor(0, 20);
      // hw.Field().display.WriteString(var, Font_6x8, true);

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
