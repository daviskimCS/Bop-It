#include <avr/io.h>
#include "timer.h"
#include <LiquidCrystal.h> // by Armin
#include <IRremote.hpp> // by Arfunio
#include <util/delay.h>

int wdt_disable();

volatile unsigned int difficultyPeriodMs = 200;

LiquidCrystal lcd(7, 8, 4, 10, 11, 12);

bool readyToStart = false;
bool gameOver = false;
bool resetGame = false;
int scoreData = 0;

void gameReset()
{
    readyToStart = false;
    gameOver = false;
    resetGame = false;
    scoreData = 0;
}

void ADC_init()
{
    ADMUX |= (1 << REFS0);
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADCSRA |= (1 << ADEN);
}

const int vrxPin = 0;
const int vryPin = 1;

unsigned short analogReadJ(unsigned char channel)
{
    ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
        ;
    return ADC;
}

enum JoystickStates
{
    neutral,
    right,
    left,
    forward,
    reverse
};
JoystickStates joystickState = neutral;

bool joystickDetected = false;

void TickFct_Joystick()
{
    unsigned short xValue = analogRead(vrxPin);
    unsigned short yValue = analogRead(vryPin);

    const unsigned short neutralMin = 450;
    const unsigned short neutralMax = 600;

    JoystickStates newJoystickState = neutral;

    if (xValue > 800)
    {
        newJoystickState = right;
    }
    else if (xValue < 400)
    {
        newJoystickState = left;
    }
    else if (yValue < 100)
    {
        newJoystickState = forward;
    }
    else if (yValue > 600)
    {
        newJoystickState = reverse;
    }
    else if (xValue >= neutralMin && xValue <= neutralMax && yValue >= neutralMin && yValue <= neutralMax)
    {
        newJoystickState = neutral;
    }

    if (newJoystickState != joystickState)
    {
        joystickState = newJoystickState;
        joystickDetected = true;

        switch (joystickState)
        {
        case right:
            Serial.println("right!");
            break;
        case left:
            Serial.println("left!");
            break;
        case forward:
            Serial.println("forward!");

            break;
        case reverse:
            Serial.println("reverse");

            break;
        case neutral:
            Serial.println("neutral");
            break;
        }
    }
}

int redPin = 6;
int greenPin = 3;
int bluePin = 5;

void setColor(int redValue, int greenValue, int blueValue)
{
    analogWrite(redPin, redValue);
    analogWrite(greenPin, greenValue);
    analogWrite(bluePin, blueValue);
}

enum IRStates
{
    waitIR,
    delayIR,
} IRState;

unsigned long lastIRTime = 0;

void TickFct_IRRemote()
{
    // Serial.println("     Remote SM Detection:: ");
    switch (IRState)
    {
    case waitIR:
        if (IrReceiver.decode())
        {
            unsigned long receivedValue = IrReceiver.decodedIRData.decodedRawData;
            // Serial.print("                Remote SM:: VALUE RECEIVED ");
            Serial.println(F("                Remote SM:: VALUE RECEIVED "));

            Serial.println(receivedValue);
            switch (receivedValue)
            {
            case 0xE916FF00:
                Serial.println(F("                                                          0 RECEIVED"));
                readyToStart = true;
                break;
            case 0xF30CFF00:
                Serial.println(F("                                                          1 RECEIVED"));
                resetGame = true;
                break;
            default:
                break;
            }
            IrReceiver.resume();
            IRState = delayIR;
            lastIRTime = customMillis();
        }
        else
        {
            IRState = waitIR;
        }
        break;
    case delayIR:
        if (customMillis() - lastIRTime >= 1000)
        {
            IRState = waitIR;
        }
        IRState = waitIR;
        break;
    default:
        IRState = waitIR;
        break;
    }
}

enum GameStates
{
    wait,
    start,
    waitForAction,
    checkForAction,
    restBreak,
    over,
    reset
} gameState;

enum PotentiometerStates
{
    potWait,
    potCheck
};

PotentiometerStates potState = potWait;
bool potDetected = false;

void TickFct_Potentiometer()
{                 
    static unsigned short readings[5];
    static int readIndex = 0;                   
    static unsigned short total = 0;            
    static unsigned short averagePotValue = 0;          

    unsigned short potValue = analogRead(A5);
    total = total - readings[readIndex];
    readings[readIndex] = potValue;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % 5;
    averagePotValue = total / 5;

    static unsigned short neutralPotValue = averagePotValue; 

    switch (potState)
    {
    case potWait:
        neutralPotValue = averagePotValue;
        potState = potCheck;
        break;

    case potCheck:
        if (abs(averagePotValue - neutralPotValue) > 10) 
        {
            potDetected = true;
            Serial.print(F(" YESSS pot movement detected! New value: "));
            Serial.println(averagePotValue);
        }
        else
        {
            Serial.println(F("NOO significant movement detected."));
        }

        potState = potWait;
        break;
    }
}


bool buttonPressDetected = false;
unsigned long currentMillis = 0;

volatile unsigned char stateHolder;

void TickFct_Button()
{
    static bool lastButtonState = LOW;
    bool buttonStateRaw = digitalRead(A4);

    if (buttonStateRaw != lastButtonState)
    {
        Serial.println("BUTTON UPDATED");
        buttonPressDetected = true;
    }
}

enum Actions
{
    none,
    button,
    jstick,
    potentiometer,
    waitPrompt
};

Actions actionCommandPromptAction = none;
Actions actionCommandLCD = none;

Actions actionInput = none;
bool actionSuccess = false;
unsigned long actionStartTime = 0;

void selectRandomAction()
{
    int randomSelection = random(0, 3);
    switch (randomSelection)
    {
    case 0:
        actionCommandPromptAction = button;
        break;
    case 1:
        actionCommandPromptAction = jstick;
        break;
    case 2:
        actionCommandPromptAction = potentiometer;
        break;
    }
}

void promptAction()
{
    selectRandomAction();

    actionCommandLCD = actionCommandPromptAction;

    actionStartTime = customMillis();

    switch (actionCommandPromptAction)
    {
    case button:
        buttonPressDetected = false;
        Serial.println(F("BUTTON: Prompt Action SM Wait for button: starting action start time"));
        actionCommandPromptAction = waitPrompt;

        break;
    case jstick:
        joystickDetected = false;
        Serial.println(F("JSTICK: Prompt Action SM Wait for button: starting action start time"));
        actionCommandPromptAction = waitPrompt;

        break;
    case potentiometer:
        potDetected = false;
        Serial.println(F("JSTICK: Prompt Action SM Wait for button: starting action start time"));
        actionCommandPromptAction = waitPrompt;
        break;
    case waitPrompt:
        break;
    default:
        actionCommandPromptAction = none;
        lcd.print("NONE");
        break;
    }
    return;
}

enum CheckActionStates
{
    checkActionInit,
    checkActionWait,
    checkActionSuccess,
    checkActionFailure
};

CheckActionStates checkActionState = checkActionInit;

void TickFct_CheckAction()
{
    switch (checkActionState)
    {
    case checkActionInit:
        Serial.println(F("INITIALIZATION of checkAction SM-- --"));
        break;
    case checkActionWait:
        Serial.print(F("                                              Check action SM WAITING WAITING WAITING -- --"));

        switch (actionCommandLCD)
        {
        case button:
            if (buttonPressDetected)
            {
                checkActionState = checkActionSuccess;
            }
            else
            {
                checkActionState = checkActionWait;
            }

            break;
        case jstick:
            if (joystickDetected)
            {
                checkActionState = checkActionSuccess;
            }
            else
            {
                checkActionState = checkActionWait;
            }

            break;
        case potentiometer:
            if (potDetected)
            {
                checkActionState = checkActionSuccess;
            }
            else
            {
                checkActionState = checkActionWait;
            }
            break;
        }

        break;

    case checkActionSuccess:
        actionSuccess = true;
        Serial.println(F("Check action SM SUCCESS -- --"));
        setColor(0, 255, 0);
        checkActionState = checkActionInit;
        break;

    case checkActionFailure:
        actionSuccess = false;
        Serial.println(F("Check action SM FAILURE -- -- "));
        setColor(255, 0, 0);

        checkActionState = checkActionInit;
        break;

    default:
        checkActionState = checkActionInit;
        break;
    }
}

void TickFct_GameReady()
{
    switch (gameState)
    {
    case wait:
        if (!resetGame)
        {
            gameState = readyToStart ? start : wait;
        }
        else
        {
            gameState = reset;
        }
        break;
    case start:
        if (!resetGame)
        {
            gameState = waitForAction;
        }
        else
        {
            gameState = reset;
        }
        break;
    case waitForAction:
        if (!resetGame)
        {
            if (!gameOver)
            {
                gameState = checkForAction;
            }
            else
            {
                gameState = over;
            }
        }
        else
        {
            gameState = reset;
        }
        break;
    case checkForAction:
        if (resetGame)
        {
            gameState = reset;
        }
        break;
    case restBreak:
        if (resetGame)
        {
            gameState = reset;
        }
        else
        {
            lcd.clear();
            lcd.print("Rest Break");
            Serial.println(F("REST BREAK REST BREAK REST BREAK"));
            _delay_ms(1000);
            gameState = waitForAction;
        }
        break;
    case over:
        lcd.print("Game over");
        _delay_ms(2500);
        lcd.clear();
        gameReset();
        gameState = wait;
        break;
    case reset:
        lcd.print("Game Resetting");
        _delay_ms(1000);
        setColor(255, 255, 255);
        lcd.clear();
        gameReset();
        gameState = wait;
        break;
    default:
        gameReset();
        break;
    }

    switch (gameState)
    {
    case wait:
        lcd.print("Ready to start?");
        lcd.setCursor(0, 1);
        lcd.print("Press 0 to play!");
        break;

    case start:
        lcd.print("Starting in 3");
        _delay_ms(500);

        lcd.clear();

        lcd.print(" - 2 - ");
        _delay_ms(500);
        lcd.clear();

        lcd.print(" - 1 - ");
        _delay_ms(500);
        lcd.clear();

        lcd.print("Go!");
        _delay_ms(100);
        lcd.clear();
        break;
    case waitForAction:
        promptAction();
        checkActionState = checkActionWait;
        break;
    case checkForAction:
        if (resetGame)
        {
            gameState = reset;
        }
        else
        {
            Serial.println(F("IN CHECK FOR ACTION : : PINGING HERE "));
            currentMillis = customMillis();
            Serial.println(currentMillis);
            Serial.println(actionStartTime);

            if (currentMillis - actionStartTime <= difficultyPeriodMs)
            {
                Serial.println(F(" ***  CHECKING WITHIN WINDOW   *** "));

                switch (actionCommandLCD)
                {
                case button:
                    lcd.print("Press the button!");
                    break;
                case jstick:
                    lcd.print("Move the joystick!");
                    break;
                case potentiometer:
                    lcd.print("Twist the knob!");
                    break;
                case waitPrompt:
                    break;
                default:
                    break;
                }

                if (actionSuccess)
                {
                    scoreData++;
                    lcd.clear();
                    lcd.print("Success");
                    lcd.setCursor(0, 1);
                    lcd.print(scoreData);

                    Serial.println(F("GAME READY ::  Successfully put in action"));
                    checkActionState = checkActionInit;
                    gameState = restBreak;
                    actionSuccess = false;
                }
                else
                {
                    gameState = checkForAction;
                }
            }
            else
            {
                lcd.clear();
                lcd.print("Failed :( ");
                lcd.setCursor(0, 1);
                lcd.print(scoreData);
                _delay_ms(1000);
                Serial.println(F("GAME READY ::  Failed to put in action"));
                checkActionState = checkActionFailure;
                gameState = over;
            }
        }
        break;
    default:
        break;
    }
}

void LCDSetup()
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
}

struct Task
{
    unsigned long period;
    unsigned long elapsedTime;
    unsigned char state;
    void (*TickFct)(void);
};

const int tasksNum = 6;

Task tasks[tasksNum] = {
    {100, 0, wait, &TickFct_GameReady},
    {20, 0, waitIR, &TickFct_IRRemote},
    {20, 0, potWait, &TickFct_Potentiometer},
    {20, 0, stateHolder, &TickFct_Button},
    {50, 0, checkActionInit, &TickFct_CheckAction},
    {20, 0, neutral, &TickFct_Joystick},
};

int main()
{
    Serial.begin(9600);

    DDRB = 0xFF;
    DDRD = 0xFF;

    // // SpeakerSetup
    // TCCR1A = 0x82;
    // TCCR1B = 0x1A;
    // // ICR1 = 1000;
    // // OCR1A = 250;

    LCDSetup();

    // TimerSetup
    TimerSet(10);
    TimerOn();

    // Initilization for JStick reading
    ADC_init();

    // IR RECEIVING INITIALIZATION
    pinMode(A3, INPUT);
    IrReceiver.begin(A3, DISABLE_LED_FEEDBACK);

    // POTENTIOMETER INITIALIZATION
    pinMode(19, INPUT);

    pinMode(A4, INPUT);

    lcd.print("Initializing... ");

    while (1)
    {
        for (int i = 0; i < tasksNum; i++)
        {
            // Serial.println("IN LOOP");
            if (tasks[i].elapsedTime >= tasks[i].period)
            {
                tasks[i].TickFct();
                tasks[i].elapsedTime = 0;
            }

            tasks[i].elapsedTime += 10;
        }

        // Print the current time to the Serial Monitor
        Serial.print(F("                            OUTSIDE LOOP TIME CHECK:      "));
        Serial.println(customMillis());

        while (!TimerFlag)
            ;
        TimerFlag = false;

        lcd.clear();
        setColor(0, 0, 0);
    }

    return 0;
}
