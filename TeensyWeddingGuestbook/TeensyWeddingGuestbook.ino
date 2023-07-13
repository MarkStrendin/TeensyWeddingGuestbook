// Teensy Wedding Recorder
// Code to run a Teensy based audio guest book for a wedding
// Based on https://www.youtube.com/watch?v=dI6ielrP1SE https://github.com/playfultechnology/audio-guestbook
// Adapted and customized by Mark Strendin
//
// Convert mp3s to wav files using this website because
// audacity files don't seem to work
// https://cloudconvert.com/mp3-to-wav
// Files must be pcm16 and  44100Hz or they will not play

// Includes
#include <Audio.h>
#include <SPI.h>
#include <SD.h>

// ***********************************************************
// * Defines
// ***********************************************************

// Which pin should we check to see if the receiver
// is still "on hook" or not?
#define HOOK_BUTTON_PIN 2

// Pin for the indicator LED
#define LED_PIN 13

// Set the master audio output volume
#define AUDIO_OUTPUT_VOLUME 0.75

// How loud should the beeps be
// You should probably leave this at 0.1 so you don't cause hearing damage
// to people with the speaker right up to their ear.
#define BEEP_VOLUME 0.1

#define BEEP_FREQUENCY 825

// Which pins to use for SDCard bits
// You probably don't want to mess with these
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// How many milliseconds should we wait between picking up the receiver and starting the prompt playback
#define PRE_PROMPT_DELAY 700

// How many samples to store in the queue
#define NBLOX 16 

// ***********************************************************
// * Globals
// ***********************************************************
enum Mode {Init, Ready, Prompting, Recording};
Mode system_mode = Mode::Init;

AudioControlSGTL5000 audio_shield_board;

// Set up audio path for playback
// This object represents the 3.5mm jack on the Teensy audio shield
AudioOutputI2S audio_jack;

// Set up audio path for beeps
// This object can make waveforms (tones at specific frequencies).
// We'll use it to make beeps.
AudioSynthWaveform sndWaveFormCreator;

// Set up audio file for the prompt
// This object can play arbitrary wav files off the SD card.
// It can be re-used, so we only need the one.
AudioPlaySdWav sndWavFilePlayer;

// Set up audio path for recording
// This object represents the microphone standoffs on the Teensy audio shield
AudioInputI2S mic_jack;

// Set up virtual audio stack
// We need a mixer to accept and combine multiple inputs (wav file and beeps)
AudioMixer4 audioMixer;

// Set up a record queue for audio input
AudioRecordQueue recordQueue;

// Hook the mixer to the audio jack L and R
// Since this is wired to a telephone handset with only one speaker, we only need
// one of these actually connected.
//AudioConnection patchCord_mixer_out_left(audioMixer, 0, audio_jack, 0); // Left channel
AudioConnection patchCord_mixer_out_right(audioMixer, 0, audio_jack, 1); // Right channel

// Hook the wave form creator to the mixer, on a new channel
AudioConnection patchCord_waveform_to_mixer(sndWaveFormCreator, 0, audioMixer, 0);

// Hook the wav file player to the mixer, on a new channel
AudioConnection patchCord_wavfile_to_mixer(sndWavFilePlayer, 0, audioMixer, 1);

// Hook the input queue into the mic
AudioConnection patchCord_mic_to_inputqueue(mic_jack, 0, recordQueue, 0);

// DEBUG: Listen to the mic
AudioConnection patchCord_mic_to_speakers_l(mic_jack, 0, audioMixer, 2);
AudioConnection patchCord_mic_to_speakers_r(mic_jack, 1, audioMixer, 3);

// ***********************************************************
// * Functions
// ***********************************************************

// Check to see if the phone is off hook by checking the status of the hook switch
boolean is_phone_off_hook() {
  if (digitalRead(HOOK_BUTTON_PIN) == LOW) {
    return true;
  } else {
    return false;
  }
}

// Play a beep to indicate that recording has started
void beep_start_recording() {
  sndWaveFormCreator.begin(BEEP_VOLUME, BEEP_FREQUENCY, WAVEFORM_SINE);
  delay(750);
  sndWaveFormCreator.amplitude(0);
}

// Play  a beep to indicate that recording has finished
void beep_end_recording() {
  for(int x=0;x<3;x++) {
    sndWaveFormCreator.begin(BEEP_VOLUME, BEEP_FREQUENCY, WAVEFORM_SINE);
    delay(75);
    sndWaveFormCreator.amplitude(0);  
    delay(75);
  }
}

// Play an arbitrary wav file
// File must be pcm16 and  44100Hz or it will not play
void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  sndWavFilePlayer.play(filename);

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (sndWavFilePlayer.isPlaying()) {
    if (!is_phone_off_hook()) {
      sndWavFilePlayer.stop();
      break;
    }
  }
}

// Return a string indicated what "mode" the system is in
// so we can display it on the serial connection
String get_system_mode() {
  if (system_mode == Mode::Init) { return "Init"; }
  if (system_mode == Mode::Ready) { return "Ready"; }
  if (system_mode == Mode::Prompting) { return "Prompting"; }
  if (system_mode == Mode::Recording) { return "Recording"; }
  return "Unknown";
}

void MTPDeviceChecks_Enable() {
    //MTP.storage()->set_DeltaDeviceCheckTimeMS(MTPcheckInterval);
    Serial.println("MTP Device Checks Enabled");
}

void MTPDeviceChecks_Disable() {
    //MTP.storage()->set_DeltaDeviceCheckTimeMS((uint32_t) -1);
    Serial.println("MTP Device Checks DISABLED");
}

// ***********************************************************
// * Setup
// ***********************************************************
void setup() {
  Serial.begin(9600);  
  system_mode = Mode::Init;
  
  // Set up indictor LED
  pinMode(LED_PIN, OUTPUT);

  // allocate some memory for the audio system
  // https://www.pjrc.com/teensy/td_libs_AudioConnection.html
  // This number is memory blocks, each which holds 128 audio samples, or approx 2.9ms of sound
  AudioMemory(60);
  
  // Set the volume on the mixer inputs to max
  audioMixer.gain(0, 1.0f);
  audioMixer.gain(1, 1.0f);
  audioMixer.gain(2, 1.0f);
  audioMixer.gain(3, 1.0f);

  // Set the mic volume gain
  audio_shield_board.micGain(5);

  audio_shield_board.enable();
  audio_shield_board.inputSelect(AUDIO_INPUT_LINEIN); //AUDIO_INPUT_MIC, AUDIO_INPUT_LINEIN
  audio_shield_board.lineInLevel(15);
  //audio_shield_board.adcHighPassFilterDisable();
  audio_shield_board.volume(AUDIO_OUTPUT_VOLUME);

  // Set up on-hook button
  pinMode(HOOK_BUTTON_PIN, INPUT_PULLUP);


  // Set up SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) 
  {    
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  } else {
    Serial.println("SD card correctly initialized");    
  } 

  //MTP.begin();
  //MTP.addFilesystem(SD, "TeensyGuestBook");
  //Serial.println("Added SD card via MTP");
  //MTPcheckInterval = MTP.storage()->get_DeltaDeviceCheckTimeMS();

  // AUDIO_BLOCK_SAMPLES should be set to 128 (outside this code)
  Serial.printf("Audio block set to %d samples\n",AUDIO_BLOCK_SAMPLES);

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

// update WAV header with final filesize/datasize
void writeOutHeader(File recordedFile, long bytesSaved) { 
  unsigned long ChunkSize = 0L;
  unsigned long Subchunk1Size = 16;
  unsigned int AudioFormat = 1;
  unsigned int numChannels = 1;
  unsigned long sampleRate = 44100;
  unsigned int bitsPerSample = 16;
  unsigned long byteRate = sampleRate*numChannels*(bitsPerSample/8);// samplerate x channels x (bitspersample / 8)
  unsigned int blockAlign = numChannels*bitsPerSample/8;
  unsigned long Subchunk2Size = 0L;
  byte byte1, byte2, byte3, byte4;

  Subchunk2Size = bytesSaved - 42; // because we didn't make space for the header to start with! Lose 21 samples...
  ChunkSize = Subchunk2Size + 34; // was 36;
  recordedFile.seek(0);
  recordedFile.write("RIFF");
  byte1 = ChunkSize & 0xff;
  byte2 = (ChunkSize >> 8) & 0xff;
  byte3 = (ChunkSize >> 16) & 0xff;
  byte4 = (ChunkSize >> 24) & 0xff;  
  recordedFile.write(byte1);  recordedFile.write(byte2);  recordedFile.write(byte3);  recordedFile.write(byte4);
  recordedFile.write("WAVE");
  recordedFile.write("fmt ");
  byte1 = Subchunk1Size & 0xff;
  byte2 = (Subchunk1Size >> 8) & 0xff;
  byte3 = (Subchunk1Size >> 16) & 0xff;
  byte4 = (Subchunk1Size >> 24) & 0xff;  
  recordedFile.write(byte1);  recordedFile.write(byte2);  recordedFile.write(byte3);  recordedFile.write(byte4);
  byte1 = AudioFormat & 0xff;
  byte2 = (AudioFormat >> 8) & 0xff;
  recordedFile.write(byte1);  recordedFile.write(byte2); 
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  recordedFile.write(byte1);  recordedFile.write(byte2); 
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;  
  recordedFile.write(byte1);  recordedFile.write(byte2);  recordedFile.write(byte3);  recordedFile.write(byte4);
  byte1 = byteRate & 0xff;
  byte2 = (byteRate >> 8) & 0xff;
  byte3 = (byteRate >> 16) & 0xff;
  byte4 = (byteRate >> 24) & 0xff;  
  recordedFile.write(byte1);  recordedFile.write(byte2);  recordedFile.write(byte3);  recordedFile.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  recordedFile.write(byte1);  recordedFile.write(byte2); 
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  recordedFile.write(byte1);  recordedFile.write(byte2); 
  recordedFile.write("data");
  byte1 = Subchunk2Size & 0xff;
  byte2 = (Subchunk2Size >> 8) & 0xff;
  byte3 = (Subchunk2Size >> 16) & 0xff;
  byte4 = (Subchunk2Size >> 24) & 0xff;  
  recordedFile.write(byte1);  recordedFile.write(byte2);  recordedFile.write(byte3);  recordedFile.write(byte4);
  recordedFile.close();
  Serial.println("header written"); 
  Serial.print("Subchunk2: "); 
  Serial.println(Subchunk2Size); 
}

// ***********************************************************
// * LOOP
// ***********************************************************

void loop() {  
  // See if we need to change the state of the system
  //Serial.println(get_system_mode() + " : " + get_hook_status());
  
  switch(system_mode) {
    case Mode::Init: 
      break;

    // ****************************************************** READY
    case Mode::Ready: 
      // If the user takes the receiver off hook start recording mode
      // That's it for now
      // If we add a playback button, check for that button here too
      if (is_phone_off_hook()) {
        Serial.println("Off hook");
        system_mode = Mode::Prompting;
        // Wait a second or so to give time to put the phone receiver to your head
        Serial.println("Waiting...");
        delay(PRE_PROMPT_DELAY);
      }
      break;

    // ****************************************************** PROMPTING
    case Mode::Prompting:
      // Check if we're off hook
      // Play the prompt
      // Play the beep
      // Switch mode to recording

      // We check via numerous if statements in this section because
      // the user could hang up the phone in the middle of the prompt
      // and in that case we want to stop and cancel the recording.

      if (is_phone_off_hook()) {      
        // Wait
        Serial.println("Waiting...");
        delay(1000);

        // Prompt
        Serial.println("Prompting...");        

        if (is_phone_off_hook()) {
          playFile("greeting.wav");          
        }
        
        delay(300);

        if (is_phone_off_hook()) {  
          system_mode = Mode::Recording;
        } else {
          system_mode = Mode::Ready;
        }        
      } else {
        system_mode = Mode::Ready;
      }

      break;
    
    // ****************************************************** RECORDING
    case Mode::Recording:    
      // Check if we're still off hook. If the receiver was hung up, do nothing.
      // Record anything input in the mic
      // When the handset goes back on hook, stop recording      
      // Switch mode back to ready
      
      if (is_phone_off_hook()) {       
        // Beep to indicate recording has started        
        beep_start_recording();

        // Pre-recording setup
        MTPDeviceChecks_Disable(); // Disable MTP device checks while recording

        // Find an available file name
        Serial.print("Finding an available filename... ");

        char filename[15];
        for (uint16_t filenum=0; filenum<=999; filenum++) {          
          snprintf(filename, 11, " %05d.wav", filenum);
          if (!SD.exists(filename)) { break; }
        }        
        Serial.println(filename);

        // Create a new file to record into
        File recorded_file = SD.open(filename, FILE_WRITE);

        // Keep track of how many bytes we record so we can use this in the file header
        unsigned long recordedBytesSaved = 0L;

        // **********************
        // Recording happens here
        // **********************

        // Record until they hang up
        Serial.println("Recording...");
        
        // Check to make sure the file opened successfully
        // Maybe make some beep codes if something broke

        recordQueue.begin();

        while(is_phone_off_hook() == true) {

          // If the recording queue has data in it, dump it to the file
          if (recordQueue.available() >= NBLOX) {
            byte buffer[NBLOX*AUDIO_BLOCK_SAMPLES*sizeof(int16_t)];            

            for (int i=0;i<NBLOX;i++)
            {
              memcpy(buffer+i*AUDIO_BLOCK_SAMPLES*sizeof(int16_t), recordQueue.readBuffer(), AUDIO_BLOCK_SAMPLES*sizeof(int16_t));
              
              recordQueue.freeBuffer();
            }

            // Write 512 bytes to the file
            recorded_file.write(buffer, sizeof buffer);
            recordedBytesSaved += sizeof buffer;
          }        
        }

        // **********************
        // Finished recording
        // **********************

        // Post-recording setup        
        Serial.println("Saving...");

        // Stop the recording queue
        recordQueue.end();

        // Flush all remaining data from queue
        while (recordQueue.available() > 0) {
          recorded_file.write((byte*)recordQueue.readBuffer(), AUDIO_BLOCK_SAMPLES*sizeof(int16_t));
          recordQueue.freeBuffer();
          recordedBytesSaved += AUDIO_BLOCK_SAMPLES*sizeof(int16_t);
        }

        // Write file header here
        writeOutHeader(recorded_file, recordedBytesSaved);

        // Close and save the file        
        recorded_file.close();

        MTPDeviceChecks_Enable();

        // Beep to indicate we're done
        delay(50);
        beep_end_recording();

        // DEBUG: Play the recorded file back        
        playFile(filename);  

      }

      // Return system to ready mode for the next recording
      system_mode = Mode::Ready;

      break;
    
  }  
}



