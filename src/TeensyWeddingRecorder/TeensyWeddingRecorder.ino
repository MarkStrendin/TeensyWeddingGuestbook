// Teensy Wedding Recorder
// Code to run a Teensy based audio guest book for a wedding
// Based on https://www.youtube.com/watch?v=dI6ielrP1SE
// Adapted and customized by Mark Strendin

// Includes
#include <Audio.h>

// Which pin should we check to see if the receiver
// is still "on hook" or not?
#define HOOK_BUTTON_PIN 2

// Pin for the indicator LED
#define LED_PIN 13

#define BEEP_VOLUME 0.1

enum Mode {Init, Ready, Prompting, Recording};
Mode system_mode = Mode::Init;

AudioControlSGTL5000 audio_shield_board;

// Set up audio path for playback
AudioOutputI2S audio_jack;

// Set up audio path for recording
AudioInputI2S  shield_mic;

// Set up audio path for beeps
AudioSynthWaveform    sndBeep;
AudioConnection       patchCord1(sndBeep, 0, audio_jack, 0);
AudioConnection       patchCord2(sndBeep, 0, audio_jack, 1);

// Set up audio file for the prompt
AudioPlaySdWav sndGreeting;

boolean is_phone_off_hook() {
  if (digitalRead(HOOK_BUTTON_PIN) == LOW) {
    return true;
  } else {
    return false;
  }
}

void beep_start_recording() {
  sndBeep.begin(BEEP_VOLUME, 825, WAVEFORM_SINE);
  delay(350);
  sndBeep.amplitude(0);
}

void beep_end_recording() {
  sndBeep.begin(BEEP_VOLUME, 825, WAVEFORM_SINE);
  delay(100);
  sndBeep.amplitude(0);
  delay(100);
  sndBeep.begin(BEEP_VOLUME, 825, WAVEFORM_SINE);
  delay(100);
  sndBeep.amplitude(0);
}

void setup() {
  Serial.begin(9600);  
  system_mode = Mode::Init;
  
  // Set up indictor LED
  pinMode(LED_PIN, OUTPUT);

  // set up audio system
  AudioMemory(60);
  audio_shield_board.enable();
  audio_shield_board.volume(1.0);
  audio_shield_board.inputSelect(AUDIO_INPUT_MIC);
  //audio_shield_board.adcHighPassFilterDisable();
  audio_shield_board.volume(0.75);

  // Set up on-hook button
  pinMode(HOOK_BUTTON_PIN, INPUT_PULLUP);


  // Set up SD card


  // Set system to Ready
  system_mode = Mode::Ready;
}

String get_hook_status() {
  if (is_phone_off_hook()) {
    return "OFF Hook";
  } else {
    return "ON Hook";
  }
}

void loop() {  
  // See if we need to change the state of the system

  Serial.println(get_system_mode() + " : " + get_hook_status());

  
  switch(system_mode) {
    case Mode::Init: 
      break;

    case Mode::Ready: 
      // If the user takes the receiver off hook start recording mode
      // That's it for now
      // If we add a playback button, check for that button here too
      if (is_phone_off_hook()) {
        Serial.println("Off hook");
        system_mode = Mode::Prompting;
        // Wait a second or so to give time to put the phone receiver to your head
        Serial.println("Waiting...");
        //delay(1500);
      }
      break;

    case Mode::Prompting:
      // Check if we're off hook
      // Play the prompt
      // Play the beep
      // Switch mode to recording

      if (is_phone_off_hook()) {      
        // Wait
        Serial.println("Waiting...");
        delay(1000);

        // Prompt
        Serial.println("Prompting...");
        
        // Play message here
        beep_start_recording();
        system_mode = Mode::Recording;
      } else {
        system_mode = Mode::Ready;
      }

      break;

    case Mode::Recording:    
      // Check if we're still off hook. If the receiver was hung up, do nothing.
      // Record anything input in the mic
      // When the handset goes back on hook, stop recording      
      // Switch mode back to ready
      
      if (is_phone_off_hook()) {       
        
        // Record until they hang up
        while(is_phone_off_hook() == true) {
          Serial.println("Recording...");
          delay(500);
        }

        Serial.println("Saving...");
        delay(50);
        beep_end_recording();
        system_mode = Mode::Ready;
      } else {
        beep_end_recording();
        system_mode = Mode::Ready;
      }

      break;
  }
  
  // Check the state of the system

  // User picks up receiver
  //  - Wait 1 second
  //  - Play prompt wav file
  //  - Check if the phone is still off the hook
  //  - Play a beep
  //  - Wait until the receiver is put back on hook
  //  - Write the file to the SD card?

  // The button does different things depending on the current state of the machine
  // So we need to track and check the state    
  
}

String get_system_mode() {
  if (system_mode == Mode::Init) { return "Init"; }
  if (system_mode == Mode::Ready) { return "Ready"; }
  if (system_mode == Mode::Prompting) { return "Prompting"; }
  if (system_mode == Mode::Recording) { return "Recording"; }
  return "Unknown";
}

