#include <Audio.h>

// GUItool: begin automatically generated code
AudioInputI2S            mic_in;           //xy=300,241
AudioOutputI2S           audio_jack_output;           //xy=475,240
AudioConnection          patchCord1(mic_in, 0, audio_jack_output, 0);
AudioConnection          patchCord2(mic_in, 1, audio_jack_output, 1);
AudioControlSGTL5000 audio_shield_board;

void setup() {
  Serial.begin(9600);
  AudioMemory(60);  

  audio_shield_board.enable();
  audio_shield_board.inputSelect(AUDIO_INPUT_MIC); //AUDIO_INPUT_MIC, AUDIO_INPUT_LINEIN
  audio_shield_board.micGain(15);
  //audio_shield_board.adcHighPassFilterDisable();
  audio_shield_board.volume(0.75);

  Serial.println("Booted");
}

void loop() {
}
