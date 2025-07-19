#define CONFIG_TINYUSB_HID_ENABLED 1
#define ARDUINO_USB_MODE 1
#define CFG_TUD_HID 1

#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <Preferences.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

// Define version
const char *aboutInfo[5] = {
    "ZYQ15-HE S2L01-AW",  // Model [Chip - 2 Characters][Light/No Light - 1 Character][Generation - 2 Characters] - [Screen Color - 2 Characters]
    "Firmware V1.0",      // Version
    "By Felix Zheng",     // Author
    "(@zfelix08)",         // Author username
    "Factory Reset Board" // Reset all settings button
};

// Define display object properties
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Define menu parameters
#define MENU_ITEM_HEIGHT 10
const char *menuItems[6][6] = {
    {"Calibration Tool", "Lighting Control", "Keymap Config", "Keypress Tuning", "About"},
    {"Release: ", "Press: ", "Travel: ", "Back"},
    {"R: ", "G: ", "B: ", "Bright: ", "Back"},
    {"Type: ", "Action: ", "Back"},
    {"Up: ", "Down: ", "RT-Up: ", "RT-Down: ", "Back"},
    {aboutInfo[0], aboutInfo[1], aboutInfo[2], aboutInfo[3], aboutInfo[4], "Back"}};
const int_fast8_t menuItemCounts[6] = {
    5,  // Number of items in the Main Menu
    15, // Calibration Tool
    17, // Lighting Control
    15, // Keymap Config
    15, // Keypress Tuning
    6   // About
};
const int_fast8_t menuItemCountsSub[6] = {
    0, // No subitems in the Main Menu
    4, // Number of subitems in the Calibration Tool
    5, // Lighting Control
    3, // Keymap Config
    5, // Keypress Tuning
    0  // No subitems in the About menu
};
int8_t menuIndex = 0;     // Current index in the main menu and submenus
int8_t menuSubIndex = -1; // Current text index in the submenus (-1 means no submenu)
int8_t menuPage = 0;      // 0 - Main Menu, 1 - Calibration Tool, 2 - Lighting Control, 3 - Keymap Config, 4 - Keypress Tuning, 5 - About

// Initalize LEDs
#define LED_PIN 40
#define LED_COUNT 17
#define MAX_BRIGHTNESS 64
CRGB leds[LED_COUNT];

// Define key bitmap
const unsigned char keyBitmap[] PROGMEM = {
    0xff, 0x00, 0x00, 0x03, 0xfc, 0x81, 0x00, 0x00, 0x02, 0x04, 0x81, 0x00, 0x00, 0x02, 0x04, 0x81,
    0x00, 0x00, 0x02, 0x04, 0x81, 0x00, 0x00, 0x02, 0x04, 0x81, 0x00, 0x00, 0x02, 0x04, 0x81, 0x00,
    0x00, 0x02, 0x04, 0xff, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x3f, 0xcf, 0xf3,
    0xfc, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04,
    0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0xff,
    0x3f, 0xcf, 0xf3, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x3f,
    0xcf, 0xf3, 0xfc, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48,
    0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12,
    0x04, 0xff, 0x3f, 0xcf, 0xf3, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0x3f, 0xcf, 0xf3, 0xfc, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81,
    0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20, 0x48, 0x12, 0x04, 0x81, 0x20,
    0x48, 0x12, 0x04, 0xff, 0x3f, 0xcf, 0xf3, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0f, 0xf1, 0xfe, 0x3f, 0xc0, 0x08, 0x11, 0x02, 0x20, 0x40, 0x08, 0x11, 0x02, 0x20,
    0x40, 0x08, 0x11, 0x02, 0x20, 0x40, 0x08, 0x11, 0x02, 0x20, 0x40, 0x08, 0x11, 0x02, 0x20, 0x40,
    0x08, 0x11, 0x02, 0x20, 0x40, 0x0f, 0xf1, 0xfe, 0x3f, 0xc0};
const int_fast8_t keyHighlights[17][2] = {
    {1, 1}, {31, 1}, {1, 13}, {11, 13}, {21, 13}, {31, 13}, {1, 23}, {11, 23}, {21, 23}, {31, 23}, {1, 33}, {11, 33}, {21, 33}, {31, 33}, {5, 43}, {16, 43}, {27, 43}};

// Define keyswitches and settings
const int8_t keyPins[15] = {12, 11, 9, 7, 5, 3, 1, 2, 4, 6, 8, 10, 13, 14, 17};                         // GPIO pins
bool keyStates[15] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; // Key states (pressed or released)
int16_t keyCalibration[15][4] = {
    // Actuation range in mV {release, press, travel, delta}
    {0, 5000, 36, 5000}, // Key 1
    {0, 5000, 36, 5000}, // Key 2
    {0, 5000, 36, 5000}, // Key 3
    {0, 5000, 36, 5000}, // Key 4
    {0, 5000, 36, 5000}, // Key 5
    {0, 5000, 36, 5000}, // Key 6
    {0, 5000, 36, 5000}, // Key 7
    {0, 5000, 36, 5000}, // Key 8
    {0, 5000, 36, 5000}, // Key 9
    {0, 5000, 36, 5000}, // Key 10
    {0, 5000, 36, 5000}, // Key 11
    {0, 5000, 36, 5000}, // Key 12
    {0, 5000, 36, 5000}, // Key 13
    {0, 5000, 36, 5000}, // Key 14
    {0, 5000, 36, 5000}  // Key 15
};
int16_t keyActuation[15][8] = {
    // Actuation distance in mm and actuation voltage in mV {release dist, press dist, RT-Up, RT-Down, release volt, press volt, RT-Up volt, RT-Down volt}}
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 1
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 2
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 3
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 4
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 5
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 6
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 7
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 8
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 9
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 10
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 11
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 12
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 13
    {10, 10, 0, 0, 2500, 2500, 0, 0}, // Key 14
    {10, 10, 0, 0, 2500, 2500, 0, 0}  // Key 15
};
int16_t keyRT[15][2] = {
    // Stores {minimum, maximum} switch voltage for one press-release RT cycle
    {0, 0}, // Key 1
    {0, 0}, // Key 2
    {0, 0}, // Key 3
    {0, 0}, // Key 4
    {0, 0}, // Key 5
    {0, 0}, // Key 6
    {0, 0}, // Key 7
    {0, 0}, // Key 8
    {0, 0}, // Key 9
    {0, 0}, // Key 10
    {0, 0}, // Key 11
    {0, 0}, // Key 12
    {0, 0}, // Key 13
    {0, 0}, // Key 14
    {0, 0}  // Key 15
};
int16_t keyRGB[17][4] = { // RGB values for each key {R, G, B, Brightness}
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100},
    {200, 200, 200, 100}
};
int16_t keyMapping[15][2] = { // {type index, action index}
    {0, 0},
    {0, 1},
    {0, 2},
    {0, 3},
    {0, 4},
    {0, 5},
    {0, 6},
    {0, 7},
    {0, 8},
    {0, 9},
    {0, 10},
    {0, 11},
    {0, 12},
    {0, 13},
    {0, 14}
};
int8_t disabledKeys = 0; // Number of disabled keys

// Define HID device
USBHIDKeyboard Keyboard;

// Define key mapping types and options
const char *keyMappingTypes[8] = {"Lttr", "Num", "NPad", "Mod", "Func", "Nav", "Arrow", "Smbl"};
const int8_t keyMappingTypesSize[8] = {26, 10, 10, 6, 12, 11, 4, 8}; // Number of options for each type
uint8_t keyMappingKeys[8][26] = {
    {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}, // Letters
    {HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0}, // Numbers
    {HID_KEY_KEYPAD_1, HID_KEY_KEYPAD_2, HID_KEY_KEYPAD_3, HID_KEY_KEYPAD_4, HID_KEY_KEYPAD_5, HID_KEY_KEYPAD_6, HID_KEY_KEYPAD_7, HID_KEY_KEYPAD_8, HID_KEY_KEYPAD_9, HID_KEY_KEYPAD_0}, // Numpad Numbers
    {HID_KEY_SHIFT_LEFT, HID_KEY_SHIFT_RIGHT, HID_KEY_CONTROL_LEFT, HID_KEY_CONTROL_RIGHT, HID_KEY_ALT_LEFT, HID_KEY_ALT_RIGHT}, // Modifiers
    {HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12}, // Function keys
    {HID_KEY_SPACE, HID_KEY_TAB, HID_KEY_ENTER, HID_KEY_ESCAPE, HID_KEY_BACKSPACE, HID_KEY_DELETE, HID_KEY_INSERT, HID_KEY_HOME, HID_KEY_END, HID_KEY_PAGE_UP, HID_KEY_PAGE_DOWN}, // Navigation keys
    {HID_KEY_ARROW_UP, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_RIGHT}, // Arrow keys
    {HID_KEY_MINUS, HID_KEY_EQUAL, HID_KEY_SLASH, HID_KEY_BACKSLASH, HID_KEY_COMMA, HID_KEY_PERIOD, HID_KEY_SEMICOLON, HID_KEY_APOSTROPHE} // Symbols
};
const char *keyMappingNames[8][26] = {
    {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"}, // Letters
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}, // Numbers
    {"PAD1", "PAD2", "PAD3", "PAD4", "PAD5", "PAD6", "PAD7", "PAD8", "PAD9", "PAD0"}, // Numpad Numbers
    {"LSHFT", "RSHFT", "LCtrl", "RCtrl", "LAlt", "RAlt"}, // Modifiers
    {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"}, // Function keys
    {"Space", "Tab", "Enter", "Esc", "Bksp", "Del", "Ins", "Home", "End", "PgUp", "PgDn"}, // Navigation keys
    {"Up", "Down", "Left", "Right"}, // Arrow keys
    {"Minus", "Equal", "Slash", "BSlsh", "Comma", "Dot", "SCol", "Apost"} // Symbols
};

// Define buttons
const int8_t buttonPins[2] = {38, 36}; // {left, right}
bool buttonStates[2] = {HIGH, HIGH};

// Reset settings variable
bool resetSettings = false;

// Define lowpass filter parameters
float alphaFast = 0.99;
float alphaSlow = 0.09;
float alpha = 0.0;
float filteredValues[15] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Define non-volatile storage (NVS) settings
Preferences settings;

int calculateVoltage(int keyIndex, bool isPress)
{
    // Release voltage + (Voltage delta * Press or Release threshold / Travel distance)
    return keyCalibration[keyIndex][0] + (keyCalibration[keyIndex][3] * keyActuation[keyIndex][isPress ? 1 : 0] / keyCalibration[keyIndex][2]);
}

int calculateVoltageRT(int keyIndex, bool isPress)
{
    // Voltage delta * RT-Up or RT-Down threshold / Travel distance
    return keyCalibration[keyIndex][3] * keyActuation[keyIndex][isPress ? 3 : 2] / keyCalibration[keyIndex][2];
}

float readKey(int pin)
{
    int rawValue = analogRead(keyPins[pin]) * 3600 / 4095; // Convert to mV
    float delta = abs(rawValue - filteredValues[keyPins[pin]]);
    alpha = delta > 8 ? alphaFast : alphaSlow;

    filteredValues[keyPins[pin]] = (alpha * rawValue) + ((1 - alpha) * filteredValues[keyPins[pin]]); // Low pass filter

    // Serial1.print(F("Key "));
    // Serial1.print(keyPins[pin]);
    // Serial1.print(F(": Raw Value = "));
    // Serial1.print(rawValue);
    // Serial1.print(F(", Filtered Value = "));
    // Serial1.println(filteredValues[keyPins[pin]]);

    return filteredValues[keyPins[pin]];
}

void keyPress()
{
    for (int i = 0; i < 15; i++)
    {
        int keyValue = readKey(i);

        if (keyActuation[i][2] != 0 && keyActuation[i][3] != 0) // When RT is enabled (RT-Up and RT-Down are not 0)
        {
            if (keyCalibration[i][3] > 0) // Switches that increase voltage as they are pressed
            {
                if (keyValue > keyActuation[i][5])
                {
                    if (keyStates[i] == HIGH)
                    {
                        if (keyValue < keyRT[i][0]) keyRT[i][0] = keyValue; // Push low reference to lowest point
                        if (keyValue > keyRT[i][0] + keyActuation[i][7]) // If the key has been depressed RT-Down voltage below it's low reference
                        {
                            keyStates[i] = LOW; // Key pressed
                            Serial1.print(i);
                            Serial1.println(F(": Key Pressed (RT: ON, +)"));
                            Keyboard.press(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key press

                            keyRT[i][1] = keyValue; // Modify high reference
                        }
                    }
                    else
                    {
                        if (keyValue > keyRT[i][1]) keyRT[i][1] = keyValue; // Push high reference to max. voltage
                        else if (keyValue < keyRT[i][1] - keyActuation[i][6]) // If the key has been released RT-Up voltage above it's high reference
                        {
                            keyStates[i] = HIGH;
                            Serial1.print(i);
                            Serial1.println(F(": Key Released (RT: ON, +)"));
                            Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release

                            keyRT[i][0] = keyValue; // Modify low reference
                        }
                        else if (keyValue < keyActuation[i][4]) // If the key exceeds its absolute (non-RT) release threshold
                        {
                            keyStates[i] = HIGH;
                            Serial1.print(i);
                            Serial1.println(F(": Key Released (RT: ON, Absolute Release, +)"));
                            Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release

                            keyRT[i][0] = keyActuation[i][5]; // Reset low reference
                        }
                    }
                }
            }
            else // Switches that decrease voltage as they are pressed
            {
                if (keyValue < keyActuation[i][5])
                {
                    if (keyStates[i] == HIGH)
                    {
                        if (keyValue > keyRT[i][0]) keyRT[i][0] = keyValue; // Push low reference to lowest point
                        if (keyValue < keyRT[i][0] + keyActuation[i][7]) // If the key has been depressed RT-Down voltage below it's low reference
                        {
                            keyStates[i] = LOW; // Key pressed
                            Serial1.print(i);
                            Serial1.println(F(": Key Pressed (RT: ON, -)"));
                            Keyboard.press(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key press

                            keyRT[i][1] = keyValue; // Modify high reference
                        }
                    }
                    else
                    {
                        if (keyValue < keyRT[i][1]) keyRT[i][1] = keyValue; // Push high reference to highest point
                        else if (keyValue > keyRT[i][1] - keyActuation[i][6]) // If the key has been released RT-Up voltage above it's high reference
                        {
                            keyStates[i] = HIGH;
                            Serial1.print(i);
                            Serial1.println(F(": Key Released (RT: ON, -)"));
                            Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release

                            keyRT[i][0] = keyValue; // Modify low reference
                        }
                        else if (keyValue > keyActuation[i][4]) // If the key exceeds its absolute (non-RT) release threshold
                        {
                            keyStates[i] = HIGH;
                            Serial1.print(i);
                            Serial1.println(F(": Key Released (RT: ON, Absolute Release, -)"));
                            Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release

                            keyRT[i][0] = keyActuation[i][5]; // Reset low reference
                        }
                    }
                }
            }
        }
        else
        {
            if (keyCalibration[i][3] > 0) // Switches that increase voltage as they are pressed
            {
                if (keyValue > keyActuation[i][5] && keyStates[i] == HIGH)
                {
                    keyStates[i] = LOW; // Key pressed
                    Serial1.print(i);
                    Serial1.println(F(": Key Pressed, +"));
                    Keyboard.press(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key press
                }
                else if (keyValue < keyActuation[i][4] && keyStates[i] == LOW)
                {
                    keyStates[i] = HIGH; // Key released
                    Serial1.print(i);
                    Serial1.println(F(": Key Released, +"));
                    Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release
                }
            }
            else // Switches that decrease voltage as they are pressed
            {
                if (keyValue < keyActuation[i][5] && keyStates[i] == HIGH)
                {
                    keyStates[i] = LOW; // Key pressed
                    Serial1.print(i);
                    Serial1.println(F(": Key Pressed, -"));
                    Keyboard.press(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key press
                }
                else if (keyValue > keyActuation[i][4] && keyStates[i] == LOW)
                {
                    keyStates[i] = HIGH; // Key released
                    Serial1.print(i);
                    Serial1.println(F(": Key Released, -"));
                    Keyboard.release(keyMappingKeys[keyMapping[i][0]][keyMapping[i][1]]); // Send key release
                }
            }
        }
    }
}

void saveNVS(const char *data)
{
    settings.begin("ZYQ15-HE", false); // Open preferences namespace
    if (data == "keyCalibration") settings.putBytes(data, keyCalibration, sizeof(keyCalibration));
    else if (data == "keyActuation") settings.putBytes(data, keyActuation, sizeof(keyActuation));
    else if (data == "keyRGB") settings.putBytes(data, keyRGB, sizeof(keyRGB));
    else if (data == "keyMapping") settings.putBytes(data, keyMapping, sizeof(keyMapping));
    else if (data == "resetSettings") settings.putBool(data, resetSettings);
    else
    {
        Serial1.println("Invalid data type for saving to NVS.");
        settings.end(); // Close preferences
        return;
    }

    Serial1.print("Data saved to NVS: ");
    Serial1.print(data);
    Serial1.print("    Size: ");
    Serial1.println(settings.getBytesLength(data));

    settings.end(); // Close preferences
}

void loadNVS()
{
    settings.begin("ZYQ15-HE", true);
    settings.getBool("resetSettings", resetSettings) == true ? resetSettings = true : resetSettings = false; // Load resetSettings flag
    settings.end();

    if (!resetSettings) // Load settings only if resetSettings is false
    {
        settings.begin("ZYQ15-HE", true); // Open preferences namespace for reading
        settings.getBytesLength("keyCalibration") == 120 ? settings.getBytes("keyCalibration", keyCalibration, sizeof(keyCalibration)) : Serial1.println("No key calibration data found in NVS.");
        settings.getBytesLength("keyActuation") == 240 ? settings.getBytes("keyActuation", keyActuation, sizeof(keyActuation)) : Serial1.println("No key actuation data found in NVS.");
        settings.getBytesLength("keyRGB") == 136 ? settings.getBytes("keyRGB", keyRGB, sizeof(keyRGB)) : Serial1.println("No key RGB data found in NVS.");
        settings.getBytesLength("keyMapping") == 60 ? settings.getBytes("keyMapping", keyMapping, sizeof(keyMapping)) : Serial1.println("No key mapping data found in NVS.");
        settings.end();
    }
    else // Save default settings to NVS if resetSettings is true
    {
        resetSettings = false;

        saveNVS("keyCalibration");
        saveNVS("keyActuation");
        saveNVS("keyRGB");
        saveNVS("keyMapping");
        saveNVS("resetSettings");

        Serial1.println("Saved default settings to NVS.");
    }
}

void drawText(int hpad, int16_t *data, int dataLen)
{
    
    int index = (menuPage == 0 || menuPage == 5) ? menuIndex : menuSubIndex;
    int itemCount = (menuPage == 0 || menuPage == 5) ? menuItemCounts[menuPage] : menuItemCountsSub[menuPage];

    for (int i = 0; i < itemCount; i++)
    {
        (i == index) ? display.setTextColor(SSD1306_BLACK, SSD1306_WHITE) : display.setTextColor(SSD1306_WHITE); // Highlight selected item

        display.setCursor(hpad, i * MENU_ITEM_HEIGHT);
        display.print(menuItems[menuPage][i]);

        if (menuPage == 3 && menuIndex < 15)
        {
            if (i == 0)
            {
                display.print(keyMappingTypes[keyMapping[menuIndex][0]]); // Print key mapping type
            }
            else if (i == 1)
            {
                display.print(keyMappingNames[keyMapping[menuIndex][0]][keyMapping[menuIndex][1]]); // Print key mapping action
            }
            continue;
        }

        if (data != NULL && i < dataLen) display.print(data[i]); // Print data if available
    }
}

void menuActions(const char *action)
{
    if (action == "back") // Go back to main menu
    {
        menuPage = 0;
        menuIndex = 0;
    }
    else if (action == "enter") // Enter submenu
    {
        menuPage = menuIndex + 1;
        menuIndex = 0;
    }
    else if (action == "bitmap") // Draw key bitmap
    {
        disabledKeys = (menuPage == 2) ? 0 : 2;
        display.drawBitmap(0, 0, keyBitmap, 38, 50, SSD1306_WHITE);

        // Highlight selected key
        if (menuIndex < 17 - disabledKeys)
            display.fillRect(keyHighlights[menuIndex + disabledKeys][0], keyHighlights[menuIndex + disabledKeys][1], 6, 6, SSD1306_WHITE);
        else if (menuIndex == 18 - disabledKeys)
            menuIndex = 0;

        // Highlight back button if selected
        display.setCursor(0, 56);
        (menuIndex == 17 - disabledKeys) ? display.setTextColor(SSD1306_BLACK, SSD1306_WHITE) : display.setTextColor(SSD1306_WHITE);
        display.print("Back");
    }
    else if (action == "select") // Select a key
    {
        menuSubIndex = 0;
    }
    else if (action == "done") // Go back to the key selection page of in a submenu
    {
        menuSubIndex = -1;
    }
}

void menuButtonPress(int buttonPin)
{
    // Left button
    if (buttonPin == buttonPins[0])
    {
        if (menuPage == 0 || menuPage == 5)
            (menuIndex < menuItemCounts[menuPage] - 1) ? menuIndex++ : menuIndex = 0; // Cycle through menu entries in main menu
        else if (menuSubIndex != -1)
            (menuSubIndex < menuItemCountsSub[menuPage] - 1) ? menuSubIndex++ : menuSubIndex = 0; // Cycle through submenus
        else
            (menuIndex < menuItemCounts[menuPage]) ? menuIndex++ : menuIndex = 0; // Cycle through menu entries in submenus
    }

    // Right button
    else if (buttonPin == buttonPins[1])
    {
        if (menuPage == 0) menuActions("enter"); // Enter submenus from main menu
        else if (menuPage == 5) // "About" submenu
        {
            if (menuIndex == 4)
            {
                resetSettings = true; // Activate resetSettings flag
                saveNVS("resetSettings"); // Save resetSettings flag to NVS

                display.clearDisplay(); // Clear display
                display.setCursor(0, 0);
                display.setTextColor(SSD1306_WHITE);
                display.setTextSize(1);
                display.print("Resetting...");
                display.display(); // Update display

                FastLED.clear(); // Clear LEDs
                FastLED.show(); // Update LEDs

                esp_restart(); // Restart the device
            }
            if (menuIndex == 5) menuActions("back"); // Go back to main menu from "about" submenu
        }
        else // Navigate through configuration submenus
        {
            if (menuSubIndex == -1) // Key selection
            {
                if (menuIndex == 17 - disabledKeys)
                    menuActions("back"); // Go back to main menu
                else
                    menuActions("select"); // Selects a key
            }
            else // Key adjustment
            {
                if (menuSubIndex == menuItemCountsSub[menuPage] - 1)
                    menuActions("done"); // Go back a level
                else if (menuSubIndex == menuItemCountsSub[menuPage] - 2) // Parameter 1 adjustment
                {
                    switch (menuPage)
                    {
                    case 1: // Calibration Tool ("Travel" value)
                        keyCalibration[menuIndex][2] < 40 ? keyCalibration[menuIndex][2] += 1 : keyCalibration[menuIndex][2] = 35; // Increase travel of switch (or reset to 35 mm)
                        keyActuation[menuIndex][5] = calculateVoltage(menuIndex, true); // Recalculate press voltage
                        keyActuation[menuIndex][4] = calculateVoltage(menuIndex, false); // Recalculate release voltage
                        saveNVS("keyCalibration");
                        break;
                    case 2: // Lighting Control ("Bright" value)
                        keyRGB[menuIndex][3] < 100 ? keyRGB[menuIndex][3] += 10 : keyRGB[menuIndex][3] = 0; // Increase brightness percentage (or reset to 0)
                        leds[menuIndex] = CRGB(
                            map(keyRGB[menuIndex][3], 0, 100, 0, keyRGB[menuIndex][0]), // Map brightness percentage to R value
                            map(keyRGB[menuIndex][3], 0, 100, 0, keyRGB[menuIndex][1]), // Map brightness percentage to G value
                            map(keyRGB[menuIndex][3], 0, 100, 0, keyRGB[menuIndex][2])  // Map brightness percentage to B value
                            );
                        saveNVS("keyRGB");
                        break;
                    case 3: // Keymap Config ("Action" value)
                        keyMapping[menuIndex][1] < keyMappingTypesSize[keyMapping[menuIndex][0]] - 1 ? keyMapping[menuIndex][1] += 1 : keyMapping[menuIndex][1] = 0; // Cycle through actions for the selected type (or reset to first action)
                        saveNVS("keyMapping");
                        break;
                    case 4: // Keypress Tuning (RT-Down value)
                        keyActuation[menuIndex][3] < keyCalibration[menuIndex][2] - keyActuation[menuIndex][1] - 1 ? keyActuation[menuIndex][3] += 1 : keyActuation[menuIndex][3] = 0; // Increase RT-Down threshold of switch or reset to 0 (RT off)
                        keyActuation[menuIndex][3] == 1 ? keyActuation[menuIndex][3] = 2 : keyActuation[menuIndex][3]; // Lowest RT-Down threshold is 2mm, otherwise innaccurate
                        keyActuation[menuIndex][7] = calculateVoltageRT(menuIndex, true); // Recalculate RT-Down voltage
                        saveNVS("keyActuation");
                        break;
                    }
                }
                else if (menuSubIndex == menuItemCountsSub[menuPage] - 3) // Parameter 2 adjustment (if applicable)
                {
                    switch (menuPage)
                    {
                    case 1: // Calibration Tool ("Press" value)
                        keyCalibration[menuIndex][1] = readKey(menuIndex); // Read key value at press
                        keyCalibration[menuIndex][3] = keyCalibration[menuIndex][1] - keyCalibration[menuIndex][0]; // Calculate voltage delta
                        keyActuation[menuIndex][5] = calculateVoltage(menuIndex, true); // Recalculate press voltage
                        keyActuation[menuIndex][4] = calculateVoltage(menuIndex, false); // Recalculate release voltage
                        saveNVS("keyCalibration");
                        break;
                    case 2: // Lighting Control ("B" value)
                        keyRGB[menuIndex][2] < 255 ? keyRGB[menuIndex][2] += 5 : keyRGB[menuIndex][2] = 0; // Increase blue value of the selected key (or reset to 0)
                        leds[menuIndex].setRGB(keyRGB[menuIndex][0], keyRGB[menuIndex][1], keyRGB[menuIndex][2]); // Set RGB values for the selected key
                        saveNVS("keyRGB");
                        break;
                    case 3: // Keymap Config ("Type" value)
                        keyMapping[menuIndex][0] < 7 ? keyMapping[menuIndex][0] += 1 : keyMapping[menuIndex][0] = 0; // Cycle through key mapping types (or reset to "Letters")
                        keyMapping[menuIndex][1] = 0; // Reset action index to 0 for the new type
                        saveNVS("keyMapping");
                        break;
                    case 4: // Keypress Tuning (RT-Up value)
                        keyActuation[menuIndex][2] < keyActuation[menuIndex][3] + keyActuation[menuIndex][1] - 1 ? keyActuation[menuIndex][2] += 1 : keyActuation[menuIndex][2] = 0; // Increase RT-Up threshold of switch or reset to 0 (RT off)
                        keyActuation[menuIndex][2] == 1 ? keyActuation[menuIndex][2] = 2 : keyActuation[menuIndex][2]; // Lowest RT-Up threshold is 2mm, otherwise innaccurate
                        keyActuation[menuIndex][6] = calculateVoltageRT(menuIndex, false); // Recalculate RT-Down voltage
                        saveNVS("keyActuation");
                        break;
                    }
                }
                else if (menuSubIndex == menuItemCountsSub[menuPage] - 4) // Parameter 3 adjustment (if applicable)
                {
                    switch (menuPage)
                    {
                    case 1: // Calibration Tool ("Release" value)
                        keyCalibration[menuIndex][0] = readKey(menuIndex); // Read key value at release
                        keyCalibration[menuIndex][3] = keyCalibration[menuIndex][1] - keyCalibration[menuIndex][0]; // Calculate voltage delta
                        keyActuation[menuIndex][5] = calculateVoltage(menuIndex, true); // Recalculate press voltage
                        keyActuation[menuIndex][4] = calculateVoltage(menuIndex, false); // Recalculate release voltage
                        saveNVS("keyCalibration");
                        break;
                    case 2: // Lighting Control ("G" value)
                        keyRGB[menuIndex][1] < 255 ? keyRGB[menuIndex][1] += 5 : keyRGB[menuIndex][1] = 0; // Increase green value of the selected key (or reset to 0)
                        leds[menuIndex].setRGB(keyRGB[menuIndex][0], keyRGB[menuIndex][1], keyRGB[menuIndex][2]); // Set RGB values for the selected key
                        saveNVS("keyRGB");
                        break;
                    case 4: // Keypress Tuning ("Down" value)
                        keyActuation[menuIndex][1] < keyCalibration[menuIndex][2] - 1 ? keyActuation[menuIndex][1] += 1 : keyActuation[menuIndex][1] = keyActuation[menuIndex][0]; // Increase press threshold of switch (or reset to the release threshold)
                        keyActuation[menuIndex][5] = calculateVoltage(menuIndex, true); // Recalculate press voltage
                        keyRT[menuIndex][0] = keyActuation[menuIndex][5]; // Set minimum RT voltage to press voltage
                        keyActuation[menuIndex][2] = 0;
                        keyActuation[menuIndex][3] = 0; // Turn off RT-Down and RT-Up to account for the new press threshold
                        saveNVS("keyActuation");
                        break;
                    }
                }
                else if (menuSubIndex == menuItemCountsSub[menuPage] - 5) // Parameter 4 adjustment (if applicable)
                {
                    switch (menuPage)
                    {
                    case 2: // Lighting Control ("R" value)
                        keyRGB[menuIndex][0] < 255 ? keyRGB[menuIndex][0] += 5 : keyRGB[menuIndex][0] = 0; // Increase red value of the selected key (or reset to 0)
                        leds[menuIndex].setRGB(keyRGB[menuIndex][0], keyRGB[menuIndex][1], keyRGB[menuIndex][2]); // Set RGB values for the selected key
                        saveNVS("keyRGB");
                        break;
                    case 4: // Keypress Tuning ("Up" value)
                        keyActuation[menuIndex][0] < keyActuation[menuIndex][1] ? keyActuation[menuIndex][0] += 1 : keyActuation[menuIndex][0] = 1; // Increase release threshold of switch (or reset to 1mm)
                        keyActuation[menuIndex][4] = calculateVoltage(menuIndex, false); // Recalculate release voltage
                        saveNVS("keyActuation");
                        break;
                    }
                }
            }
        }
    }
}

void drawMenu()
{
    display.setCursor(0, 0);

    if (menuPage == 0 || menuPage == 5) // Main menu and About
    {
        drawText(0, NULL, 0);
    }
    else // Submenus
    {
        menuActions("bitmap");

        if (menuPage == 1) // Calibration Tool
        {
            drawText(48, menuIndex == 15 ? NULL : keyCalibration[menuIndex], 3);
        }
        else if (menuPage == 2) // Lighting Control
        {
            drawText(48, menuIndex == 17 ? NULL : keyRGB[menuIndex], 4);
        }
        else if (menuPage == 3) // Keymap Config
        {
            drawText(48, menuIndex == 15 ? NULL : keyMapping[menuIndex], 2);
        }
        else if (menuPage == 4) // Keypress Tuning
        {
            drawText(48, menuIndex == 15 ? NULL : keyActuation[menuIndex], 4);
        }
    }

    display.display();
}

void UpdateDisplayTask(void *pvParameters)
{
    while (true)
    {
        drawMenu();

        // Check button states
        for (int i = 0; i < 2; i++)
        {
            if (digitalRead(buttonPins[i]) == LOW && buttonStates[i] == HIGH)
            {
                buttonStates[i] = LOW;          // Button pressed
                menuButtonPress(buttonPins[i]); // Handle button press
                display.clearDisplay();         // Clear display for new menu
            }
            else if (digitalRead(buttonPins[i]) == HIGH && buttonStates[i] == LOW)
            {
                buttonStates[i] = HIGH; // Button released
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Update display/check config buttons every 10 ms (100 hz)
    }
}

void UpdateKeyPressTask(void *pvParameters)
{
    while (true)
    {
        keyPress(); // Read key states

        vTaskDelay(pdMS_TO_TICKS(1)); // Update key states every 1 ms (1000 hz)
    }
}

void UpdateLEDsTask(void *pvParameters)
{
    while (true)
    {
        FastLED.show(); // Show updated LED states

        vTaskDelay(pdMS_TO_TICKS(125)); // Update LEDs every 125 ms (8 hz)
    }
}

void debugTask(void *pvParameters)
{
    while (true)
    {
        Serial1.print("Menu Page: ");
        Serial1.print(menuPage);
        Serial1.print("  |  Menu Index: ");
        Serial1.print(menuIndex);
        Serial1.print("  |  Menu SubIndex: ");
        Serial1.print(menuSubIndex);
        Serial1.print("  |  Button States: ");
        Serial1.print(buttonStates[0]);
        Serial1.print(", ");
        Serial1.println(buttonStates[1]);
        vTaskDelay(pdMS_TO_TICKS(250)); // Print debug info every quarter second
    }
}

void setup()
{
    Wire.begin(33, 35);

    analogReadResolution(12); // Set ADC resolution to 12 bits (4096 levels)
    analogSetAttenuation(ADC_11db); // Set ADC attenuation to 11 dB (0-3.6V range)

    Serial1.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for Serial to initialize
    Serial1.println("Started ZYQ15-HE");

    // Initialize the OLED object
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial1.println("SSD1306 allocation failed");
        for (;;)
            ;
    }

    // Set button modes
    for (int i = 0; i < 2; i++) pinMode(buttonPins[i], INPUT_PULLUP);

    // Set key pin modes
    for (int i = 0; i < 15; i++) pinMode(keyPins[i], INPUT);

    // Load NVS
    loadNVS();
    Serial1.println("NVS loaded");

    // Loading screen
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 24);
    display.print("Loading");
    display.display();
    display.startscrollright(0x03, 0x04);
    
    // Initialize LEDs
    FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, LED_COUNT);
    FastLED.clear();
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 350); // Set max power to 5V and 350mA
    FastLED.setBrightness(0); // Turn LEDs off initially to limit inrush current
    for (int i = 0; i < LED_COUNT; i++) leds[i] = CRGB(keyRGB[i][0], keyRGB[i][1], keyRGB[i][2]); // Set initial RGB values for each key
    FastLED.show();

    // Gradually increase brightness to avoid inrush current
    for (int i = 1; i < MAX_BRIGHTNESS; i++)
    {
        FastLED.setBrightness(i);   
        FastLED.show();
        delay(50);
    }

    // Initialize USB
    Keyboard.begin();
    USB.begin();

    // Stop loading screen
    display.stopscroll();
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    // Initialize tasks
    xTaskCreate(UpdateDisplayTask, "UpdateDisplay", 2048, NULL, 1, NULL);
    xTaskCreate(UpdateKeyPressTask, "UpdateKeyPress", 2048, NULL, 1, NULL);
    xTaskCreate(UpdateLEDsTask, "UpdateLEDs", 2048, NULL, 1, NULL);
    // xTaskCreate(debugTask, "Debug", 2048, NULL, 1, NULL);

    Serial1.println("Setup complete");
}

void loop()
{

}

#endif