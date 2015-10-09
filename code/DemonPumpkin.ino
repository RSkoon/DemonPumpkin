/*
    Project: Demon Pumpkin
    Version: v1.0.2015.1003
    Author: Skoon Studios, LLC
    Distributed under the terms of the GNU General Public License (details below)

    Description: This program shows off simple LED and Servo motor control using
    an ATMega328 chip.  This was designed as part of a Halloween scare, but could
    feasibly be used in any sort of moving item.

    Hardware List:
        ATMega328 Chip
        2x LED
        Micro Servo

    Implementation:
        Wiring diagrams are available at http://www.skoonstudios.com.
        This code available at http://www.github.com/SkoonStudios/DemonPumpkin

    ******************************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <Servo.h>

#define DEBUG_VALUE 0   // Set to 1 to enable serial debug tracing

enum EYE_STATE
{
    ES_OFF = 0,
    ES_FADE_IN,
    ES_FADE_OUT,
    ES_SOLID,
    ES_BLINK_LEFT,
    ES_BLINK_RIGHT,
    ES_BLINK_BOTH,
    ES_CRAZY
};

enum MOUTH_STATE
{
    MOUTH_CLOSED = 0,
    MOUTH_OPENING,
    MOUTH_CLOSING
};

const int LED_FADE_STEP = 5;           // Step amount for fading LEDs
const int MOUTH_STEP = 5;               // Step amount for mouth

#if DEBUG_VALUE
const unsigned long WAKE_INTERVAL_MSEC = 7500;  // Wakeup Interval - 7.5 seconds
const unsigned long EVENT_INTERVAL_MSEC = 5000;  // Event Interval - 5 seconds
#else
const unsigned long WAKE_INTERVAL_MSEC = 120000;  // Wakeup Interval - 2 minutes
const unsigned long EVENT_INTERVAL_MSEC = 20000;  // Event Interval - 20 seconds
#endif

// Pin Setup
int _leftLEDPin = 5;        // LED pin for Left Eye
int _rightLEDPin = 6;       // LED pin for Right Eye
int _servoPin = 9;          // Pin for Servo control

// Vars
Servo _mouthServo;          // Servo for mouth
int _leftBrightness;        // Store brightness of Left Eye
int _rightBrightness;       // Store brightness of Right Eye
int _mouthPosition;         // Store current mouth position

EYE_STATE _eyeState;        // Current Eye State
MOUTH_STATE _mouthState;    // Current Mouth State
bool _isAwake;              // Set when the events start

unsigned long _NextEventTime;   // Time for Next Event 
unsigned long _TempEventTime;   // Temporary variable to handle some timed events

// ****************************************************************************
// setup()
//
// the setup function runs once when you press reset or power the board
// ****************************************************************************
void setup() {
#if DEBUG_VALUE
    Serial.begin(115200);
    Serial.println("STARTING...");
#endif

    _isAwake = false;
    _eyeState = ES_OFF;
    _mouthState = MOUTH_CLOSED;

    _leftBrightness = 0;
    _rightBrightness = 0;
    _mouthPosition = 90;

    _NextEventTime = millis() + 10000;  // Initial Delay is only 20sec

    // Initialize the RNG
    randomSeed(analogRead(0));

    // Setup and send data to pins
    pinMode(_leftLEDPin, OUTPUT);
    pinMode(_rightLEDPin, OUTPUT);

    analogWrite(_leftLEDPin, _leftBrightness);
    analogWrite(_rightLEDPin, _rightBrightness);

    setMouthPosition(_mouthPosition);
    _mouthServo.detach();
}

// ****************************************************************************
// loop()
//
// the loop function runs over and over again until power down or reset
// ****************************************************************************
void loop() {
    // Time to trigger an event
    if (millis() >= _NextEventTime)
    {
#if DEBUG_VALUE
        Serial.print("loop: Event Fired: ");
        Serial.println(millis());
#endif

        // Set to 0 until we fade in / out
        _NextEventTime = 0xFFFFFFFF;

        if (!_isAwake)
        {
            _eyeState = ES_FADE_IN;
            _isAwake = true;
        }
        else
        {
            _eyeState = ES_FADE_OUT;

            _mouthState = MOUTH_CLOSED;
            if (_mouthPosition != 90)
            {
                _mouthPosition = 90;
                setMouthPosition(_mouthPosition);    // Reset Mouth To Closed

                // Sometimes the servo can get real noisy, and it doesn't exactly hurt to disconnect
                // this until its time to move it again
                _mouthServo.detach();
            }
        }
    }
    // Either waiting or event in progress
    else
    {
        // Only update states if awake
        if (_isAwake)
        {
            updateEyeState();
            updateMouthState();
        }
    }

    delay(30);
}

// ****************************************************************************
// getRandomStates()
//
// Uses the random number generator to set a new eye and mouth fx
// ****************************************************************************
void getRandomStates()
{
    // Only want blink states
    _eyeState = (EYE_STATE)random(3, 8);

    // Set the temp interval for eye events
    _TempEventTime = millis() + 500;

    // 90% of the time will move mouth
    if (random(0, 10) <= 8)
    {
        _mouthState = MOUTH_OPENING;
    }

#if DEBUG_VALUE
    Serial.print("eye_state / mouth_state: ");
    Serial.print(_eyeState);
    Serial.print(" / ");
    Serial.println(_mouthState);
#endif
}

// ****************************************************************************
// updateEyeState
//
// Each loop update handle the update to the eyes
// ****************************************************************************
void updateEyeState()
{
    switch (_eyeState)
    {
    case ES_FADE_IN:
    case ES_FADE_OUT:
        eyeFadeInOut();
        break;

    case ES_BLINK_LEFT:
    case ES_BLINK_RIGHT:
    case ES_BLINK_BOTH:
        eyeBlink();
        break;

    case ES_CRAZY:
        eyeCrazy();
        break;
    }
}

// ****************************************************************************
// ****************************************************************************
void updateMouthState()
{
    if (_mouthState != MOUTH_CLOSED)
    {
        // 90 is Closed, 175 is Open so count up or down accordingly
        _mouthPosition += (_mouthState == MOUTH_OPENING ? MOUTH_STEP : -MOUTH_STEP);

        if (_mouthPosition >= 175)
        {
            _mouthPosition = 175;
            _mouthState = MOUTH_CLOSING;
        }
        else if (_mouthPosition <= 90)
        {
            _mouthPosition = 90;
            _mouthState = MOUTH_OPENING;
        }

        setMouthPosition(_mouthPosition);
    }
}

// ****************************************************************************
// setMouthPosition( int )
//
// Sets the servo to the new posistion.
//    - Min = 0  (mouth closed)
//    - Max = 90 (mouth open)
// ****************************************************************************
void setMouthPosition(int newPosition)
{
#if DEBUG_VALUE
    Serial.print("servo pos: ");
    Serial.println(newPosition);
#endif

    // Ensure the servo is rehooked up
    if (!_mouthServo.attached())
    {
        _mouthServo.attach(_servoPin);
        delay(25);
    }

    _mouthServo.write(newPosition);
    delay(15);
}

// ****************************************************************************
// eyeFadeInOut()
//
// Fade In / Out LEDs
// ****************************************************************************
void eyeFadeInOut()
{
    int fStep = _eyeState == ES_FADE_IN ? LED_FADE_STEP : -LED_FADE_STEP;

    // Needed for the 'crazy eye' scenario to fade out equally
    if (_leftBrightness != _rightBrightness)
    {
        _leftBrightness = _rightBrightness = max(_leftBrightness, _rightBrightness);
    }

    _leftBrightness += fStep;
    _rightBrightness += fStep;

    if (_leftBrightness < 0)
    {
        _leftBrightness = 0;
        _rightBrightness = 0;
    }

    analogWrite(_leftLEDPin, _leftBrightness);
    analogWrite(_rightLEDPin, _rightBrightness);

    // Update state once we hit either limit
    // Fade Out Complete
    if (_leftBrightness == 0)
    {
        _eyeState = ES_OFF;
        _isAwake = false;
        _NextEventTime = millis() + WAKE_INTERVAL_MSEC;

#if DEBUG_VALUE
        Serial.print("Now / Next: ");
        Serial.print(millis());
        Serial.print(" / ");
        Serial.println(_NextEventTime);
#endif
    }
    // Fade In Complete
    else if (_leftBrightness == 255)
    {
        getRandomStates();
        _NextEventTime = millis() + EVENT_INTERVAL_MSEC;
    }
}

// ****************************************************************************
// eyeBlink()
// 
// Updates LED for eye blinking
// ****************************************************************************
void eyeBlink()
{
    // Ready for a blink change, otherwise do nothing
    if (millis() >= _TempEventTime)
    {
        if ((_eyeState == ES_BLINK_BOTH) || (_eyeState == ES_BLINK_LEFT))
        {
            _leftBrightness = (_leftBrightness == 255 ? 0 : 255);
            analogWrite(_leftLEDPin, _leftBrightness);
        }

        if ((_eyeState == ES_BLINK_BOTH) || (_eyeState == ES_BLINK_RIGHT))
        {
            _rightBrightness = (_rightBrightness == 255 ? 0 : 255);
            analogWrite(_rightLEDPin, _rightBrightness);
        }

        _TempEventTime = millis() + random(250, 1250);
    }
}

// ****************************************************************************
// eyeCrazy()
// 
// Updates LED with random brightness
// ****************************************************************************
void eyeCrazy()
{
    if (millis() >= _TempEventTime)
    {
        _leftBrightness = random(10, 256);
        _rightBrightness = random(10, 256);

#if DEBUG_VALUE
        Serial.print(_leftBrightness);
        Serial.print(" / ");
        Serial.println(_rightBrightness);
#endif

        analogWrite(_leftLEDPin, _leftBrightness);
        analogWrite(_rightLEDPin, _rightBrightness);

        _TempEventTime = millis() + random(250, 750);
    }
}
