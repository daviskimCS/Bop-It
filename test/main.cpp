#include <avr/io.h>
#include "timer.h"
#include <LiquidCrystal.h>
#include <IRremote.hpp>
#include <util/delay.h>

bool buttonPressDetected = false;
unsigned long currentMillis = 0;
const unsigned long debounceDelay = 50;

void TickFct_Button()
{
    static unsigned long lastDebounceTime = 0;
    static bool lastButtonState = HIGH; // Start with HIGH if using INPUT_PULLUP
    bool buttonState = digitalRead(A4); // Read current button state

    if (buttonState != lastButtonState)
    {
        lastDebounceTime = millis(); // Reset debounce timer if state changes
        Serial.println("Button state changed, resetting debounce timer.");
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // If the state has been stable for longer than debounceDelay
        if (buttonState != lastButtonState)
        {
            lastButtonState = buttonState; // Update the button state

            if (buttonState == LOW)
            { // Button is pressed (LOW when using INPUT_PULLUP)
                buttonPressDetected = true;
                Serial.println(F("Button Pressed"));
            }
            else
            {
                buttonPressDetected = false;
                Serial.println(F("Button Released"));
            }
        }
    }
}

enum States
{
    WAIT_FOR_PRESS,
    ACTION_ON_PRESS,
    WAIT_FOR_RELEASE
};
States buttonState = WAIT_FOR_PRESS;

void loop()
{
    TickFct_Button(); // Call the debounce function

    switch (buttonState)
    {
    case WAIT_FOR_PRESS:
        if (buttonPressDetected)
        {
            buttonState = ACTION_ON_PRESS;
            Serial.println("Transition to ACTION_ON_PRESS");
            // Perform the action on button press here
        }
        break;

    case ACTION_ON_PRESS:
        Serial.println("ACTION_ON_PRESS state. Performing action.");
        // Code to handle action
        buttonState = WAIT_FOR_RELEASE; // Move to waiting for release
        break;

    case WAIT_FOR_RELEASE:
        Serial.println("WAIT_FOR_RELEASE state. Waiting for button release.");
        if (!buttonPressDetected)
        { // Wait until button is released
            buttonState = WAIT_FOR_PRESS;
            Serial.println("Transition to WAIT_FOR_PRESS");
        }
        break;
    }
}

void setup()
{
    pinMode(A4, INPUT); // Use internal pull-up resistor
    Serial.begin(9600);        // Initialize serial communication
    Serial.println("Setup complete, starting loop.");
}