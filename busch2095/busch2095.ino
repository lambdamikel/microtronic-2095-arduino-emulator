/*
   A Cassette Tape Emulator for the
   Busch 2090 Microtronic Computer System
   for Arduino Uno R3

   (C) 2017 Michael Wessel
   lambdamikel@gmail.com
   https://www.michael-wessel.info

   Idea, Background Research and Raspberry PI Python Version:
   Martin Sauter
   https://blog.wirelessmoves.com/
   https://github.com/martinsauter/Busch-2090-Projects

   This piece of software emulates the 2095 Cassette Interface
   for the Busch 2090 Microtronic Computer System and allows
   to save and load program from/to the Microtronic to an SD Card.

   Required hardware:
   - Arduino Uno R3
   - LCD & Keypad Shield
   - SDCard and Ethernet Shield
   - Cables and 14 PIN IC socket for 2090 connection

   Notes:
   if your LCD buttons don't work properly, have a look at the
   analog threshold values in int readLCDButtons() { ... } and
   adjust as necessary


   License of this code: GPLv3
   https://www.gnu.org/licenses/gpl-3.0.en.html

*/

#include <LiquidCrystal.h>
#include <SPI.h>
#include <SdFat.h>

//
// Ports of the Busch 2090 - either connect
// to the IC Socket for the 2095 interface using
// an 14 PIN DIP IC socket, or connect to the
// external input / outputs of the 2090.
//

#define BUSCH_IN2 A5
#define BUSCH_IN3 A4
#define BUSCH_IN4 A3

#define BUSCH_OUT3 A2
#define BUSCH_OUT2 A1
#define BUSCH_OUT1 PD2

#define BUSCH_IN1  PD3

//
// Busch 2090 read / write port operations delays (ms)
//

#define READ_CLOCK_DELAY 4 // SAVE PGM2 Clock Delay 
#define WRITE_CLOCK_DELAY 10 // LOAD PGM1 Clock Delay 
#define READ_DELAY_NEXT_VALUE 200  // SAVE PGM2 Next Word Delay 
#define WRITE_DELAY_NEXT_VALUE 180 // LOAD PGM1 Next Word Delay

//
// Cursor and LCD display control
//

#define CURSOR_OFF 8
#define DISPLAY 1200
#define BLINK_DELAY 250

byte cursor = CURSOR_OFF;
boolean blink = true;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
typedef char LCDLine[17];
LCDLine file;

//
// SDCard
//

#define SDCARD_CHIP_SELECT 4

SdFat SD;
SdFile root;
File myFile;

//
// LCD Keypad Shield Buttons Mnemonics
//

#define RIGHT  0
#define UP     1
#define DOWN   2
#define LEFT   3
#define SELECT 4
#define NONE   5

int cur_button = 0;

int readLCDButtons() {

  cur_button = analogRead(0);

  // notice: these values may need to be tuned!!
  // and adjusted to your actual LCD Keypad shield
  // the are analog thresholds

  if (cur_button > 850) return NONE;
  if (cur_button < 50)   return RIGHT;
  if (cur_button < 150)  return UP;
  if (cur_button < 350)  return DOWN;
  if (cur_button < 500)  return LEFT;
  if (cur_button < 800)  return SELECT;

  return NONE;

}

void waitForLCDButtonRelease() {
  while (readLCDButtons() != NONE) {};
  return;
}

//
// Arduino Setup
//

void setup() {

  Serial.begin(9600);

  //
  // LCD
  //

  lcd.begin(2, 16);
  showAuthor();

  //
  // PIN Modes
  //

  pinMode(BUSCH_IN1, OUTPUT);
  pinMode(BUSCH_IN2, OUTPUT);
  pinMode(BUSCH_IN3, OUTPUT);
  pinMode(BUSCH_IN4, OUTPUT);

  pinMode(BUSCH_OUT3, INPUT);
  pinMode(BUSCH_OUT2, INPUT);
  pinMode(BUSCH_OUT1, INPUT);

  //
  // Init Pins
  //

  digitalWrite(BUSCH_IN1, LOW);
  digitalWrite(BUSCH_IN2, LOW);
  digitalWrite(BUSCH_IN3, LOW);
  digitalWrite(BUSCH_IN4, LOW);

  //
  // SD Card init
  //

  if (!SD.begin(SDCARD_CHIP_SELECT, SD_SCK_MHZ(50))) {

    lcd.clear();
    lcd.print("SD Card ERROR!");
    SD.initErrorHalt();
    exit(0);
  } else {
    lcd.clear();
    lcd.print("SD Card Ready");
  }

  delay(DISPLAY);
  showInfo();

}

//
// Auxiliary Operations for 2090 Communication
//

int clock(int pin) {

  digitalWrite(pin, LOW);
  delay(READ_CLOCK_DELAY);

  digitalWrite(pin, HIGH);
  delay(READ_CLOCK_DELAY);

  int bit = digitalRead(BUSCH_OUT1);

  return bit;

}

void clockWrite(int pin, int bit) {

  digitalWrite(BUSCH_IN1, bit);

  digitalWrite(pin, HIGH);
  delay(WRITE_CLOCK_DELAY);

  digitalWrite(pin, LOW);
  delay(WRITE_CLOCK_DELAY);

}

int decodeHex(char c) {

  if (c >= 65 && c <= 70 )
    return c - 65 + 10;
  else if ( c >= 48 && c <= 67 )
    return c - 48;
  else return -1;

}

void storeNibble(byte nibble, boolean first) {

  if (first) {

    digitalWrite(BUSCH_IN1, nibble & 0b0001);

    digitalWrite(BUSCH_IN3, LOW);
    delay(WRITE_CLOCK_DELAY);

    digitalWrite(BUSCH_IN3, HIGH);
    delay(WRITE_CLOCK_DELAY);

  } else {

    clockWrite(BUSCH_IN2, nibble & 0b0001);

  }

  clockWrite(BUSCH_IN2, nibble & 0b0010);
  clockWrite(BUSCH_IN2, nibble & 0b0100);
  clockWrite(BUSCH_IN2, nibble & 0b1000);

}

void resetPins() {

  digitalWrite(BUSCH_IN4, LOW);
  digitalWrite(BUSCH_IN3, LOW);
  digitalWrite(BUSCH_IN2, LOW);
  digitalWrite(BUSCH_IN1, LOW);

}

//
// PGM 2 - Save to SD Card
//

void pgm2() {

  int aborted = createName();

  if ( aborted == -1 ) {
    lcd.clear();
    lcd.print("CANCELED!");
    delay(DISPLAY);
    return;
  }

  if (SD.exists(file)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Overwriting");
    lcd.setCursor(0, 1);
    lcd.print(file);
    SD.remove(file);
    delay(DISPLAY);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving");
  lcd.setCursor(0, 1);
  lcd.print(file);
  delay(DISPLAY);

  myFile = SD.open( file , FILE_WRITE);

  if (! myFile ) {
    lcd.clear();
    lcd.print("SD ERROR!");
    delay(DISPLAY);
    return;
  }

  //
  //
  //

  cursor = CURSOR_OFF;
  int pc = 0;

  //
  //
  //

  resetPins();  
  delay(READ_DELAY_NEXT_VALUE);

  //
  //
  //

  clock(BUSCH_IN3);

  for (int x = 0; x < 256; x++) {

    delay(READ_DELAY_NEXT_VALUE);

    int bit = clock(BUSCH_IN4);

    delay(READ_CLOCK_DELAY);

    int nibble = bit;

    if (!digitalRead(BUSCH_OUT3)) {
      break;
    }

    lcd.setCursor(0, 0);
    lcd.print("PC ");
    lcd.print(pc / 16, 16);
    lcd.print(pc % 16, 16);
    lcd.print(" : ");
    lcd.print(pc);
    lcd.print("     ");

    lcd.setCursor(0, 1);
    lcd.print("OP ");

    for (int i = 1; i < 12; i++) {
      bit = clock(BUSCH_IN2);
      int pos = i % 4;
      nibble |= (bit << pos);
      if (pos == 3) {
        lcd.print(nibble, 16);
        myFile.print(nibble, 16);
        nibble = 0;
      }
    }

    clock(BUSCH_IN2);
    clock(BUSCH_IN2);

    lcd.print(" -> SAVE");
    myFile.println();

    pc++;

  }

  myFile.close();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saved");
  lcd.setCursor(0, 1);
  lcd.print(file);
  delay(DISPLAY);

  //
  //
  //
  
  resetPins();  
  return;

}

//
// PGM 1 - Load from SD Card
//

void pgm1() {

  int aborted = selectFile();

  if ( aborted == -1 ) {
    lcd.clear();
    lcd.print("CANCELED!");
    delay(DISPLAY);
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading");
  lcd.setCursor(0, 1);
  lcd.print(file);
  delay(DISPLAY);

  myFile = SD.open( file , FILE_READ);

  if (! myFile || ! myFile.isOpen()) {
    lcd.clear();
    lcd.print("SD ERROR!");
    delay(DISPLAY);
    return;
  }

  cursor = CURSOR_OFF;
  boolean readingComment = false;
  boolean readingOrigin = false;

  //
  //
  //

  int pc = 0;
  int count = 0;

  int curX = 3;
  int curY = 1;

  byte op = 0;
  byte arg1 = 0;
  byte arg2 = 0;

  //
  //
  //

  resetPins();  
  delay(WRITE_DELAY_NEXT_VALUE);

  //
  //
  //

  while (1) {

    if (count == 0) {
      lcd.setCursor(0, 0);
      lcd.print("PC ");
      lcd.print(pc / 16, 16);
      lcd.print(pc % 16, 16);
      lcd.print(" : ");
      lcd.print(pc);
      lcd.print("        ");

      lcd.setCursor(0, 1);
      lcd.print("OP ");

    }

    int b = myFile.read();
    if (b == -1)
      break;

    if (b == '\n' || b == '\r') {
      readingComment = false;
      readingOrigin = false;
    } else if (b == '#') {
      readingComment = true;
    } else if (b == '@') {
      readingOrigin = true;
    }

    if (!readingComment && !readingOrigin &&
        b != '\r' && b != '\n' && b != '\t' && b != ' ' && b != '@' ) { // skip whitespace

      switch ( b ) {
        case 'I' : b = '1'; break; // correct for some common OCR errors
        case 'l' : b = '1'; break;
        case 'P' : b = 'D'; break;
        case 'Q' : b = '0'; break;
        case 'O' : b = '0'; break;
      }

      int decoded = decodeHex(b);

      if ( decoded == -1) {
        lcd.clear();
        lcd.print("ERROR!");
        lcd.setCursor(0, 1);
        lcd.print("Byte ");
        lcd.print(count);
        lcd.print(" ");
        lcd.print((char) b);
        delay(DISPLAY);
        break;
      }

      lcd.setCursor(curX, curY);
      lcd.write(b);
      curX++;

      switch ( count ) {
        case 0 : op  = decoded; count = 1;  break;
        case 1 : arg1 = decoded; count = 2; break;
        case 2 :

          arg2 = decoded;
          count = 0;

          lcd.setCursor(curX, curY);
          lcd.write(" -> LOAD");
          curX = 3;

          delay(WRITE_DELAY_NEXT_VALUE);

          //
          // write data
          //

          storeNibble(op, true);
          storeNibble(arg1, false);
          storeNibble(arg2, false);


          delay(WRITE_CLOCK_DELAY);

          pc++;

      }
    }
  }

  myFile.close();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loaded: RESET!");
  lcd.setCursor(0, 1);
  lcd.print(file);
  delay(DISPLAY);

  //
  //
  //
  
  resetPins();  
  return;

}

//
// File Browser & Filename Creation Operations
//

void clearFileBuffer() {

  for (int i = 0; i < 16; i++)
    file[i] = ' ';

  file[16] = 0;

}

int selectFileNo(int no) {
  int count = 0;

  clearFileBuffer();

  SD.vwd()->rewind();
  while (root.openNext(SD.vwd(), O_READ)) {
    if (! root.isDir()) {
      count++;
      root.getName(file, 20);
      if (count == no ) {
        root.close();
        return count;
      }
    }
    root.close();
  }

  return 0;

}

int countFiles() {

  int count = 0;

  SD.vwd()->rewind();
  while (root.openNext(SD.vwd(), O_READ)) {
    if (! root.isDir())
      count++;
    root.close();
  }

  return count;

}

int selectFile() {

  lcd.clear();

  int no = 1;
  int count = countFiles();
  selectFileNo(no);

  unsigned long last = millis();
  boolean blink = false;

  waitForLCDButtonRelease();

  while ( readLCDButtons() !=  SELECT ) {

    lcd.setCursor(0, 0);
    lcd.print("Load MIC ");
    lcd.print(no);
    lcd.print("/");
    lcd.print(count);
    lcd.print("    "); // erase previoulsy left characters if number got smaller

    if ( millis() - last > BLINK_DELAY) {

      last = millis();
      lcd.setCursor(0, 1);
      blink = !blink;

      if (blink)
        lcd.print("                 ");
      else
        lcd.print(file);
    }

    switch ( readLCDButtons() ) {
      case UP : waitForLCDButtonRelease(); if (no < count) no = selectFileNo(no + 1); else no = selectFileNo(1);  break;
      case DOWN : waitForLCDButtonRelease(); if (no > 1) no = selectFileNo(no - 1); else no = selectFileNo(count); break;
      case LEFT : waitForLCDButtonRelease(); return -1;
      default : break;
    }
  }

  return 0;

}

int createName() {

  lcd.clear();

  waitForLCDButtonRelease();

  clearFileBuffer();
  file[0] = 'P';
  file[1] = 0;

  int cursor = 0;
  int length = 1;

  unsigned long last = millis();
  boolean blink = false;

  readLCDButtons();

  while ( readLCDButtons() != SELECT ) {

    lcd.setCursor(0, 0);
    lcd.print("Save MIC - Name:");

    if ( millis() - last > 100) {

      last = millis();

      lcd.setCursor(0, 1);
      lcd.print(file);
      lcd.setCursor(cursor, 1);

      blink = !blink;

      if (blink)
        if (file[cursor] == ' ' )
          lcd.print("_");
        else
          lcd.print(file[cursor]);
      else
        lcd.print("_");
    }

    switch ( readLCDButtons() ) {
      case UP : file[cursor]++; waitForLCDButtonRelease(); break;
      case DOWN : file[cursor]--; waitForLCDButtonRelease(); break;
      case RIGHT : cursor++; waitForLCDButtonRelease(); break;
      case LEFT : waitForLCDButtonRelease(); return -1;
      default : break;
    }

    if (cursor < 0)
      cursor = 0;
    else if (cursor > 7)
      cursor = 7;

    if (cursor > length - 1 ) {
      file[cursor] = '0';
      length++;
    }
  }

  cursor++;
  file[cursor++] = '.';
  file[cursor++] = 'M';
  file[cursor++] = 'I';
  file[cursor++] = 'C';
  file[cursor++] = 0;

  return 0;

}

//
// LCD Info & Author Info
//

void showInfo() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LEFT   PGM1/LOAD");
  lcd.setCursor(0, 1);
  lcd.print("RIGHT  PGM2/SAVE");
}

void showAuthor() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2095 TapeArduEmu");
  lcd.setCursor(0, 1);
  lcd.print("by Sauter&Wessel");
  delay(DISPLAY);
}

//
// Arduino Main Loop
//

void loop() {

  switch ( readLCDButtons() ) {
    case RIGHT: {
        waitForLCDButtonRelease();
        pgm2();
        showInfo();
        break;
      }
    case LEFT: {
        waitForLCDButtonRelease();
        pgm1();
        showInfo();
        break;
      }
    case UP:
    case DOWN:
    case SELECT: {
        waitForLCDButtonRelease();
        showAuthor();
        showInfo();
        break;
      }
  }
}


