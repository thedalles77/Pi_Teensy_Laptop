//
// Keyboard and Touchpad controller for Sony Viao PCG-K25 laptop.
// This software is in the public domain.
// 
// Revision History
// Rev 1.00 - Nov 5, 2017 - Original Release
// Rev 1.01 - Nov 10, 2017 - Expanded i2c request event to include 32 characters
//
// The ps/2 code for the Touchpad is based on https://playground.arduino.cc/uploads/ComponentLib/mouse.txt
// The ps/2 definitions are described at http://computer-engineering.org/ps2mouse/
// The USB Mouse Functions are described at https://www.pjrc.com/teensy/td_mouse.html
// The USB Keyboard Functions are described at https://www.pjrc.com/teensy/td_keyboard.html
//
// Keyboard part number is KFRMBA151B
// The print screen and num lock keys were not functional on my keyboard so they do not show up in the matrix.
// The Menu key is not included in Teensyduino so it will be used as a print screen key. 
//
// The keyboard matrix: columns (inputs) across the top and rows (outputs) along the side 
/*
        0          1          2          3          4          5          6          7
0                           CTRL-R                                      CTRL-L  
1               ARROW-L    ARROW-D    ARROW-U    PAGE-D     PAGE-U       END       ARROW-R
2                ENTER                   ]                     =          " 
3     F12        MENU        /           ;         [           P          -        BCKSPACE
4    INSERT                                        \         HOME         L        DELETE
5     F10       COMMA      PERIOD        i        ZERO         9          F         F11
6     F8          M          B           8         U           O          J         F9
7     F7          N          G           Y         K           7          H         6
8     F5          V          S           T         R           5          C         F6
9     F3          X                      E         4           3          D         F4
10    F1          Z        SPACE         Q         2           1          W         F2
11                                                          SHIFT-L                SHIFT-R
12    ~                      A                    TAB      CAPS LCK                 ESC
13              ALT-R                  ALT-L
14                                                GUI
15    Fn

*/
//
#include <Wire.h>  // needed for I2C
//
// Define the keyboard columns that will be inputs to the Teensy (internal pullups in Teensy)
#define Col0 PIN_E0
#define Col1 PIN_E4
#define Col2 PIN_C1
#define Col3 PIN_A0
#define Col4 PIN_C2
#define Col5 PIN_A5
#define Col6 PIN_C4
#define Col7 PIN_A6
// Define the keyboard rows that will be driven low or floated by the Teensy (acts like open drain)
#define Row0 PIN_D3
#define Row1 PIN_D4
#define Row2 PIN_D5
#define Row3 PIN_E5
#define Row4 PIN_D7
#define Row5 PIN_E1
#define Row6 PIN_C0
#define Row7 PIN_A4
#define Row8 PIN_C3
#define Row9 PIN_A1
#define Row10 PIN_C5
#define Row11 PIN_A2
#define Row12 PIN_C6
#define Row13 PIN_A7
#define Row14 PIN_C7
#define Row15 PIN_A3
//
// Define the touchpad clock and data connections to the Teensy (bi-directional signals)
#define MDATA PIN_B3 // The MDATA & MCLK are driven low or floated (pull ups to 5V are in the touchpad chip). 
#define MCLK PIN_B2 // They are also read by the Teensy as inputs.
//
// Define the volume up and down, menu, and on/off controls from the Teensy to the LCD Controller card.
// These 4 controls are initiated by holding down the Fn key and then pushing a function key as follows:
// Fn & F1 = Menu, Fn & F3 = Vol_Dn, Fn & F4 = Vol_Up, Fn & F7 = On_Off
#define Vol_Up PIN_B7 // The Teensy takes the place of the push button switches to navigate the menus of the card. 
#define Vol_Dn PIN_B6 // The signals are driven low or floated by the Teensy (acts like open drain).
#define Menu PIN_B5   // The LCD Controller card has pullups on these signals to 3.3 volts.
#define On_Off PIN_B4 // DO NOT drive these to 5 volts. It may damage the LCD Controller chip.
//
#define BLINK_LED PIN_D6 // LED on Teensy board blinks at 1 second rate to show it's alive
//
#define RESET_PI PIN_B1 // Teensy outputs low pulse to reset Pi 
//
#define SHUTDOWN PIN_B0 // Teensy outputs high pulse to shut down the voltage regulators
//
#define CAPS_LED PIN_E7 // Send low to turn on the Caps Lock LED
//
#define DISK_LED PIN_E6 // spare LED used for code debug
//
// Declare variables that will be used by functions
boolean slots_full = LOW; // Goes high when slots 1 thru 6 contain keys
// slot 1 thru slot 6 hold the normal key values to be sent over USB. 
int slot1 = 0; //value of 0 means the slot is empty and can be used.  
int slot2 = 0; 
int slot3 = 0; 
int slot4 = 0; 
int slot5 = 0; 
int slot6 = 0;
//
// Declare variables that pi controls via i2c
boolean debug = LOW; // HIGH turns on the DISK_LED (used for code debug)
boolean reset_all = LOW; // HIGH resets the Pi and Teensy
boolean kill_power = LOW; // HIGH disables all 3 voltage regulators
//
// Function to clear the slot that contains the key name
void clear_slot(int key) {
  if (slot1 == key) {
    slot1 = 0;
  }
  else if (slot2 == key) {
    slot2 = 0;
  }
 else if (slot3 == key) {
    slot3 = 0;
  }
 else if (slot4 == key) {
    slot4 = 0;
  }
 else if (slot5 == key) {
    slot5 = 0;
  }
 else {
    slot6 = 0;
  }
  slots_full = LOW;
}
// Function to load the key name into the first available slot
void load_slot(int key) {
  if (!slot1)  {
    slot1 = key;
  }
  else if (!slot2) {
    slot2 = key;
  }
  else if (!slot3) {
    slot3 = key;
  }
  else if (!slot4) {
    slot4 = key;
  }
  else if (!slot5) {
    slot5 = key;
  }
  else {
    slot6 = key;
  }
  if (!slot1 || !slot2 || !slot3 || !slot4 || !slot5 || !slot6)  {
    slots_full = LOW;
  }
  else {
    slots_full = HIGH;
  }
}
// Function to send a pin to high impedance (float)
void go_z(int pin)
{
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}
// Function to send a pin to a logic low (0 volts)
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
// Function to send a pin to a logic 1 (5 volts)
void go_1(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);  
}
// Function to send the Touchpad a command
void touchpad_write(char data)
{
  char i;
  char parity = 1;
 // put pins in output mode 
  go_z(MDATA);
  go_z(MCLK);
  delayMicroseconds(300);
  go_0(MCLK);
  delayMicroseconds(300);
  go_0(MDATA);
  delayMicroseconds(10);
  // start bit 
  go_z(MCLK);
  // wait for touchpad to take control of clock)
  while (digitalRead(MCLK) == HIGH)
    ;
  // clock is low, and we are clear to send data 
  for (i=0; i < 8; i++) {
    if (data & 0x01) {
      go_z(MDATA);
    } 
    else {
      go_0(MDATA);
    }
    // wait for clock cycle 
    while (digitalRead(MCLK) == LOW)
      ;
    while (digitalRead(MCLK) == HIGH)
      ;
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }  
  // parity 
  if (parity) {
    go_z(MDATA);
  } 
  else {
    go_0(MDATA);
  }
  while (digitalRead(MCLK) == LOW)
    ;
  while (digitalRead(MCLK) == HIGH)
    ;
  // stop bit 
  go_z(MDATA);
  delayMicroseconds(50);
  while (digitalRead(MCLK) == HIGH)
    ;
  // wait for touchpad to switch modes 
  while ((digitalRead(MCLK) == LOW) || (digitalRead(MDATA) == LOW))
    ;
  // put a hold on the incoming data. 
  go_0(MCLK);
}
// Function to read a byte of data from the Touchpad
char touchpad_read(void)
{
  char data = 0x00;
  int i;
  char bity = 0x01;
  // start the clock 
  go_z(MCLK);
  go_z(MDATA);
  delayMicroseconds(50);
  while (digitalRead(MCLK) == HIGH)
    ;
  delayMicroseconds(5);  // wait for clock ring to settle 
  while (digitalRead(MCLK) == LOW) // eat start bit 
    ;
  for (i=0; i < 8; i++) {
    while (digitalRead(MCLK) == HIGH)
      ;
    if (digitalRead(MDATA) == HIGH) {
      data = data | bity;
    }
    while (digitalRead(MCLK) == LOW)
      ;
    bity = bity << 1;
  }
  // eat parity bit, which we ignore 
  while (digitalRead(MCLK) == HIGH)
    ;
  while (digitalRead(MCLK) == LOW)
    ;
  // eat stop bit 
  while (digitalRead(MCLK) == HIGH)
    ;
  while (digitalRead(MCLK) == LOW)
    ;
  // put a hold on the incoming data. 
  go_0(MCLK);

  return data;
}
// Function to initialize the Touchpad
void touchpad_init()
{
  go_z(MCLK);
  go_z(MDATA);
  //  Sending reset to touchpad 
  touchpad_write(0xff);
  touchpad_read();  // ack byte
  //  Read ack byte
  touchpad_read();  // blank 
  touchpad_read();  // blank 
  // Default resolution is 4 counts/mm which is too small
  //  Sending resolution command
  touchpad_write(0xe8);
  touchpad_read();  // ack byte
  touchpad_write(0x03); // value of 03 gives 8 counts/mm resolution
  touchpad_read();  // ack byte
  //  Sending remote mode code so the touchpad will send data only when polled
  touchpad_write(0xf0);  // remote mode 
  touchpad_read();  // ack byte
  delayMicroseconds(100);
}
// Function to send the 4 keyboard modifier keys over usb
void send_modifiers(int mod_shift, int mod_ctrl, int mod_alt, int mod_gui) {
  Keyboard.set_modifier(mod_shift | mod_ctrl | mod_alt | mod_gui);
  Keyboard.send_now();
}
// Function to send the keyboard normal keys in the 6 slots over usb
void send_normals(int slot1, int slot2, int slot3, int slot4, int slot5, int slot6) {
  Keyboard.set_key1(slot1);
  Keyboard.set_key2(slot2);
  Keyboard.set_key3(slot3);
  Keyboard.set_key4(slot4);
  Keyboard.set_key5(slot5);
  Keyboard.set_key6(slot6);
  Keyboard.send_now();
}
// Function to initialize the keyboard
void keyboard_init()
{
  pinMode(Col0, INPUT_PULLUP); // Configure the 8 keyboard columns with pullups
  pinMode(Col1, INPUT_PULLUP);
  pinMode(Col2, INPUT_PULLUP);
  pinMode(Col3, INPUT_PULLUP);
  pinMode(Col4, INPUT_PULLUP);
  pinMode(Col5, INPUT_PULLUP);
  pinMode(Col6, INPUT_PULLUP);
  pinMode(Col7, INPUT_PULLUP);
//
  go_z(Row0); // Send all 16 rows to high impedance (the off state)
  go_z(Row1);
  go_z(Row2);
  go_z(Row3);
  go_z(Row4);
  go_z(Row5);
  go_z(Row6);
  go_z(Row7);
  go_z(Row8);
  go_z(Row9);
  go_z(Row10);
  go_z(Row11);
  go_z(Row12);
  go_z(Row13);
  go_z(Row14);
  go_z(Row15);
//
  send_modifiers(0, 0, 0, 0); // tell the pi all mod keys are released
  send_normals(0, 0, 0, 0, 0, 0); // tell the pi all normal keys are released
}
//  Function to initialize the lcd control interface
void lcd_control_init()
{
  go_z(Vol_Dn); // send all lcd controls to hi z
  go_z(Vol_Up);
  go_z(Menu);
  go_z(On_Off);
}
//  Function to initialize the reset and shutdown signals and turn off the disk led
void reset_shutdown_init()
{
  go_z(RESET_PI); // put reset signal in inactive state
  go_0(SHUTDOWN); // put shutdown signal in inactive state
  go_1(DISK_LED); // turn off disk led 
}
// Function to pulse the Menu key on the lcd control card
void pulse_menu()
{
  go_0(Menu); //Pulse Menu key low
  delay(200);
  go_z(Menu);
  delay(800);
}
// Function to pulse the Vol Up key on the lcd control card
void pulse_vol_up()
{
  go_0(Vol_Up); //Pulse Vol_Up key low
  delay(200);
  go_z(Vol_Up);
  delay(800);
}
// Function to pulse the Vol_Dn key on the lcd control card
void pulse_vol_dn()
{
  go_0(Vol_Dn); //Pulse Vol_Dn key low
  delay(200);
  go_z(Vol_Dn);
  delay(800);
}
//  Function to give selectable seconds delay  
void sec_delay(int sec)
{
  int i;
  for (i=0; i < sec; i++) {
    delay(1000); //delay 1 second
    }
}
// Function to receive commands over i2c
// Commands are: shutdown = 0x5a, reset = 0xb7, debug led on = 0x10, debug led off = 0x11
void receiveEvent(int numBytes) {
  byte read_value;
  int i;
  for (i=0; i < numBytes; i++) {
    read_value = Wire.read();
    if (read_value == 0x5a) {  
      kill_power = HIGH; // Send variable "true" for shutdown on next keyboard polling cycle
    }
    if (read_value == 0xb7) {
      reset_all = HIGH; // Send variable "true" for reset on next keyboard polling cycle
    }
    if (read_value == 0x10) {
      debug = HIGH; // Send variable "true" for led turn on at the next keyboard polling cycle
    }
    if (read_value == 0x11) {
      debug = LOW; // Send variable "false" for led turn off at the next keyboard polling cycle
    }
  }
}
// Function to send Teensy code version number, date and author to Pi
void requestEvent() {
  Wire.write("Version #01.01  Nov 10, 2017 MFA");
}
// Setup the keyboard and touchpad. Float the lcd controls & pi reset. Drive the shutdown inactive.
void setup() {
  reset_shutdown_init(); // initialize reset and shutdown signals
  lcd_control_init(); // initialize lcd control signals
  sec_delay(30); //wait to let pi boot up before starting so pi registers the usb keyboard
  keyboard_init(); // initialize keyboard 
  touchpad_init(); // initialize touchpad
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event to receive command from Pi
  Wire.onRequest(requestEvent); // register event to send info back to Pi
}
// Declare and Initialize Keyboard Variables
boolean old_key_1 = HIGH; // old_key_ ... holds the state of every normal keyboard switch
boolean old_key_2 = HIGH; // as it was on the previous scan. 
boolean old_key_3 = HIGH; // Initialize all of the old_key_ ... variables to High (the off state).
boolean old_key_4 = HIGH;
boolean old_key_5 = HIGH;
boolean old_key_6 = HIGH;
boolean old_key_7 = HIGH;
boolean old_key_8 = HIGH;
boolean old_key_9 = HIGH;
boolean old_key_0 = HIGH;
boolean old_key_A = HIGH;
boolean old_key_B = HIGH;
boolean old_key_C = HIGH;
boolean old_key_D = HIGH;
boolean old_key_E = HIGH;
boolean old_key_F = HIGH;
boolean old_key_G = HIGH;
boolean old_key_H = HIGH;
boolean old_key_I = HIGH;
boolean old_key_J = HIGH;
boolean old_key_K = HIGH;
boolean old_key_L = HIGH;
boolean old_key_M = HIGH;
boolean old_key_N = HIGH;
boolean old_key_O = HIGH;
boolean old_key_P = HIGH;
boolean old_key_Q = HIGH;
boolean old_key_R = HIGH;
boolean old_key_S = HIGH;
boolean old_key_T = HIGH;
boolean old_key_U = HIGH;
boolean old_key_V = HIGH;
boolean old_key_W = HIGH;
boolean old_key_X = HIGH;
boolean old_key_Y = HIGH;
boolean old_key_Z = HIGH;
boolean old_key_LEFT = HIGH;
boolean old_key_RIGHT = HIGH;
boolean old_key_UP = HIGH;
boolean old_key_DOWN = HIGH;
boolean old_key_TILDE = HIGH;
boolean old_key_MINUS = HIGH;
boolean old_key_EQUAL = HIGH;
boolean old_key_BACKSPACE = HIGH;
boolean old_key_BACKSLASH = HIGH;
boolean old_key_RIGHT_BRACE = HIGH;
boolean old_key_LEFT_BRACE = HIGH;
boolean old_key_ENTER = HIGH;
boolean old_key_QUOTE = HIGH;
boolean old_key_SEMICOLON = HIGH;
boolean old_key_SLASH = HIGH;
boolean old_key_PERIOD = HIGH;
boolean old_key_COMMA = HIGH;
boolean old_key_SPACE = HIGH;
boolean old_key_CAPS_LOCK = HIGH;
boolean old_key_TAB = HIGH;
boolean old_key_ESC = HIGH;
boolean old_key_PAGE_UP = HIGH;
boolean old_key_PAGE_DOWN = HIGH;
boolean old_key_END = HIGH;
boolean old_key_HOME = HIGH;
boolean old_key_INSERT = HIGH;
boolean old_key_DELETE = HIGH;
boolean old_key_PRINTSCREEN = HIGH;
boolean old_key_F1 = HIGH;
boolean old_key_F2 = HIGH;
boolean old_key_F3 = HIGH;
boolean old_key_F4 = HIGH;
boolean old_key_F5 = HIGH;
boolean old_key_F6 = HIGH;
boolean old_key_F7 = HIGH;
boolean old_key_F8 = HIGH;
boolean old_key_F9 = HIGH;
boolean old_key_F10 = HIGH;
boolean old_key_F11 = HIGH;
boolean old_key_F12 = HIGH;
boolean old_key_MENU = HIGH; // the Menu key is not used in Teensyduino so it becomes the printscreen key
//
boolean old_key_SHIFT = HIGH; // old_key_ ... holds the state of every modifier keyboard switch
boolean old_key_CTRL = HIGH; // Initialize all of the old_key_ ... variables to High (the off state).
boolean old_key_ALT = HIGH;
boolean old_key_GUI = HIGH;
//
boolean Fn_pressed = HIGH; // Active low, Saves the state of the Fn key 
//
boolean touchpad_enabled = HIGH; // Active high, controls whether the touchpad is used or not
boolean button_change = LOW; // Active high, shows when a touchpad left or right button has changed since last polling cycle
//
int mod_shift = 0; // These 4 variables are sent over USB as modifier keys.
int mod_ctrl = 0; // Either set to 0 or MODIFIER_ ...   SHIFT, CTRL, ALT, GUI
int mod_alt = 0;
int mod_gui = 0;
// Declare and Initialize Touchpad variables
char mstat; // touchpad status reg = Y overflow, X overflow, Y sign bit, X sign bit, Always 1, Middle Btn, Right Btn, Left Btn
char mx; // touchpad x movement = 8 data bits. The sign bit is in the status register to 
         // make a 9 bit 2's complement value. Left to right on the touchpad gives a positive value. 
char my; // touchpad y movement = 8 bits plus sign. Touchpad movement bottom to top gives a positive value.
boolean over_flow; // set if x or y movement values are bad due to overflow
boolean left_button = 0; // on/off variable for left button = bit 0 of mstat
boolean right_button = 0; // on/off variable for right button = bit 1 of mstat
boolean old_left_button = 0; // on/off variable for left button status from the previous polling cycle
boolean old_right_button = 0; // on/off variable for right button status from the previous polling cycle
//
int blink_count = 0; // loop counter
boolean blinky = LOW; // Blink LED state
//
extern volatile uint8_t keyboard_leds; // 8 bits sent from Pi to Teensy that give keyboard LED status. Caps lock is bit D1.
//
// Main Loop scans the keyboard switches and then poles the touchpad 
//
void loop() {  
// 
// -------Scan keyboard matrix Rows 0 thru 15 & Columns 0 thru 7-------
//
// ------------------------------------------------------Row 15-----------------------------
  go_0(Row15); // Activate Row (send it low), then read the column
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************Fn Key***************************
  //  Fn key is checked first so the Fn_pressed variable can be used later when the function keys are scanned
  if (!digitalRead(Col0)) { // Check if Fn key is pressed
    Fn_pressed = LOW; // Fn pressed
  }
  else {
    Fn_pressed = HIGH; // Fn not pressed  
  }
  go_z(Row15); // send row back to off state
//
// ------------------------------------------------------Row 0-----------------------------
  go_0(Row0); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************CTRL Key L and R***************************
  //  Check if left or right control key is pressed and wasn't pressed last time
  if ((!digitalRead(Col2) || (!digitalRead(Col6))) && (old_key_CTRL)) {
    mod_ctrl = MODIFIERKEY_CTRL;
    old_key_CTRL = LOW;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  } 
  //  Check if left and right control keys are released and was pressed last time
  else if (digitalRead(Col2) && digitalRead(Col6) && (!old_key_CTRL)) {
    mod_ctrl = 0;
    old_key_CTRL = HIGH;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  } 
  go_z(Row0); // send row back to off state
// ------------------------------------------------------Row 11-----------------------------
  go_0(Row11); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************SHIFT Key L and R***************************
  //  Check if left or right shift key is pressed and wasn't pressed last time
  if ((!digitalRead(Col5) || (!digitalRead(Col7))) && (old_key_SHIFT)) {
    mod_shift = MODIFIERKEY_SHIFT;
    old_key_SHIFT = LOW;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }
  //  Check if left and right shift keys are released and was pressed last time
  else if (digitalRead(Col5) && digitalRead(Col7) && (!old_key_SHIFT)) {
    mod_shift = 0;
    old_key_SHIFT = HIGH;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }
  //
  go_z(Row11); // send row back to off state
// ------------------------------------------------------Row 13-----------------------------
  go_0(Row13); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************ALT Key L and R***************************
  //  Check if left or right alt key is pressed and wasn't pressed last time
  if ((!digitalRead(Col1) || (!digitalRead(Col3))) && (old_key_ALT)) {
    mod_alt = MODIFIERKEY_ALT;
    old_key_ALT = LOW;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }
  //  Check if left and right alt keys are released and was pressed last time
  else if (digitalRead(Col1) && digitalRead(Col3) && (!old_key_ALT)) {
    mod_alt = 0;
    old_key_ALT = HIGH;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }
  //
  go_z(Row13); // send row back to off state
// ------------------------------------------------------Row 14-----------------------------
  go_0(Row14); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************GUI Key***************************
  //  Check if gui key is pressed and wasn't pressed last time
  if (!digitalRead(Col4) && (old_key_GUI)) {
    mod_gui = MODIFIERKEY_GUI;
    old_key_GUI = LOW;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }
  //  Check if gui key is released and was pressed last time
  else if (digitalRead(Col4) && (!old_key_GUI)) {
    mod_gui = 0;
    old_key_GUI = HIGH;
    send_modifiers(mod_shift, mod_ctrl, mod_alt, mod_gui); // use function to send 4 modifiers over usb
  }   
  go_z(Row14); // send row back to off state
// --------------------------------------------------Row 1---------------------------------
  go_0(Row1); // Activate Row (send it low), then read the columns
//
  delayMicroseconds(10); // give the row time to go low and settle out
// *****************Left Arrow Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_LEFT && (!slots_full)) {
    load_slot(KEY_LEFT); //update first available slot with key name
    old_key_LEFT = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_LEFT) {
    clear_slot(KEY_LEFT); // clear slot that contains key name
    old_key_LEFT = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Down Arrow Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_DOWN && (!slots_full)) {
    load_slot(KEY_DOWN); //update first available slot with key name
    old_key_DOWN = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_DOWN) {
    clear_slot(KEY_DOWN); // clear slot that contains key name
    old_key_DOWN = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Up Arrow Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_UP && (!slots_full)) {
    load_slot(KEY_UP); //update first available slot with key name
    old_key_UP = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_UP) {
    clear_slot(KEY_UP); // clear slot that contains key name
    old_key_UP = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Page Down Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_PAGE_DOWN && (!slots_full)) {
    load_slot(KEY_PAGE_DOWN); //update first available slot with key name
    old_key_PAGE_DOWN = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_PAGE_DOWN) {
    clear_slot(KEY_PAGE_DOWN); // clear slot that contains key name
    old_key_PAGE_DOWN = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Page Up Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_PAGE_UP && (!slots_full)) {
    load_slot(KEY_PAGE_UP); //update first available slot with key name
    old_key_PAGE_UP = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_PAGE_UP) {
    clear_slot(KEY_PAGE_UP); // clear slot that contains key name
    old_key_PAGE_UP = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************End Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_END && (!slots_full)) {
    load_slot(KEY_END); //update first available slot with key name
    old_key_END = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_END) {
    clear_slot(KEY_END); // clear slot that contains key name
    old_key_END = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Right Arrow Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_RIGHT && (!slots_full)) {
    load_slot(KEY_RIGHT); //update first available slot with key name
    old_key_RIGHT = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_RIGHT) {
    clear_slot(KEY_RIGHT); // clear slot that contains key name
    old_key_RIGHT = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row1); // send row back to off state
// ------------------------------------------------------Row 2-----------------------------
  go_0(Row2); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************Enter Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_ENTER && (!slots_full)) {
    load_slot(KEY_ENTER); //update first available slot with key name
    old_key_ENTER = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_ENTER) {
    clear_slot(KEY_ENTER); // clear slot that contains key name
    old_key_ENTER = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Right Brace Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_RIGHT_BRACE && (!slots_full)) {
    load_slot(KEY_RIGHT_BRACE); //update first available slot with key name
    old_key_RIGHT_BRACE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_RIGHT_BRACE) {
    clear_slot(KEY_RIGHT_BRACE); // clear slot that contains key name
    old_key_RIGHT_BRACE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Equal Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_EQUAL && (!slots_full)) {
    load_slot(KEY_EQUAL); //update first available slot with key name
    old_key_EQUAL = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_EQUAL) {
    clear_slot(KEY_EQUAL); // clear slot that contains key name
    old_key_EQUAL = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Quote Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_QUOTE && (!slots_full)) {
    load_slot(KEY_QUOTE); //update first available slot with key name
    old_key_QUOTE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_QUOTE) {
    clear_slot(KEY_QUOTE); // clear slot that contains key name
    old_key_QUOTE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row2); // send row back to off state
// ------------------------------------------------------Row 3-----------------------------
  go_0(Row3); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F12 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pushed
  if (!digitalRead(Col0) && old_key_F12 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F12); //update first available slot with key name
    old_key_F12 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F12) {
    clear_slot(KEY_F12); // clear slot that contains key name
    old_key_F12 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F12 keys are pushed
  if (!digitalRead(Col0) && (!Fn_pressed)) {  
    touchpad_enabled = !touchpad_enabled; // toggle touchpad on/off
  }     
    while (digitalRead(Col0) == LOW) // wait until F12 key is released
    ; 
// *****************Print Screen Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_PRINTSCREEN && (!slots_full)) {
    load_slot(KEY_PRINTSCREEN); //update first available slot with key name
    old_key_PRINTSCREEN = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_PRINTSCREEN) {
    clear_slot(KEY_PRINTSCREEN); // clear slot that contains key name
    old_key_PRINTSCREEN = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Slash Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_SLASH && (!slots_full)) {
    load_slot(KEY_SLASH); //update first available slot with key name
    old_key_SLASH = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_SLASH) {
    clear_slot(KEY_SLASH); // clear slot that contains key name
    old_key_SLASH = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Semicolon Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_SEMICOLON && (!slots_full)) {
    load_slot(KEY_SEMICOLON); //update first available slot with key name
    old_key_SEMICOLON = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_SEMICOLON) {
    clear_slot(KEY_SEMICOLON); // clear slot that contains key name
    old_key_SEMICOLON = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Left Brace Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_LEFT_BRACE && (!slots_full)) {
    load_slot(KEY_LEFT_BRACE); //update first available slot with key name
    old_key_LEFT_BRACE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_LEFT_BRACE) {
    clear_slot(KEY_LEFT_BRACE); // clear slot that contains key name
    old_key_LEFT_BRACE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************P Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_P && (!slots_full)) {
    load_slot(KEY_P); //update first available slot with key name
    old_key_P = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_P) {
    clear_slot(KEY_P); // clear slot that contains key name
    old_key_P = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************MINUS Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_MINUS && (!slots_full)) {
    load_slot(KEY_MINUS); //update first available slot with key name
    old_key_MINUS = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_MINUS) {
    clear_slot(KEY_MINUS); // clear slot that contains key name
    old_key_MINUS = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************BACKSPACE Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_BACKSPACE && (!slots_full)) {
    load_slot(KEY_BACKSPACE); //update first available slot with key name
    old_key_BACKSPACE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_BACKSPACE) {
    clear_slot(KEY_BACKSPACE); // clear slot that contains key name
    old_key_BACKSPACE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row3); // send row back to off state
// ------------------------------------------------------Row 4-----------------------------
  go_0(Row4); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************INSERT Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col0) && old_key_INSERT && (!slots_full)) {
    load_slot(KEY_INSERT); //update first available slot with key name
    old_key_INSERT = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_INSERT) {
    clear_slot(KEY_INSERT); // clear slot that contains key name
    old_key_INSERT = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************BACKSLASH Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_BACKSLASH && (!slots_full)) {
    load_slot(KEY_BACKSLASH); //update first available slot with key name
    old_key_BACKSLASH = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_BACKSLASH) {
    clear_slot(KEY_BACKSLASH); // clear slot that contains key name
    old_key_BACKSLASH = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************HOME Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_HOME && (!slots_full)) {
    load_slot(KEY_HOME); //update first available slot with key name
    old_key_HOME = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_HOME) {
    clear_slot(KEY_HOME); // clear slot that contains key name
    old_key_HOME = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************L Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_L && (!slots_full)) {
    load_slot(KEY_L); //update first available slot with key name
    old_key_L = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_L) {
    clear_slot(KEY_L); // clear slot that contains key name
    old_key_L = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************DELETE Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_DELETE && (!slots_full)) {
    load_slot(KEY_DELETE); //update first available slot with key name
    old_key_DELETE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_DELETE) {
    clear_slot(KEY_DELETE); // clear slot that contains key name
    old_key_DELETE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row4); // send row back to off state
// ------------------------------------------------------Row 5-----------------------------
  go_0(Row5); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F10 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col0) && old_key_F10 && (!slots_full)) {
    load_slot(KEY_F10); //update first available slot with key name
    old_key_F10 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F10) {
    clear_slot(KEY_F10); // clear slot that contains key name
    old_key_F10 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************COMMA Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_COMMA && (!slots_full)) {
    load_slot(KEY_COMMA); //update first available slot with key name
    old_key_COMMA = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_COMMA) {
    clear_slot(KEY_COMMA); // clear slot that contains key name
    old_key_COMMA = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************PERIOD Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_PERIOD && (!slots_full)) {
    load_slot(KEY_PERIOD); //update first available slot with key name
    old_key_PERIOD = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_PERIOD) {
    clear_slot(KEY_PERIOD); // clear slot that contains key name
    old_key_PERIOD = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************i Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_I && (!slots_full)) {
    load_slot(KEY_I); //update first available slot with key name
    old_key_I = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_I) {
    clear_slot(KEY_I); // clear slot that contains key name
    old_key_I = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************0 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_0 && (!slots_full)) {
    load_slot(KEY_0); //update first available slot with key name
    old_key_0 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_0) {
    clear_slot(KEY_0); // clear slot that contains key name
    old_key_0 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************9 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_9 && (!slots_full)) {
    load_slot(KEY_9); //update first available slot with key name
    old_key_9 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_9) {
    clear_slot(KEY_9); // clear slot that contains key name
    old_key_9 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************F Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_F && (!slots_full)) {
    load_slot(KEY_F); //update first available slot with key name
    old_key_F = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_F) {
    clear_slot(KEY_F); // clear slot that contains key name
    old_key_F = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************F11 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_F11 && (!slots_full)) {
    load_slot(KEY_F11); //update first available slot with key name
    old_key_F11 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_F11) {
    clear_slot(KEY_F11); // clear slot that contains key name
    old_key_F11 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row5); // send row back to off state
// ------------------------------------------------------Row 6-----------------------------
  go_0(Row6); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F8 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col0) && old_key_F8 && (!slots_full)) {
    load_slot(KEY_F8); //update first available slot with key name
    old_key_F8 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F8) {
    clear_slot(KEY_F8); // clear slot that contains key name
    old_key_F8 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************M Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_M && (!slots_full)) {
    load_slot(KEY_M); //update first available slot with key name
    old_key_M = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_M) {
    clear_slot(KEY_M); // clear slot that contains key name
    old_key_M = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************B Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_B && (!slots_full)) {
    load_slot(KEY_B); //update first available slot with key name
    old_key_B = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_B) {
    clear_slot(KEY_B); // clear slot that contains key name
    old_key_B = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************8 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_8 && (!slots_full)) {
    load_slot(KEY_8); //update first available slot with key name
    old_key_8 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_8) {
    clear_slot(KEY_8); // clear slot that contains key name
    old_key_8 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************U Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_U && (!slots_full)) {
    load_slot(KEY_U); //update first available slot with key name
    old_key_U = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_U) {
    clear_slot(KEY_U); // clear slot that contains key name
    old_key_U = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************O Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_O && (!slots_full)) {
    load_slot(KEY_O); //update first available slot with key name
    old_key_O = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_O) {
    clear_slot(KEY_O); // clear slot that contains key name
    old_key_O = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************J Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_J && (!slots_full)) {
    load_slot(KEY_J); //update first available slot with key name
    old_key_J = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_J) {
    clear_slot(KEY_J); // clear slot that contains key name
    old_key_J = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************F9 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_F9 && (!slots_full)) {
    load_slot(KEY_F9); //update first available slot with key name
    old_key_F9 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_F9) {
    clear_slot(KEY_F9); // clear slot that contains key name
    old_key_F9 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row6); // send row back to off state
// ------------------------------------------------------Row 7-----------------------------
  go_0(Row7); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F7 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pushed
  if (!digitalRead(Col0) && old_key_F7 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F7); //update first available slot with key name
    old_key_F7 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F7) {
    clear_slot(KEY_F7); // clear slot that contains key name
    old_key_F7 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
   // Check if Fn and F7 keys are pressed
  if (!digitalRead(Col0) && (!Fn_pressed)) {  // send On_Off low then send back to high Z
    go_0(On_Off);
    while (!digitalRead(Col0)) // wait until F7 key is released
    ;
    go_z(On_Off);
  } 
// *****************N Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_N && (!slots_full)) {
    load_slot(KEY_N); //update first available slot with key name
    old_key_N = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_N) {
    clear_slot(KEY_N); // clear slot that contains key name
    old_key_N = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************G Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_G && (!slots_full)) {
    load_slot(KEY_G); //update first available slot with key name
    old_key_G = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_G) {
    clear_slot(KEY_G); // clear slot that contains key name
    old_key_G = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Y Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_Y && (!slots_full)) {
    load_slot(KEY_Y); //update first available slot with key name
    old_key_Y = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_Y) {
    clear_slot(KEY_Y); // clear slot that contains key name
    old_key_Y = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************K Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_K && (!slots_full)) {
    load_slot(KEY_K); //update first available slot with key name
    old_key_K = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_K) {
    clear_slot(KEY_K); // clear slot that contains key name
    old_key_K = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************7 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_7 && (!slots_full)) {
    load_slot(KEY_7); //update first available slot with key name
    old_key_7 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_7) {
    clear_slot(KEY_7); // clear slot that contains key name
    old_key_7 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************H Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_H && (!slots_full)) {
    load_slot(KEY_H); //update first available slot with key name
    old_key_H = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_H) {
    clear_slot(KEY_H); // clear slot that contains key name
    old_key_H = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************6 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_6 && (!slots_full)) {
    load_slot(KEY_6); //update first available slot with key name
    old_key_6 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_6) {
    clear_slot(KEY_6); // clear slot that contains key name
    old_key_6 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row7); // send row back to off state
// ------------------------------------------------------Row 8-----------------------------
  go_0(Row8); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F5 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and Fn not pressed
  if (!digitalRead(Col0) && old_key_F5 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F5); //update first available slot with key name
    old_key_F5 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F5) {
    clear_slot(KEY_F5); // clear slot that contains key name
    old_key_F5 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F5 keys are pushed for Brightness decrease
  if (!digitalRead(Col0) && (!Fn_pressed)) { // move thru the menus to decrease brightness
    pulse_menu();
    pulse_menu();
    pulse_menu();
    while (!digitalRead(Col0)) { // repeat until F5 key is released
      pulse_vol_dn();
    }
  } 
// *****************V Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_V && (!slots_full)) {
    load_slot(KEY_V); //update first available slot with key name
    old_key_V = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_V) {
    clear_slot(KEY_V); // clear slot that contains key name
    old_key_V = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************S Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_S && (!slots_full)) {
    load_slot(KEY_S); //update first available slot with key name
    old_key_S = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_S) {
    clear_slot(KEY_S); // clear slot that contains key name
    old_key_S = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************T Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_T && (!slots_full)) {
    load_slot(KEY_T); //update first available slot with key name
    old_key_T = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_T) {
    clear_slot(KEY_T); // clear slot that contains key name
    old_key_T = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************R Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_R && (!slots_full)) {
    load_slot(KEY_R); //update first available slot with key name
    old_key_R = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_R) {
    clear_slot(KEY_R); // clear slot that contains key name
    old_key_R = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************5 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_5 && (!slots_full)) {
    load_slot(KEY_5); //update first available slot with key name
    old_key_5 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_5) {
    clear_slot(KEY_5); // clear slot that contains key name
    old_key_5 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************C Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_C && (!slots_full)) {
    load_slot(KEY_C); //update first available slot with key name
    old_key_C = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_C) {
    clear_slot(KEY_C); // clear slot that contains key name
    old_key_C = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************F6 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and Fn not pressed
  if (!digitalRead(Col7) && old_key_F6 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F6); //update first available slot with key name
    old_key_F6 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_F6) {
    clear_slot(KEY_F6); // clear slot that contains key name
    old_key_F6 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
    // Check if Fn and F6 keys are pushed for Brightness increase
  if (!digitalRead(Col7) && (!Fn_pressed)) { // move thru the menus to get to the brightness adjust
    pulse_menu();
    pulse_menu();
    pulse_menu();
    while (!digitalRead(Col7)) { // repeat until F6 key is released
      pulse_vol_up(); //Pulse Vol_Up low to Increase brightness
    }
  }   
  go_z(Row8); // send row back to off state
// ------------------------------------------------------Row 9-----------------------------
  go_0(Row9); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F3 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pushed
  if (!digitalRead(Col0) && old_key_F3 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F3); //update first available slot with key name
    old_key_F3 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F3) {
    clear_slot(KEY_F3); // clear slot that contains key name
    old_key_F3 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F3 keys are pushed
  if (!digitalRead(Col0) && (!Fn_pressed)) {  // send volumn down low then send back to high Z
      go_0(Vol_Dn);
      while (digitalRead(Col0) == LOW) // wait until F3 key is released
      ;
      delay(1);  // wait for switch bounce to end
      go_z(Vol_Dn);
  } 
// *****************X Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_X && (!slots_full)) {
    load_slot(KEY_X); //update first available slot with key name
    old_key_X = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_X) {
    clear_slot(KEY_X); // clear slot that contains key name
    old_key_X = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************E Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_E && (!slots_full)) {
    load_slot(KEY_E); //update first available slot with key name
    old_key_E = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_E) {
    clear_slot(KEY_E); // clear slot that contains key name
    old_key_E = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************4 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_4 && (!slots_full)) {
    load_slot(KEY_4); //update first available slot with key name
    old_key_4 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_4) {
    clear_slot(KEY_4); // clear slot that contains key name
    old_key_4 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************3 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_3 && (!slots_full)) {
    load_slot(KEY_3); //update first available slot with key name
    old_key_3 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_3) {
    clear_slot(KEY_3); // clear slot that contains key name
    old_key_3 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************D Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_D && (!slots_full)) {
    load_slot(KEY_D); //update first available slot with key name
    old_key_D = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_D) {
    clear_slot(KEY_D); // clear slot that contains key name
    old_key_D = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************F4 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pushed
  if (!digitalRead(Col7) && old_key_F4 && (!slots_full) && Fn_pressed)  {
    load_slot(KEY_F4); //update first available slot with key name
    old_key_F4 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_F4) {
    clear_slot(KEY_F4); // clear slot that contains key name
    old_key_F4 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F4 keys are pushed
    if (!digitalRead(Col7) && (!Fn_pressed)) {  // send volumn up low then send back to high Z
      go_0(Vol_Up);
      while (digitalRead(Col7) == LOW) // wait until F4 key is released
      ;
      delay(1);  // wait for switch bounce to end
      go_z(Vol_Up);
  }
  go_z(Row9); // send row back to off state
// ------------------------------------------------------Row 10-----------------------------
  go_0(Row10); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// *****************F1 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pushed
  if (!digitalRead(Col0) && old_key_F1 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F1); //update first available slot with key name
    old_key_F1 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_F1) {
    clear_slot(KEY_F1); // clear slot that contains key name
    old_key_F1 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F1 keys are pressed
  if (!digitalRead(Col0) && (!Fn_pressed)) {  // send menu low then send back to high Z
    go_0(Menu);
    while (digitalRead(Col0) == LOW) // wait until F1 key is released
    ;
    go_z(Menu);
  } 
// *****************Z Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col1) && old_key_Z && (!slots_full)) {
    load_slot(KEY_Z); //update first available slot with key name
    old_key_Z = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col1) && !old_key_Z) {
    clear_slot(KEY_Z); // clear slot that contains key name
    old_key_Z = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************SPACE Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_SPACE && (!slots_full)) {
    load_slot(KEY_SPACE); //update first available slot with key name
    old_key_SPACE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_SPACE) {
    clear_slot(KEY_SPACE); // clear slot that contains key name
    old_key_SPACE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************Q Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col3) && old_key_Q && (!slots_full)) {
    load_slot(KEY_Q); //update first available slot with key name
    old_key_Q = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col3) && !old_key_Q) {
    clear_slot(KEY_Q); // clear slot that contains key name
    old_key_Q = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************2 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_2 && (!slots_full)) {
    load_slot(KEY_2); //update first available slot with key name
    old_key_2 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_2) {
    clear_slot(KEY_2); // clear slot that contains key name
    old_key_2 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// *****************1 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_1 && (!slots_full)) {
    load_slot(KEY_1); //update first available slot with key name
    old_key_1 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col5) && !old_key_1) {
    clear_slot(KEY_1); // clear slot that contains key name
    old_key_1 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// ****************W Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col6) && old_key_W && (!slots_full)) {
    load_slot(KEY_W); //update first available slot with key name
    old_key_W = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col6) && !old_key_W) {
    clear_slot(KEY_W); // clear slot that contains key name
    old_key_W = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// ****************F2 Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty and the Fn key is not pressed
  if (!digitalRead(Col7) && old_key_F2 && (!slots_full) && Fn_pressed) {
    load_slot(KEY_F2); //update first available slot with key name
    old_key_F2 = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time 
  else if (digitalRead(Col7) && !old_key_F2) {
    clear_slot(KEY_F2); // clear slot that contains key name
    old_key_F2 = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if Fn and F2 keys are pushed for Mute on/off
  if (!digitalRead(Col7) && (!Fn_pressed)) { // move thru the menus to toggle mute on/off
    pulse_menu();
    pulse_vol_up();
    pulse_menu();
    pulse_vol_dn();
    pulse_menu();
    pulse_vol_dn();
    delay(5000); // Wait until Menu screen goes away  
  } 
  go_z(Row10); // send row back to off state
// ------------------------------------------------------Row 12-----------------------------
  go_0(Row12); // Activate Row (send it low), then read the columns
  //
  delayMicroseconds(10); // give time to let the signals settle out
  //
// ****************TILDE Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col0) && old_key_TILDE && (!slots_full)) {
    load_slot(KEY_TILDE); //update first available slot with key name
    old_key_TILDE = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col0) && !old_key_TILDE) {
    clear_slot(KEY_TILDE); // clear slot that contains key name
    old_key_TILDE = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// ****************A Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col2) && old_key_A && (!slots_full)) {
    load_slot(KEY_A); //update first available slot with key name
    old_key_A = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col2) && !old_key_A) {
    clear_slot(KEY_A); // clear slot that contains key name
    old_key_A = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// ****************TAB Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col4) && old_key_TAB && (!slots_full)) {
    load_slot(KEY_TAB); //update first available slot with key name
    old_key_TAB = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col4) && !old_key_TAB) {
    clear_slot(KEY_TAB); // clear slot that contains key name
    old_key_TAB = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
// ****************CAPS LOCK Key***************************
// Sending KEY_CAPS_LOCK to a windows 7 PC works fine but sending it to a Pi causes the Teensy to lock up if you 
// don't put in a small delay after you send_normals
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col5) && old_key_CAPS_LOCK && (!slots_full)) {
    load_slot(KEY_CAPS_LOCK); //update first available slot with key name
    old_key_CAPS_LOCK = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
    delay(10); //this delay keeps the teensy happy (perhaps it gives the pi time to send caps lock led status)
  }
  // Check if key is released and was pressed last time
  if (digitalRead(Col5) && !old_key_CAPS_LOCK) {
    clear_slot(KEY_CAPS_LOCK); // clear slot that contains key name
    old_key_CAPS_LOCK = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
    delay(10); //this delay keeps the teensy happy (perhaps it gives the pi time to send caps lock led status)
   }
// ****************ESC Key***************************
  // Check if key is pressed and wasn't pressed last time and a usb slot is empty
  if (!digitalRead(Col7) && old_key_ESC && (!slots_full)) {
    load_slot(KEY_ESC); //update first available slot with key name
    old_key_ESC = LOW; //remember key is now pressed
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  // Check if key is released and was pressed last time
  else if (digitalRead(Col7) && !old_key_ESC) {
    clear_slot(KEY_ESC); // clear slot that contains key name
    old_key_ESC = HIGH; // remember key is now released
    send_normals(slot1, slot2, slot3, slot4, slot5, slot6); // use function to send 6 slots over usb
  }
  go_z(Row12); // send row back to off state
// -------------------------------------------------Keyboard scan complete------------------------------------------
//
// 
// --------------------------------------Poll the touchpad for new movement data and button pushes----------------------------------
//
  over_flow = 0; // assume no overflow until status is received 
  touchpad_write(0xeb);  // "eb" = request data
  touchpad_read();      // ignore ack
  mstat = touchpad_read(); // read and save into status variable
  mx = touchpad_read(); // read and save into x variable
  my = touchpad_read(); // read and save into y variable
  if (((0x80 & mstat) == 0x80) || ((0x40 & mstat) == 0x40))  {   // x or y overflow bits set?
    over_flow = 1; // set the overflow flag
  }   
// change the x data from 9 bit to 8 bit 2's complement
  mx = mx >> 1; // convert to 7 bits of data by dividing by 2
  mx = mx & 0x7f; // don't allow sign extension
  if ((0x10 & mstat) == 0x10) {   // move the sign into 
    mx = 0x80 | mx;              // the 8th bit position
  } 
// change the y data from 9 bit to 8 bit 2's complement and then take the 2's complement 
// because y movement on ps/2 format is the opposite direction of Mouse.move function
  my = my >> 1; // convert to 7 bits of data by dividing by 2
  my = my & 0x7f; // don't allow sign extension
  if ((0x20 & mstat) == 0x20) {   // move the sign into 
    my = 0x80 | my;              // the 8th bit position
  } 
  my = (~my + 0x01); // change the sign of y data by taking the 2's complement (invert and add 1)
// zero out mx and my if over_flow is set
  if (over_flow) {   // check if overflow is set
    mx = 0x00;       // data is garbage so zero it out
    my = 0x00;
  } 
// send the x and y data back via usb if either one is non-zero and the touchpad is enabled
  if (((mx != 0x00) || (my != 0x00)) && touchpad_enabled){
    Mouse.move(mx,my);
  }
//
// send the touchpad left and right button status over usb
// using Mouse.set_buttons(LEFT, MIDDLE, RIGHT) function
// 1 for button pressed, 0 for not pressed. Middle is always 0 for this touch pad.
  if ((0x01 & mstat) == 0x01) {   // if left button set 
    left_button = 1;   
  }
  else {   
    left_button = 0; // clear left button  
  }
  if ((0x02 & mstat) == 0x02) {   // if right button set 
    right_button = 1;   
  } 
  else {   
    right_button = 0; // clear right button  
  }
// Determine if the left or right touch pad buttons have changed since last polling cycle
  button_change = (left_button ^ old_left_button) | (right_button ^ old_right_button);
// Don't send button status if there's no change since last time. Only send if touchpad is enabled.
  if (button_change && touchpad_enabled){
    Mouse.set_buttons(left_button, 0, right_button); // send button status
  }
// transfer left and right button status into the "old" variables for the next polling cycle
  old_left_button = left_button;
  old_right_button = right_button;  
// ---------------------------------------------Touchpad complete-------------------------------------------
//
// ************Pi and Teensy Reset via keyboard***********************************************
  // send pi a reset pulse if control-alt-r keys are pressed
  if (!old_key_R && !old_key_ALT && !old_key_CTRL) {  
    reset_all = HIGH;
  }
//
// ************Laptop Shutdown via keyboard***********************************************
  // Turn voltage regulators off if control-alt-s keys are pressed
  if (!old_key_S && !old_key_ALT && !old_key_CTRL) {  
    kill_power = HIGH;
  }
//

// Turn on the Caps Lock LED (by sending a low) if bit D1 in the keyboard_leds variable is set, else turn off the LED.
//
  if ((keyboard_leds & 0x02) == 0x02) {
    go_0(CAPS_LED); // turn on the CAPS LOCK LED
  }
  else {
    go_1(CAPS_LED); // turn off the CAPS LOCK LED
  }
// Look at variables controlled by I2C commands & keyboard
  if (kill_power) {  
    sec_delay(4); // give Pi time to finish shutdown before removing power
    go_1(SHUTDOWN); // send a logic 1 to turn off all power  
  }
  if (reset_all) {
    go_0(RESET_PI); // send Pi reset active low
    delayMicroseconds(300); // reset pulse width
    go_z(RESET_PI); // send reset back to off state (hi Z). Do not drive this to 5 volts or it may damage the Pi.
    _restart_Teensyduino_(); // reset the teensy so it comes up at the same time as the pi
  }
  if (debug) {
    go_0(DISK_LED); // turn on the led with the disk icon 
  }
  else {
    go_1(DISK_LED); // turn off the led with the disk icon 
  }
// Blink LED on Teensy to show it's alive
//
  if (blink_count == 0x0a) {  
    pinMode(BLINK_LED, OUTPUT);
    digitalWrite(BLINK_LED, blinky);
    blinky = !blinky;
    blink_count = 0;
  }
  else {
    blink_count = blink_count + 1;
  }
//
  delay(23);//The keyboard & touchpad scan takes about 7 msec so wait 23 msec before proceeding with next polling cycle              
}

