
/*
   This file is free software; you can redistribute it and/or modify
   it under the terms of either the GNU General Public License version 2
   or the GNU Lesser General Public License version 2.1, both as
   published by the Free Software Foundation.
*/

#include <SPI.h>
#include "SdFat.h"


uint32_t sampleRate = 22050;
String workingFolder = "lang/";
String fnNotFound = workingFolder + "notFound.wav";
String rdir = "";
String dirIndexSt[100];
int indexLength = 0;

#define SUsb SerialUSB
#define SSer Serial

const uint8_t SD_CS_PIN = 4;
SdFat sd;
File file;
SdFile dirFile;


//File myFile;
signed int s;

bool __StartFlag;
volatile uint32_t __SampleIndex;
uint32_t __HeadIndex;
uint32_t __NumberOfSamples; // Number of samples to read in block
uint8_t *__WavSamples;


String first14[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen" };
String prefixes[] = {"twen", "thir", "for", "fif", "six", "seven", "eigh", "nine"};


void setup() {
  pinMode(13, OUTPUT);
  pinMode(10, INPUT);
  SUsb.begin(9600);
  SSer.begin(9600);
  Sprintln("Setup is starting.");
  for (byte i = 0; i < 10; i++) {
    delay(300);
    digitalWrite(13, LOW);
    delay(300);
    digitalWrite(13, HIGH);
  }
  digitalWrite(13, LOW);

  if (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(50))) {
    Sprintln(F("No SD found."));
    while (1) {
      delay(50);
      digitalWrite(13, LOW);
      delay(100);
      digitalWrite(13, HIGH);
    }
    sd.initErrorHalt();
  }

  __StartFlag = false;
  __SampleIndex = 0;          //in order to start from the beginning
  __NumberOfSamples = 1024; //samples to read to have a buffer

  __WavSamples = (uint8_t *) malloc(__NumberOfSamples * sizeof(uint8_t));
  for (int i = 0; i < __NumberOfSamples - 1; i++)
    __WavSamples[i] = 512;

  analogWriteResolution(10); //set the Arduino DAC for 10 bits of resolution (max)
  tcConfigure(sampleRate);
  tcStartCounter();

  loadIndex(workingFolder + "langDir.txt");
  if (rdir == "") {
    Sprintln("\n\nMake sure file lang/langDir.txt existSUsb.\nTerminating.");
    while (1) {
      delay(100);
      digitalWrite(13, LOW);
      delay(500);
      digitalWrite(13, HIGH);
    }
  }


  Sprintln("looping... Type a text or '#help' for help.");

  //  playSentence("Boot");
  // playSentence("this is a 123.45");
  //playFile("talk.wav");
  //  playFile("music.wav");
  // playSentence("aircraft ABC 1 2 3 4 5 6 7 8 9 10 11 12 13   about answer another above anchor air aircraft ahead apart anxiety");
  // playFile("en-7k/seatbe/seven.wav");
}

void loop() {
  consoleRead();
  while (digitalRead(10) == 0) {
    Sprintln("This is a test");
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
    digitalWrite(13, HIGH);
    playSentence("This is a test");
    digitalWrite(13, LOW);
  }
}


void consoleRead() {
  String st = "";
  while (SUsb.available() > 0) {
    char c = SUsb.read();
    st += c;
  }
  while (SSer.available() > 0) {
    delay(10);
    char c = SSer.read();
    st += c;
  }
  if (st != "") {

    if (st.substring(0, 1) == "#") {
      st.replace("\r", "");
      st.replace("\n", "");

      Sprint("command "); Sprintln(st);
      if (st.substring(1, 5) == "help") {
        Sprintln("\n\nHelp file for Commands\n");
        Sprintln("#play <dir>/<file>  example #play /lang/custom/myspeech.wav");
        Sprintln("#dir <new working dir>  example #dir en/7k/ ");
        Sprintln("\n\n ");
      }
      if (st.substring(1, 5) == "play") {
        playFile(st.substring(6, st.length()));
      }
      if (st.substring(1, 4) == "dir") {
        rdir = workingFolder + st.substring(5, st.length());
        loadIndex(rdir);
      }
    }
    else {
      st.toLowerCase();
      playSentence(st);
    }
  }
}


void Sprintln(String st) {
  SUsb.println(st);
  SSer.println(st);
}
void Sprint(String st) {
  SUsb.print(st);
  SSer.print(st);
}

int intIndex(String st) {
  for (int i = 0; i <= st.length(); i++) {
    if ((st.charAt(i) >= '0') && (st.charAt(i) <= '9'))
      return i;
  }
  return -1;
}

int intIndexEnd(String st, int ni) {
  for (int i = ni; i <= st.length(); i++) {
    if ((st.charAt(i) < '0') || (st.charAt(i) > '9'))
      return i ;
  }
  return -1;
}

String replaceNumberWithStrings(String st) {
  int numberStartIndex = intIndex(st);
  String beforeNumberString = "";
  String afterNumberString = "";
  String numberFoundString = "";
  while (numberStartIndex >= 0) {
    int numberStartIndexEnd = intIndexEnd(st, numberStartIndex);
    beforeNumberString = st.substring(0, numberStartIndex);
    afterNumberString = st.substring(numberStartIndexEnd, st.length());
    char cbefore = st.charAt(numberStartIndexEnd - 1);
    char cafter = st.charAt(numberStartIndexEnd + 1);
    numberFoundString = st.substring(numberStartIndex, numberStartIndexEnd);
    if (numberFoundString.toInt() != 0) numberFoundString =  inttostr(numberFoundString.toInt()) ;
    else numberFoundString = "";
    if ((st.charAt(numberStartIndexEnd) == '.') && (cbefore >= '0') && (cbefore <= '9') && (cafter >= '0') && (cafter <= '9')) {
      st = beforeNumberString + numberFoundString + " point " + afterNumberString.substring(1, afterNumberString.length());
    }
    else     if ((st.charAt(numberStartIndexEnd) == ',') && (cbefore >= '0') && (cbefore <= '9') && (cafter >= '0') && (cafter <= '9')) {
      st = beforeNumberString + numberFoundString + " comma " + afterNumberString.substring(1, afterNumberString.length());
    }
    else {
      st = beforeNumberString + numberFoundString + afterNumberString;
    }
    numberStartIndex = intIndex(st);
  }
  return st;
}

String convertSentenceEN(String st) {
  String cst = st;
  st.toLowerCase();
  cst = replaceNumberWithStrings(cst);

  for (byte c = 33; c < 43; c++)  cst.replace(char(c), ' ');
  cst.replace("+", " plus ");
  cst.replace("-", " minus ");
  cst.replace(",", " ");
  cst.replace(".", " ");
  cst.replace("'s", " ");
  for (byte c = 58; c < 65; c++) cst.replace(char(c), ' ');
  for (byte c = 91; c < 61; c++) cst.replace(char(c), ' ');
  for (byte c = 123; c < 255; c++) cst.replace(char(c), ' ');
  return cst;
}

bool doubleBlanks(String st) {
  for (int i = 0; i < st.length() ; i++) {
    if ((st.charAt(i) == ' ') && (st.charAt(i + 1) == ' ')) return 1;
  }
  return 0;
}
String skipBlank(String st) {
  while (st.charAt(0) == ' ') st = st.substring(1, st.length());
  while (doubleBlanks(st) == 1) {
    for (int i = 0; i < st.length() ; i++) {
      if ((st.charAt(i) == ' ') && (st.charAt(i + 1) == ' ')) st.remove(i, 1);
    }
  }
  return st;
}

void playSentence(String st) {
  st = convertSentenceEN(st);
  st = skipBlank(st);

  Sprintln("Playing : " + st);
  while (st.length() > 0) {
    String pst = st.substring(0, st.indexOf(' '));
    if (st.indexOf(' ') > 0)
      st = st.substring(st.indexOf(' ') + 1, st.length());
    else
      st = "";
    playWord(pst);
  }
}




//https://codereview.stackexchange.com/questions/30525/convert-number-to-words
String inttostr( const unsigned int number )
{
  if ( number <= 14 )
    return first14[number];
  if ( number < 20 )
    return prefixes[number - 12] + "teen";
  if ( number < 100 ) {
    unsigned int remainder = number - (static_cast<int>(number / 10) * 10);
    return prefixes[number / 10 - 2] + (0 != remainder ? "ty " + inttostr(remainder) : "ty");
  }

  if ( number < 1000 ) {
    unsigned int remainder = number - (static_cast<int>(number / 100) * 100);
    return first14[number / 100] + (0 != remainder ? " hundred " + inttostr(remainder) : " hundred");
  }
  if ( number < 1000000 ) {
    unsigned int thousands = static_cast<int>(number / 1000);
    unsigned int remainder = number - (thousands * 1000);
    return inttostr(thousands) + (0 != remainder ? " thousand " + inttostr(remainder) : " thousand");
  }
  if ( number < 1000000000 ) {
    unsigned int millions = static_cast<int>(number / 1000000);
    unsigned int remainder = number - (millions * 1000000);
    return inttostr(millions) + (0 != remainder ? " million " + inttostr(remainder) : " million");
  }
}

void loadIndex(String wdir) {
  File fn = sd.open(wdir);
  delay(1000);
  if (fn.available()) {
    rdir = workingFolder + fn.readStringUntil('\n');
  }
  Sprint("Active directory "); Sprintln(rdir);
  if (rdir != "") {
    fn = sd.open(rdir + "dirIndex.txt");
    delay(1000);
    if (!fn.available()) {
      Sprintln("No index file at :" + rdir + "dirindex.txt");
      while (1) {};
    }
    int c = 0;
    while (fn.available()) {
      String st = fn.readStringUntil('\n');
      dirIndexSt[c] = st;
      c++;
    }
    indexLength = c;
  }
  else {
    Sprintln("\n\nfile : " + workingFolder + "langDir.txt not found");
  }
}


String wordPath(String  w) {
  w.toLowerCase();
  for (byte i = 0; i < indexLength; i++) {
    if ((i + 1) < indexLength) {
      if ((w >= dirIndexSt[i]) && (w <= dirIndexSt[i + 1]))
      {
        String found = rdir + dirIndexSt[i] + "/" + w + ".wav";
        found.replace(" ", "");
        return found;
      }
    }
  }
  if (w >= dirIndexSt[indexLength - 1] ) {
    String found = rdir + dirIndexSt[indexLength - 1] + "/" + w + ".wav";
    found.replace(" ", "");
    return found;
  }
  return "NotFound";
}

void playWord(String w) {
  w.replace(" ", "");
  w.replace("\r", "");
  w.replace("\n", "");


  if (w.length() > 0) {
    playFile(wordPath(w));
  }
}


/*
   Copyright (c) 2015 by
  Arturo Guadalupi <a.guadalupi@arduino.cc>
  Angelo Scialabba <a.scialabba@arduino.cc>
  Claudio Indellicati <c.indellicati@arduino.cc> <bitron.it@gmail.com>

   Audio library for Arduino Zero.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either the GNU General Public License version 2
   or the GNU Lesser General Public License version 2.1, both as
   published by the Free Software Foundation.
*/

int stime = 0;
byte playing = 0;

void playFile(String fn) {
  fn.trim();
  file = sd.open(fn);
  if (!file.available()) {
    Sprint("file not found  :");
    Sprintln(fn);
    file = sd.open(fnNotFound);
  }
  __StartFlag = false;
  __WavSamples = (uint8_t *) malloc(__NumberOfSamples * sizeof(uint8_t));
  while (file.available()) {
    if (!__StartFlag)
    {
      for (int i = 0; i < 1000; i++) {
        byte c = file.read();
      }
      file.read(__WavSamples, __NumberOfSamples);
      __HeadIndex = 0;
      __SampleIndex = 0;
      __StartFlag = true;
      tcConfigure(sampleRate);
      tcStartCounter();
    }
    else
    {
      delay(10);
      uint32_t current__SampleIndex = __SampleIndex;
      if (current__SampleIndex > __HeadIndex) {
        file.read(&__WavSamples[__HeadIndex], current__SampleIndex - __HeadIndex);
        __HeadIndex = current__SampleIndex;
      }
      else  {
        file.read(&__WavSamples[__HeadIndex], __NumberOfSamples - 1 - __HeadIndex);
        file.read(__WavSamples, current__SampleIndex);
        __HeadIndex = current__SampleIndex;
      }
    }
  }
  tcReset();
  file.close();
  free(__WavSamples);
}


void TC5_Handler (void)
{
  if (__SampleIndex < __NumberOfSamples - 1)
  {
    analogWrite(A0, __WavSamples[__SampleIndex++]);
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
  }
  else
  {
    __SampleIndex = 0;
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
  }

  /*
      analogWrite(A0, myFile.read());
      TC5->COUNT16.INTFLAG.bit.MC0 = 1;
      s--;
  */
}



// Configures the TC to generate output events at the sample frequency.
//Configures the TC in Frequency Generation mode, with an event output once
//each time the audio sample frequency period expireSUsb.
void tcConfigure(int sampleRate)
{
  // Enable GCLK for TCC2 and TC5 (timer counter input clock)
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
  while (GCLK->STATUS.bit.SYNCBUSY);

  tcReset(); //reset TC5

  // Set Timer counter Mode to 16 bits
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  //set prescaler and enable TC5
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
  TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate - 1);
  while (tcIsSyncing());

  // Configure interrupt request
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Enable the TC5 interrupt request
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  while (tcIsSyncing()); //wait until TC5 is done syncing
}

//Function that is used to check if TC5 is done syncing
//returns true when it is done syncing
bool tcIsSyncing()
{
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

//This function enables TC5 and waits for it to be ready
void tcStartCounter()
{
  // Sprintln(TC5->COUNT16.CTRLA.reg);
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (tcIsSyncing()); //wait until snyc'd
}

//Reset TC5
void tcReset()
{
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);
}

//disable TC5
void tcDisable()
{
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (tcIsSyncing());
}

