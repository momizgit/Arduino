// --- USER SETTINGS ---
#define DEBUG_MODE  true  // true = 10s timer, false = 2hr timer
#define STATUS_FADE false  // true = Breathing (PWM), false = Blinking (Digital)

// --- PHYSICAL PIN MAPPING (ATmega8 DIP28) ---
const int physRX      = 2;  // PD0
const int physTX      = 3;  // PD1
const int physRelay   = 24; // PC1 (A1)
const int physButton  = 25; // PC2 (A2)
const int physPWM_LED = 17; // PB3 (D11)
const int physLDR     = 23; // PC0 (A0)

// --- ARDUINO PIN MAPPING ---
const int sensorPin = A0;   
const int relayPin  = A1;   
const int buttonPin = A2;   
const int ledPin    = LED_BUILTIN; //11;   

// --- CONSTANTS ---
const unsigned long twoHours = DEBUG_MODE ? 10000 : 7200000; 
const unsigned long cycleTime = 4000; // 4s total (2s up/2s down OR 2s on/2s off)

// --- VARIABLES ---
bool day = false;
bool lastDayState = false;    
bool lastButtonState = HIGH;
bool timerActive = false;

unsigned long dayStartTime = 0;     
unsigned long lastDebounceTime = 0;
unsigned long lastSerialTime = 0;

void setup() {
  Serial.begin(9600); // UART Physical Pins 2 & 3
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); 
  
  digitalWrite(relayPin, LOW);
  Serial.println(F("--- System Initialized ---"));
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. UPDATE STATUS LED (Handles Fade vs Blink)
  updateStatusLED(currentMillis);

  // 2. LDR LOGIC (Physical Pin 23)
  int sensorValue = analogRead(sensorPin);
  bool currentLdrDay = (sensorValue >= 512); 
  if (currentLdrDay != lastDayState) {
    day = currentLdrDay;
    lastDayState = currentLdrDay;
  }

  // 3. BUTTON TOGGLE (Physical Pin 25)
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) { lastDebounceTime = currentMillis; }
  if ((currentMillis - lastDebounceTime) > 50) { // 50ms Debounce
    if (reading == LOW && lastButtonState == HIGH) {
      day = !day; 
      Serial.print(F("Manual Toggle. Day: ")); Serial.println(day);
    }
  }
  lastButtonState = reading;

  // 4. RELAY CONTROL (Physical Pin 24)
  // Millis overflow handled by (Now - Start >= Interval)
  if (day) {
    if (!timerActive) {
      dayStartTime = currentMillis;
      timerActive = true;
    }
    if (timerActive && (currentMillis - dayStartTime >= twoHours)) {
      digitalWrite(relayPin, HIGH); 
    }
  } else {
    timerActive = false;           
    digitalWrite(relayPin, LOW);    
  }

  // 5. SERIAL MONITOR
  if (currentMillis - lastSerialTime >= 2000) {
    lastSerialTime = currentMillis;
    printStatus(currentMillis);
  }
}

// --- DUAL-MODE LED FUNCTION ---
void updateStatusLED(unsigned long now) {
  unsigned long phase = now % cycleTime;

  if (STATUS_FADE) {
    // --- FADE MODE (PWM) ---
    int brightness;
    if (phase < 2000) {
      brightness = map(phase, 0, 2000, 0, 255); // Fade Up
    } else {
      brightness = map(phase, 2000, 4000, 255, 0); // Fade Down
    }
    analogWrite(ledPin, brightness);
  } 
  else {
    // --- BLINK MODE (Digital) ---
    if (phase < 2000) {
      digitalWrite(ledPin, HIGH); // On for 2s
    } else {
      digitalWrite(ledPin, LOW);  // Off for 2s
    }
  }
}

void printStatus(unsigned long now) {
  Serial.print(F("Day: ")); Serial.print(day);
  Serial.print(F(" | Relay: ")); Serial.print(digitalRead(relayPin));
  if (day && !digitalRead(relayPin)) {
    unsigned long elapsed = (now - dayStartTime) / 1000;
    Serial.print(F(" | Time Elapsed: ")); Serial.print(elapsed); Serial.print(F("s"));
  }
  Serial.println();
}
