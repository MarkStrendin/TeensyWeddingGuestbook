// Teensy Wedding Recorder
// Code to run a Teensy based audio guest book for a wedding
// Based on https://www.youtube.com/watch?v=dI6ielrP1SE
// Adapted and customized by Mark Strendin

// Which pin should we check to see if the receiver
// is still "on hook" or not?
#define HOOK_BUTTON_PIN 2

// Pin for the indicator LED
#define LED_PIN 13

int lastButtonState;

void setup() {
  Serial.begin(9600);
  // Set up the necesary pin for the on-hook button
  pinMode(HOOK_BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(HOOK_BUTTON_PIN);
}

void loop() {
  // read the value of the button
  int buttonState = digitalRead(HOOK_BUTTON_PIN);

  if (lastButtonState != buttonState) {  // state changed
    delay(50);                           // debounce time

    if (buttonState == LOW) {    
      Serial.println("Off hook");
      digitalWrite(LED_PIN, HIGH);   // set the LED on 
    } else {
      Serial.println("On hook");
      digitalWrite(LED_PIN, LOW);    // set the LED off
    }

    lastButtonState = buttonState;
  }

}
