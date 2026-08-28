#include <EEPROM.h>

// --- HARDWARE PINS ---
const int kickOut = 2;
const int snareOut = 3;
const int hihatOut = 4;
const int closedHatGate = 5;

const int btnPlay = 7;
const int btnShift = 8;
const int btnLeft = 13; // Requires external 1k-2.2k pull-up to 5V
const int btnRight = 12;
const int potTempo = A4;
const int syncOut = A5; // Dedicated Sync Out Jack

const int btnDrums[4] = {A3, A2, A1, A0}; // Kick, Snare, CHH, OHH
const int ledDrums[4] = {9, 6, 10, 11};   // PWM capable

// --- LED SETTINGS ---
const int LED_BRIGHT = 255;
const int LED_DIM = 15;

// --- STATE VARIABLES ---
bool isLiveMode = true;
bool isPlaying = false;
int currentVoice = 0; // 0=Kick, 1=Snare, 2=CHH, 3=OHH
int currentPage = 0;  // 0 to 3 (Pages 1-4)
byte seqLength = 16;  
int currentStep = 0;

// Sequence Data [voice][step]
bool pattern[4][16] = {false};
bool muted[4] = {false, false, false, false};

// Timing & Swing
unsigned long lastStepTime = 0;
unsigned long stepInterval = 125; 
int swingAmount = 0; 
int savedPotVal = 0;

// Soft-Takeover (Catch-up) Variables
bool tempoLocked = false;
bool swingLocked = false;
bool lengthLocked = false;
bool lastShiftState = false;
bool lastLengthEditState = false;
bool wasShiftHeldForPot = false;

// Triggers (Hardware vs Visuals)
const int TRIG_LEN = 15;
unsigned long triggerStartTime[4] = {0, 0, 0, 0};
unsigned long visualTriggerTime[4] = {0, 0, 0, 0}; 
bool triggerActive[4] = {false, false, false, false};
bool gateClosed = true;

// Sync Trigger Variables
unsigned long syncTriggerStartTime = 0;
bool syncTriggerActive = false;

// Button Debouncing & State
bool lastDrums[4] = {false, false, false, false};
bool lastPlay = false;
bool lastLeft = false;
bool lastRight = false;
bool allPressedHandled = false;

unsigned long lastDrumTime[4] = {0, 0, 0, 0}; 
unsigned long lastNavTime = 0;
const int DEBOUNCE_DELAY = 30; 

// UI Animations
unsigned long voiceShowTimer = 0;
bool showingVoice = false;
unsigned long pageShowTimer = 0;
bool showingPage = false;
unsigned long modeShowTimer = 0;
bool showingMode = false;
bool modeScrollDir = true; 

// --- MIDI VARIABLES ---
byte midiState = 0;
byte midiCommand = 0;
byte midiNote = 0;
byte midiVelocity = 0;
byte midiClockCounter = 0;
bool useExternalClock = false;
unsigned long lastMidiClockTime = 0;

void setup() {
  pinMode(kickOut, OUTPUT);
  pinMode(snareOut, OUTPUT);
  pinMode(hihatOut, OUTPUT);
  pinMode(closedHatGate, OUTPUT);
  
  pinMode(syncOut, OUTPUT);
  digitalWrite(syncOut, HIGH); 

  pinMode(btnPlay, INPUT_PULLUP);
  pinMode(btnShift, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);

  for (int i = 0; i < 4; i++) {
    pinMode(btnDrums[i], INPUT_PULLUP);
    pinMode(ledDrums[i], OUTPUT);
  }

  digitalWrite(closedHatGate, HIGH); 
  
  Serial.begin(31250); // Initialize MIDI hardware UART

  startupAnimation();

  byte savedLen = EEPROM.read(0);
  if (savedLen >= 1 && savedLen <= 16) seqLength = savedLen;

  int addr = 1;
  for (int v = 0; v < 4; v++) {
    for (int s = 0; s < 16; s++) {
      byte val = EEPROM.read(addr++);
      pattern[v][s] = (val == 1);
    }
  }
  savedPotVal = analogRead(potTempo);
}

void loop() {
  unsigned long currentMillis = millis();
  bool shiftHeld = (digitalRead(btnShift) == LOW);

  handleMIDI();
  handleSequencerTick(currentMillis);
  handleTriggers(currentMillis);
  handleButtons(shiftHeld, currentMillis);
  handleTempoAndSwing(shiftHeld);
  updateLEDs(currentMillis, shiftHeld);
}

// --- HIGH-SPEED MIDI HANDLER (PURE SOUND MODULE MODE) ---
void handleMIDI() {
  while (Serial.available() > 0) {
    byte incomingByte = Serial.read();

    // 1. RUTHLESS SYSTEM FILTER
    // 0xF8 to 0xFF are System Real-Time messages (Clock, Start, Stop, Active Sensing).
    // By instantly calling 'continue', we throw ALL of them in the trash!
    if (incomingByte >= 0xF8) {
      continue; 
    }

    // 2. CHANNEL FILTER
    // We ONLY care about Note On for Channel 10 (0x99). 
    if (incomingByte >= 0x80) {
      if (incomingByte == 0x99) {
        midiCommand = incomingByte;
        midiState = 1; // Open the gate: we are ready to read the note!
      } else {
        midiState = 0; // Close the gate! Ignore this status and all its data bytes.
      }
    } 
    // 3. PROCESS THE DATA BYTES (Only if the gate is open for Ch 10)
    else if (midiState == 1) {
      midiNote = incomingByte;
      midiState = 2;
    } 
    else if (midiState == 2) {
      midiVelocity = incomingByte;
      midiState = 1; // Reset to 1 for "Running Status" (rapid-fire notes)

      // Fire the analog triggers!
      if (midiVelocity > 0) {
        if (midiNote == 36) fireTrigger(0, true);      // Kick (C2)
        else if (midiNote == 38) fireTrigger(1, true); // Snare (D2)
        else if (midiNote == 44) fireTrigger(2, true); // CHH (G#2)
        else if (midiNote == 46) fireTrigger(3, true); // OHH (A#2)
      }
    }
  }
}

// --- CORE SEQUENCER ---
void handleSequencerTick(unsigned long currentMillis) {
  // Auto-fallback to internal clock if MIDI is unplugged for 500ms
  if (useExternalClock && (currentMillis - lastMidiClockTime > 500)) {
    useExternalClock = false;
  }

  // If external MIDI is driving us, ignore the internal timer!
  if (useExternalClock || !isPlaying) return;

  unsigned long currentInterval = stepInterval;
  if (currentStep % 2 != 0) currentInterval += (stepInterval * swingAmount) / 100;
  else currentInterval -= (stepInterval * swingAmount) / 100;

  if (currentMillis - lastStepTime >= currentInterval) {
    lastStepTime += currentInterval; 
    playCurrentStepAndAdvance();
  }
}

void playCurrentStepAndAdvance() {
  digitalWrite(syncOut, LOW); 
  syncTriggerActive = true;
  syncTriggerStartTime = millis();
  
  for (int i = 0; i < 4; i++) {
    if (pattern[i][currentStep]) {
      if (i == 3 && pattern[2][currentStep]) continue; 
      fireTrigger(i, false); 
    }
  }
  
  currentStep++;
  if (currentStep >= seqLength) currentStep = 0;
}

// --- HARDWARE TRIGGERS ---
void fireTrigger(int voice, bool isManual) {
  if (!isManual && muted[voice]) return; 

  if (voice == 0) digitalWrite(kickOut, HIGH);
  else if (voice == 1) digitalWrite(snareOut, HIGH);
  else if (voice == 2) {
    gateClosed = true;
    digitalWrite(closedHatGate, HIGH);
    digitalWrite(hihatOut, HIGH);
  }
  else if (voice == 3) {
    gateClosed = false;
    digitalWrite(closedHatGate, LOW);
    digitalWrite(hihatOut, HIGH);
  }
  
  triggerActive[voice] = true;
  triggerStartTime[voice] = millis();
  
  if (!isManual) {
    visualTriggerTime[voice] = millis(); 
  }
}

void handleTriggers(unsigned long currentMillis) {
  for (int i = 0; i < 4; i++) {
    if (triggerActive[i] && (currentMillis - triggerStartTime[i] >= TRIG_LEN)) {
      triggerActive[i] = false;
      if (i == 0) digitalWrite(kickOut, LOW);
      else if (i == 1) digitalWrite(snareOut, LOW);
      else if (i == 2 || i == 3) digitalWrite(hihatOut, LOW);
    }
  }
  digitalWrite(closedHatGate, gateClosed ? HIGH : LOW);

  if (syncTriggerActive && (currentMillis - syncTriggerStartTime >= TRIG_LEN)) {
    syncTriggerActive = false;
    digitalWrite(syncOut, HIGH); 
  }
}

// --- EEPROM SAVE HELPERS ---
void saveStepToEEPROM(int voice, int step, bool state) {
  int addr = 1 + (voice * 16) + step;
  EEPROM.update(addr, state ? 1 : 0);
}

void saveFullPatternToEEPROM() {
  int addr = 1;
  for (int v = 0; v < 4; v++) {
    for (int s = 0; s < 16; s++) {
      EEPROM.update(addr++, pattern[v][s] ? 1 : 0);
    }
  }
}

// --- BUTTON LOGIC ---
void handleButtons(bool shiftHeld, unsigned long currentMillis) {
  bool playPressed = (digitalRead(btnPlay) == LOW);
  bool leftPressed = (digitalRead(btnLeft) == LOW);
  bool rightPressed = (digitalRead(btnRight) == LOW);

  if (playPressed && !lastPlay && (currentMillis - lastNavTime > DEBOUNCE_DELAY)) {
    if (shiftHeld) {
      isLiveMode = !isLiveMode;
      showingMode = true;
      modeShowTimer = currentMillis;
      modeScrollDir = isLiveMode; 
    } else {
      isPlaying = !isPlaying;
      if (isPlaying) { 
        currentStep = 0; 
        lastStepTime = currentMillis; 
      }
    }
    lastNavTime = currentMillis;
  }
  lastPlay = playPressed;

  if (!isLiveMode) {
    if (leftPressed && !lastLeft && (currentMillis - lastNavTime > DEBOUNCE_DELAY)) {
      if (shiftHeld) {
        currentPage = (currentPage - 1 + 4) % 4;
        showPageSelect(currentMillis);
      } else {
        currentVoice = (currentVoice - 1 + 4) % 4;
        showVoiceSelect(currentMillis); 
      }
      lastNavTime = currentMillis;
    }
    
    if (rightPressed && !lastRight && (currentMillis - lastNavTime > DEBOUNCE_DELAY)) {
      if (shiftHeld) {
        currentPage = (currentPage + 1) % 4;
        showPageSelect(currentMillis);
      } else {
        currentVoice = (currentVoice + 1) % 4;
        showVoiceSelect(currentMillis); 
      }
      lastNavTime = currentMillis;
    }
  }
  lastLeft = leftPressed;
  lastRight = rightPressed;

  bool allPressed = true;
  for (int i = 0; i < 4; i++) {
    bool drumPressed = (digitalRead(btnDrums[i]) == LOW);
    if (!drumPressed) allPressed = false;

    if (drumPressed && !lastDrums[i] && (currentMillis - lastDrumTime[i] > DEBOUNCE_DELAY)) {
      if (isLiveMode) {
        if (shiftHeld) {
          muted[i] = !muted[i]; 
        } else {
          fireTrigger(i, true); 
        }
      } else {
        if (shiftHeld) {
          muted[i] = !muted[i]; 
        } else {
          int stepIndex = (currentPage * 4) + i;
          pattern[currentVoice][stepIndex] = !pattern[currentVoice][stepIndex];
          saveStepToEEPROM(currentVoice, stepIndex, pattern[currentVoice][stepIndex]);
        }
      }
      lastDrumTime[i] = currentMillis;
    }
    lastDrums[i] = drumPressed;
  }

  // GLOBAL ACTIONS (Shift + All 4 Buttons)
  if (shiftHeld && allPressed && !allPressedHandled) {
    if (!isLiveMode) {
      clearPatternAnimation();
      for (int v = 0; v < 4; v++) {
        for (int s = 0; s < 16; s++) pattern[v][s] = false;
        muted[v] = false; 
      }
      saveFullPatternToEEPROM();
      
      // Reset de UI view naar Kick en Pagina 1
      currentVoice = 0; 
      currentPage = 0;  
    } 
    allPressedHandled = true;
  } else if (!allPressed) {
    allPressedHandled = false;
  }
}

// --- TEMPO, SWING & LENGTH (PRO SOFT-TAKEOVER) ---
void handleTempoAndSwing(bool shiftHeld) {
  int potVal = analogRead(potTempo);
  bool leftHeld = (digitalRead(btnLeft) == LOW);
  bool rightHeld = (digitalRead(btnRight) == LOW);
  
  bool lengthEditActive = (!isLiveMode && shiftHeld && leftHeld && rightHeld);

  if (shiftHeld && !lastShiftState) swingLocked = true;
  if (!shiftHeld && lastShiftState) tempoLocked = true;
  lastShiftState = shiftHeld;

  if (lengthEditActive && !lastLengthEditState) lengthLocked = true;
  lastLengthEditState = lengthEditActive;

  if (lengthEditActive) {
    if (lengthLocked) {
      int potLen = map(potVal, 0, 1023, 16, 1);
      if (abs(potLen - seqLength) <= 1) lengthLocked = false;
    }

    if (!lengthLocked) {
      int newLen = map(potVal, 0, 1023, 16, 1);
      if (newLen != seqLength) {
        seqLength = newLen;
        EEPROM.update(0, seqLength);
      }
    }
    
    swingLocked = true; 
    wasShiftHeldForPot = true;
    return;
  }

  if (shiftHeld) {
    if (swingLocked) {
      int potSwing = map(potVal, 0, 1023, 50, 0);
      if (abs(potSwing - swingAmount) <= 2) swingLocked = false;
    }
    
    if (!swingLocked) {
      swingAmount = map(potVal, 0, 1023, 50, 0);
    }
    wasShiftHeldForPot = true;
  } else {
    if (tempoLocked) {
      int potBpm = map(potVal, 0, 1023, 160, 40);
      int currentBpm = 15000 / stepInterval;
      if (abs(potBpm - currentBpm) <= 3) tempoLocked = false;
    }
    
    if (!tempoLocked) {
      int bpm = map(potVal, 0, 1023, 160, 40);
      stepInterval = 15000 / bpm; 
    }
  }
}

// --- UI ANIMATIONS & LEDS ---
void fadeTransition(int ledOut1, int ledIn1, int ledOut2 = -1, int ledIn2 = -1) {
  for (int i = 0; i <= 255; i += 5) { 
    if (ledOut1 >= 0) analogWrite(ledDrums[ledOut1], 255 - i);
    if (ledIn1 >= 0) analogWrite(ledDrums[ledIn1], i);
    if (ledOut2 >= 0) analogWrite(ledDrums[ledOut2], 255 - i);
    if (ledIn2 >= 0) analogWrite(ledDrums[ledIn2], i);
    delay(3); 
  }
}

void startupAnimation() {
  for (int i = 0; i < 4; i++) analogWrite(ledDrums[i], 0);
  fadeTransition(-1, 0); 
  fadeTransition(0, 1);  
  fadeTransition(1, 2);  
  fadeTransition(2, 3);  
  fadeTransition(-1, 0);       
  fadeTransition(0, 1, 3, 2);  
  fadeTransition(1, 0, 2, 3);  
  fadeTransition(3, -1);       
  fadeTransition(0, 1);  
  fadeTransition(1, 2);  
  fadeTransition(2, 3);  
  fadeTransition(3, -1); 
}

void showVoiceSelect(unsigned long currentMillis) {
  showingVoice = true;
  voiceShowTimer = currentMillis;
}

void showPageSelect(unsigned long currentMillis) {
  showingPage = true;
  pageShowTimer = currentMillis;
}

void clearPatternAnimation() {
  for (int blink = 0; blink < 3; blink++) {
    for (int i = 0; i < 4; i++) analogWrite(ledDrums[i], LED_BRIGHT);
    delay(100);
    for (int i = 0; i < 4; i++) analogWrite(ledDrums[i], 0);
    delay(100);
  }
}

void updateLEDs(unsigned long currentMillis, bool shiftHeld) {
  if (showingMode) {
    unsigned long elapsed = currentMillis - modeShowTimer;
    if (elapsed < 240) { 
      int ledIndex = elapsed / 60; 
      if (modeScrollDir) ledIndex = 3 - ledIndex; 
      
      for (int i = 0; i < 4; i++) {
        if (i == ledIndex) analogWrite(ledDrums[i], LED_BRIGHT);
        else analogWrite(ledDrums[i], 0);
      }
      return; 
    } else {
      showingMode = false;
    }
  }

  bool leftHeld = (digitalRead(btnLeft) == LOW);
  bool rightHeld = (digitalRead(btnRight) == LOW);
  if (!isLiveMode && shiftHeld && leftHeld && rightHeld) {
    int lenPage = (seqLength - 1) / 4;
    int lenStep = (seqLength - 1) % 4;
    for (int i = 0; i < 4; i++) {
      if (i == lenStep) analogWrite(ledDrums[i], LED_BRIGHT);
      else if (i == lenPage) analogWrite(ledDrums[i], LED_DIM);
      else analogWrite(ledDrums[i], 0);
    }
    return;
  }

  if (showingVoice) {
    if (currentMillis - voiceShowTimer < 300) {
      bool flashState = ((currentMillis - voiceShowTimer) % 100) < 50; 
      for (int i = 0; i < 4; i++) {
        if (i == currentVoice && flashState) analogWrite(ledDrums[i], LED_BRIGHT);
        else analogWrite(ledDrums[i], 0);
      }
      return;
    } else {
      showingVoice = false;
    }
  }

  if (showingPage) {
    if (currentMillis - pageShowTimer < 400) {
      for (int i = 0; i < 4; i++) {
        if (i == currentPage) analogWrite(ledDrums[i], LED_BRIGHT);
        else analogWrite(ledDrums[i], 0);
      }
      return;
    } else {
      showingPage = false;
    }
  }

  if (isLiveMode) {
    for (int i = 0; i < 4; i++) {
      if (digitalRead(btnDrums[i]) == LOW) {
        analogWrite(ledDrums[i], LED_BRIGHT);
      } else if (currentMillis - visualTriggerTime[i] < 50) {
        analogWrite(ledDrums[i], LED_DIM);
      } else {
        analogWrite(ledDrums[i], 0);
      }
    }
  } else {
    int playheadPage = currentStep / 4;
    int playheadLedIndex = currentStep % 4;

    for (int i = 0; i < 4; i++) {
      int absoluteStep = (currentPage * 4) + i;
      
      if (isPlaying && (currentPage == playheadPage) && (i == playheadLedIndex)) {
        analogWrite(ledDrums[i], LED_BRIGHT);
      } else if (pattern[currentVoice][absoluteStep]) {
        analogWrite(ledDrums[i], LED_DIM);
      } else {
        analogWrite(ledDrums[i], 0);
      }
    }
  }
}