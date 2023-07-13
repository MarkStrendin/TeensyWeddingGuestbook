# Teensy Wedding Guestbook
An audio wedding guestbook driven by a Teensy microcontroller board.
Built for a coworker that was about to have a wedding.

In my area, people rent these types of devices out to weddings for $100-$500 CAD per event.

Inspired by and adapted from https://www.youtube.com/watch?v=dI6ielrP1SE / https://github.com/playfultechnology/audio-guestbook

# Hardware Components
 - 1 GPO746 Rotary Phone (https://www.gporetro.com/rotary-dial-phones/gpo-746-rotary/)   
 - 1 SD card, formatted FAT32 (I used a Samsung EVO 32GB that I had lying around)
 - 1 SparkFun Teensy 4.0 board (https://www.digikey.ca/en/products/detail/sparkfun-electronics/DEV-15583/10384551)
 - 1 SparkFun Teensy Audio Adapter Board Rev D (https://www.digikey.ca/en/products/detail/sparkfun-electronics/DEV-15845/11205992)
 - 1 SparkFun 3.55 Jack Breakout Board (https://www.digikey.ca/en/products/detail/sparkfun-electronics/BOB-11570/6006052?s=N4IgTCBcDaIMoAcCGAnA1gMQK4DsAEAzAHQCsAtmSALoC%2BQA)
 - 1 Battery pack or cell phone charger (I used an Anker PowerCore 5K cell phone charger that I had lying around).
 - 1 male-to-male 3.5mm audio cable, as short as possible
 - Various headers to connect the two boards together (I used some from this kit https://www.pishop.ca/product/maker-essentials-various-headers/)
  - 2 14-pin male headers
  - 2 14-pin female headers
  - 1 2-pin male header (for mic port)
  - 1 4-pin male hader (for 3.5mm breakout board)
 - Various jumper wires to connect components from the phone to the board (I used some from this kit https://www.pishop.ca/product/40-x-jumper-cable-for-arduino-20cm/)

# Audio format for playback (Greeting/Prompts)
I created a text-to-speech audio prompt using [Amazon Polly](https://aws.amazon.com/polly/), which downloads as an MP3.
I then needed to convert this to a pcm16 44100Hz wav file for the Teensy to be able to play it back. I had issues getting Audacity to do this conversion (likely because I don't know what I'm doing), but converted wav files from this website worked: https://cloudconvert.com/mp3-to-wav.
