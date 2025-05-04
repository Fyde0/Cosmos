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

using namespace std;
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
void ResetAllSeqs() {
  // sequencer advances before step is processed,
  // so you need to set it to the last step when starting
  seq1.SetCurrentStep(7);
  seq2.SetCurrentStep(7);
  pitchSeq.SetCurrentStep(7);
  // set phase to end so that you don't have to wait for the next tick
  clock.SetPhaseToEnd();
  stepTime = 0;
}
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  // for CPU %
  uint32_t start = System::GetTick();

  // midi clock (bpm is set in main)
  hw.ProcessMidiClock();

  if (hw.UsingMidiClock()) {
    bool midiIsPlaying = hw.MidiIsPlaying();
    if (!play && midiIsPlaying) {
      ResetAllSeqs();
      play = true;
    }
    if (play && !midiIsPlaying) {
      play = false;
    }
  }

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
  // y position of text rows on screen
  uint8_t row1 = 0;
  uint8_t row2 = 11;
  uint8_t row3 = 19;
  uint8_t row4 = 30;
  uint8_t row5 = 38;
  uint8_t row6 = 48;
  uint8_t row7 = 56;
  // offset text on string to the right to center
  uint8_t screenOffset = 6;
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

    if (shift1 && hw.SwitchRisingEdge(2) && !hw.UsingMidiClock()) {
      if (play) {
        play = false; // stop
      } else {
        ResetAllSeqs();
        play = true;
      }
    }

    // keys

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

    // Knobs
    // 1     2     3     4     5     6     7     8
    // No shifts
    // Tran, Osc?, Osc?, Osc?, EnvD, Freq, Res , FilD?
    // Shift 1
    // BPM , Mult, Key , Scal, EnvA, FilA, FilS, Comp?
    // Shift 2
    // Pitch

    // LFOs in shift 2?
    // Osc mode and filter mode in shift 1?

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
          switch (i) {
          case 0:
            // knob 1, transpose?
            pitchSeq.SetTranspose(
                static_cast<int>(hw.ScaleKnob(i, -24.0f, 24.0f)));
            break;
          case 1:
            // knob 2, env1 attack
            env1.SetAttack(hw.ScaleKnob(i, 0.001f, 5.0f, true));
            break;
          case 2:
            // knob 3, env1 decay
            env1.SetDecay(hw.ScaleKnob(i, 0.001f, 5.0f, true));
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
            env2.SetAttack(hw.ScaleKnob(i, 0.001f, 5.0f, true));
            break;
          case 6:
            // knob 7, env2 decay
            env2.SetDecay(hw.ScaleKnob(i, 0.001f, 5.0f, true));
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

      // print BPM
      string bpmStr = "BPM:" + to_string(static_cast<int>(clock.GetBpm())) +
                      clock.GetMultChar();
      hw.PrintToScreen(bpmStr.c_str(), 0, row1);
      // print CPU usage
      string cpuStr = "CPU:" + to_string(static_cast<int>(cpuUsage)) + "%";
      hw.PrintToScreen(cpuStr.c_str(), 86, row1);
      // print shifts
      if (shift1) {
        hw.PrintToScreen("Shift 1", 0, 56);
      }
      if (shift2) {
        hw.PrintToScreen("Shift 2", 86, 56);
      }

      // print sequence to screen
      for (size_t i = 0; i < 8; i++) {
        uint8_t xPos = i * 30 + screenOffset; // + offset to center
        uint8_t yPos = row2;
        // invert color if step is active
        bool color = !(seq1.IsStepActive(i));
        // values for second row
        if (i > 3) {
          xPos = xPos - (4 * 30);
          yPos = row3;
        }
        // if step is playing use [ ]
        string noteStr = "";
        (play && seq1.GetCurrentStep() == i) ? noteStr += "[" : noteStr += " ";
        noteStr += pitchSeq.StepToName(i);
        (play && seq1.GetCurrentStep() == i) ? noteStr += "]" : noteStr += " ";
        hw.PrintToScreen(noteStr.c_str(), xPos, yPos, color);
      }

      // TODO add switch
      
      // No switches
      string pos1Text = "Trns";
      string pos1Val = to_string(pitchSeq.GetTranspose());
      string pos2Text = "????";
      string pos2Val = "";
      string pos3Text = "????";
      string pos3Val = "";
      string pos4Text = "????";
      string pos4Val = "";
      string pos5Text = "EnvD";
      FixedCapStr<8> pos5Val("");
      pos5Val.AppendFloat(env1.GetDecay());
      string pos6Text = "Freq";
      // format filter frequency
      FixedCapStr<8> pos6Val("");
      float filtFreq = filter1.GetFreq();
      if (filtFreq < 100.f) {
        // eg 50.0
        pos6Val.AppendFloat(filter1.GetFreq(), 1);
      } else if (filtFreq < 10000.f) {
        // eg 250 or 5000
        pos6Val.AppendInt(static_cast<int>(filter1.GetFreq()));
      } else {
        // eg 12k
        pos6Val.AppendInt(static_cast<int>(filter1.GetFreq() / 1000));
        pos6Val.Append("k");
      }
      string pos7Text = "Q";
      FixedCapStr<8> pos7Val("");
      pos7Val.AppendFloat(filter1.GetQ());
      string pos8Text = "FilD";
      FixedCapStr<8> pos8Val("");
      pos8Val.AppendFloat(env2.GetDecay());

      hw.PrintToScreen(pos1Text.c_str(), screenOffset, row4);
      hw.PrintToScreen(pos1Val.c_str(), screenOffset, row5);
      hw.PrintToScreen(pos2Text.c_str(), screenOffset + 30 * 1, row4);
      hw.PrintToScreen(pos2Val.c_str(), screenOffset + 30 * 1, row5);
      hw.PrintToScreen(pos3Text.c_str(), screenOffset + 30 * 2, row4);
      hw.PrintToScreen(pos3Val.c_str(), screenOffset + 30 * 2, row5);
      hw.PrintToScreen(pos4Text.c_str(), screenOffset + 30 * 3, row4);
      hw.PrintToScreen(pos4Val.c_str(), screenOffset + 30 * 3, row5);
      hw.PrintToScreen(pos5Text.c_str(), screenOffset, row6);
      hw.PrintFixedCapStrToScreen(pos5Val, screenOffset, row7);
      hw.PrintToScreen(pos6Text.c_str(), screenOffset + 30 * 1, row6);
      hw.PrintFixedCapStrToScreen(pos6Val, screenOffset + 30 * 1, row7);
      hw.PrintToScreen(pos7Text.c_str(), screenOffset + 30 * 2, row6);
      hw.PrintFixedCapStrToScreen(pos7Val, screenOffset + 30 * 2, row7);
      hw.PrintToScreen(pos8Text.c_str(), screenOffset + 30 * 3, row6);
      hw.PrintFixedCapStrToScreen(pos8Val, screenOffset + 30 * 3, row7);

      // FixedCapStr<32> var("");
      // var.AppendFloat(pitchSeq.GetTranspose());
      // hw.Field().display.SetCursor(0, 40);
      // hw.Field().display.WriteString(var, Font_6x8, true);

      hw.UpdateDisplay();
    }

    if (mainCount % LEDS_UPDATE_DELAY == 0) {
      hw.ProcessLeds(stepTime);
    }

    System::Delay(MAIN_DELAY);
  }
}
