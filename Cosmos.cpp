#include "Clock.hpp"
#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Filter.hpp"
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
Filter filter1;
Filter filter2;
Envelope env1;

// play/pause
bool play = false;

/**
 * AUDIO CALLBACK
 */

// audio
float out1, out2;
// for CPU %
float cpuUsage = 0.f;
// count step time for blinking LEDs
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
          pitchSeq.SetCurrentStep(0);
        }
        // seq1 = group A = 7 to 15, makes no sense
        hw.BlinkKeyLed(seq1.GetCurrentStep() + 8);
        hw.BlinkKeyLed(seq2.GetCurrentStep());

        // start step
        stepTime = 0;

        if (seq1.IsCurrentStepActive()) {
          env1.Trigger();
          // set oscillator frequency
          osc.SetFreq(pitchSeq.GetCurrentNoteHertz());
        }
      }
    }

    float envOut = env1.Process();
    osc.SetAmp(envOut);
    osc.Process(&out1, &out2);
    out1 = filter1.Process(out1);
    out2 = filter2.Process(out2);

    out[0][i] = out1;
    out[1][i] = out2;
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
  osc.SetMode(Oscillator::MODE_SAW);
  filter1.Init(hw.Field().AudioSampleRate());
  filter2.Init(hw.Field().AudioSampleRate());
  env1.Init(hw.Field().AudioSampleRate());

  // shift button
  bool shift = false;
  // main loop iterations
  uint8_t mainCount = 0;
  while (1) {
    // count main loop iterations
    ++mainCount;

    /**
     * CONTROLS
     */

    hw.ProcessAllControls();

    // keys
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

    // switches
    // switch 1, play/pause
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
    // switch 2, shift
    shift = hw.SwitchPressed(2);

    // knobs
    for (size_t i = 0; i < 8; i++) {
      // do stuff only if the knob was moved
      if (hw.DidKnobChange(i)) {
        // change notes if shift is pressed
        if (shift) {
          // notes are from 0 to 87, see Quantizer class
          pitchSeq.SetNote(i, static_cast<int>(hw.ScaleKnob(i, 0, 87)));
        } else {
          // BPM, Mult, Freq, Q,
          switch (i) {
          case 0:
            // knob 1, bpm (from 20 to 220)
            clock.SetFreq(hw.ScaleKnob(i, 20, 220) / 60.f);
            break;
          case 1:
            // knob 2, bpm mult
            clock.SetMult(static_cast<int>(hw.ScaleKnob(i, 0, 10.9f)));
            break;
          case 2:
            // knob 3, filter frequency
            filter1.SetFreq(20.f * pow(1000.0f, hw.ScaleKnob(i, 0.0f, 1.0f)));
            filter2.SetFreq(20.f * pow(1000.0f, hw.ScaleKnob(i, 0.0f, 1.0f)));
            break;
          case 3:
            // knob 4, filter q
            filter1.SetQ(hw.ScaleKnob(i, 0.2f, 5.0f));
            filter2.SetQ(hw.ScaleKnob(i, 0.2f, 5.0f));
            break;
          }
        }
      }
    }

    /**
     * UI
     */

    // only update screen every x iterations
    if (mainCount % DISPLAY_UPDATE_DELAY == 0) {

      hw.ClearDisplay();

      hw.PrintBPM(clock.GetBpm(), clock.GetMultChar(), 0, 0);
      hw.PrintCPU(cpuUsage, 86, 0);
      hw.PrintShift(shift, 0, 56);

      // FixedCapStr<32> var("");
      // var.AppendFloat(hw.ScaleKnob(1, 0, 10.9f));
      // hw.Field().display.SetCursor(0, 20);
      // hw.Field().display.WriteString(var, Font_6x8, true);

      hw.UpdateDisplay();
    }

    if (mainCount % LEDS_UPDATE_DELAY == 0) {
      hw.ProcessLeds(stepTime);
    }

    System::Delay(MAIN_DELAY);
  }
}
