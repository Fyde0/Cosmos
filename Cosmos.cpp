#include "Clock.hpp"
#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Filter.hpp"
#include "Oscillator.hpp"
#include "PitchSequencer.hpp"
#include "TriggerSequencer.hpp"
#include "daisy_field.h"

#define MAIN_DELAY 5 // ms, main loop iteration time (separate from audio)
#define DISPLAY_UPDATE_DELAY 10 // update display every x main iterations
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
Envelope env2;

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
//
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  // for CPU %
  uint32_t start = System::GetTick();

  // midi clock (this is set in main)
  hw.ProcessMidiClock();

  // buffer loop
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
          env2.Trigger();
          // set oscillator frequency
          osc.SetFreq(pitchSeq.GetCurrentNoteHertz());
        }
      }
    }

    float env1Out = env1.Process();
    float env2Out = env2.Process();
    osc.SetAmp(env1Out);
    osc.Process(&out1, &out2);
    filter1.AddFreq(env2Out);
    filter2.AddFreq(env2Out);
    out1 = filter1.Process(out1 * 0.50f);
    out2 = filter2.Process(out2 * 0.50f);

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
  hw.InitMidi();
  clock.Init(2, hw.Field().AudioSampleRate());
  seq1.Init(8);
  seq2.Init(8);
  pitchSeq.Init(8);
  osc.Init(hw.Field().AudioSampleRate());
  osc.SetMode(Oscillator::MODE_SAW);
  filter1.Init(hw.Field().AudioSampleRate());
  filter2.Init(hw.Field().AudioSampleRate());
  env1.Init(hw.Field().AudioSampleRate());
  env2.Init(hw.Field().AudioSampleRate());

  // shift buttons
  bool shift1 = false;
  bool shift2 = false;
  // main loop iterations
  uint8_t mainCount = 0;
  while (1) {
    // count main loop iterations
    ++mainCount;

    // set bpm from midi
    if (hw.UsingMidiClock()) {
      clock.SetFreq(hw.GetMidiClock() / 60.0f);
    }

    /**
     * CONTROLS
     */

    hw.ProcessAllControls();

    // switches
    shift1 = hw.SwitchPressed(1);
    shift2 = hw.SwitchPressed(2);

    // keys
    if (shift1 && !shift2) {
      // shift1 + A1, play/pause
      if (hw.KeyboardRisingEdge(8)) {
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
    }

    // no shift, toggle steps
    if (!shift2 && !shift1) {
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
    }

    // knobs
    for (size_t i = 0; i < 8; i++) {
      // do stuff only if the knob was moved
      if (hw.DidKnobChange(i)) {
        // shift 1
        if (shift1 && !shift2) {
          switch (i) {
          case 0:
            // knob 1, bpm (from 20 to 220), only if there's no midi clock
            if (!hw.UsingMidiClock()) {
              clock.SetFreq(static_cast<int>(hw.ScaleKnob(i, 20, 220.9)) /
                            60.f);
            }
            break;
          case 1:
            // knob 2, bpm mult
            clock.SetMult(static_cast<int>(hw.ScaleKnob(i, 0, 10.9f)));
            break;
          }
        }
        // shift 2, change notes
        if (shift2 && !shift1) {
          // notes are from 21 to 108, see Quantizer class
          pitchSeq.SetNote(i, static_cast<int>(hw.ScaleKnob(i, 21, 108)));
        }
        // no shift
        if (!shift1 && !shift2) {
          // BPM, Mult, Freq, Q,
          switch (i) {
          case 0:
            // knob 1, transpose?
            break;
          case 1:
            // knob 2, env1 attack
            env1.SetAttack(hw.ScaleKnob(i, 0.001f, 5.0f));
            break;
          case 2:
            // knob 3, env1 decay
            env1.SetDecay(hw.ScaleKnob(i, 0.001f, 5.0f));
            break;
          case 3:
            // knob 4, filter frequency
            filter1.SetFreq(hw.ScaleKnob(i, 0.0f, 1.0f));
            filter2.SetFreq(hw.ScaleKnob(i, 0.0f, 1.0f));
            break;
          case 4:
            // knob 5, filter q
            filter1.SetQ(hw.ScaleKnob(i, 0.0f, 1.0f));
            filter2.SetQ(hw.ScaleKnob(i, 0.0f, 1.0f));
            break;
          case 5:
            // knob 6, env2 attack
            env2.SetAttack(hw.ScaleKnob(i, 0.001f, 5.0f));
            break;
          case 6:
            // knob 7, env2 decay
            env2.SetDecay(hw.ScaleKnob(i, 0.001f, 5.0f));
            break;
          case 7:
            // knob 8, env2 scale
            env2.SetScale(hw.ScaleKnob(i, 0.0f, 1.0f));
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
      hw.PrintShift(1, shift1, 0, 56);
      hw.PrintShift(2, shift2, 86, 56);

      // print sequence to screen
      for (size_t i = 0; i < 8; i++) {
        uint8_t xPos = i * 30 + 6; // + offset to center
        uint8_t yPos = 15;
        // invert color if step is active
        bool color = !(seq1.IsStepActive(i));
        // values for second row
        if (i > 3) {
          xPos = xPos - (4 * 30);
          yPos = yPos + 10;
        }
        // if step is playing use [ ]
        FixedCapStr<4> noteStr("");
        (play && seq1.GetCurrentStep() == i) ? noteStr.Append("[")
                                             : noteStr.Append(" ");
        noteStr.Append(pitchSeq.StepToName(i));
        (play && seq1.GetCurrentStep() == i) ? noteStr.Append("]")
                                             : noteStr.Append(" ");
        hw.Field().display.SetCursor(xPos, yPos);
        hw.Field().display.WriteString(noteStr, Font_6x8, color);
      }

      // FixedCapStr<32> var("Midi BPM: ");
      // if (hw.UsingMidiClock()) {
      //   var.AppendInt(hw.GetMidiClock());
      // } else {
      //   var.Append("no");
      // }
      // hw.Field().display.SetCursor(0, 40);
      // hw.Field().display.WriteString(var, Font_6x8, true);
      // FixedCapStr<64> var2("");
      // hw.Field().display.SetCursor(0, 50);
      // hw.Field().display.WriteString(var2, Font_6x8, true);

      hw.UpdateDisplay();
    }

    if (mainCount % LEDS_UPDATE_DELAY == 0) {
      hw.ProcessLeds(stepTime);
    }

    System::Delay(MAIN_DELAY);
  }
}
