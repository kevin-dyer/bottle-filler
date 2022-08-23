#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Button.h>


LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// Rotary Encoder Inputs
#define CLK 2
#define DT 3
#define SW 8

#define CLK2 9
#define DT2 10
#define SW2 11//not right! sensor flow already there

//Relay
#define relay1 7 //Gas 1
#define relay2 6 //Liquid 1
#define relay3 5 //Gas 2
#define relay4 4 //Liquid 2

//Encoder 
int currentStateCLK;
int lastStateCLK;
int currentStateCLK2;
int lastStateCLK2;

//Encoder buttons
Button button(8);
Button button2(11);


//Menu
int menuIndex = 0;
//int nextMenuIndex = 0;
const int menuLength = 5;
String page = "menu";
String menuTitles[menuLength] = {
  "Fill Routine",
  "Liquid Out",
  "Gas Out",
  "Set Gas %",
  "Set Purge mL"
};
unsigned long pressStart;
unsigned long pressStart2;
const int shortPressMs = 800;

//Fill variables
String stage;
int doneTime;
String stage2;
int doneTime2;
const int doneWait = 3000;
int lastDisplayedFlowCount;
int lastDisplayedFlowCount2;

// hall flow sensor
//1,380 pulses per Liter
unsigned char flowsensor = 12; // Sensor Input
unsigned char flowsensor2 = 13; // Sensor Input

int flowCount = 0;
int flowCount2 = 0;

int liquidSetPoint = 750;
int liquidSetPoint2 = 750;
int gasPercent = 100; // percentage
int purgeSetPoint = 20;


int getEncoderVal(int initialVal, int minimum = 0, int maximum = INT16_MAX) {
  int nextVal = initialVal;
  //handle encoder input
  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);
  
  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK) {
      // Encoder is rotating CW so increment
      if (nextVal < maximum) {
        nextVal++;
      }
    } else {
      if (nextVal > minimum) {
        nextVal--;
      }
    }
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  return nextVal;
}

int getEncoder2Val(int initialVal, int minimum = 0, int maximum = INT16_MAX) {
  int nextVal = initialVal;
  //handle encoder input
  // Read the current state of CLK
  currentStateCLK2 = digitalRead(CLK2);
  
  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK2 != lastStateCLK2  && currentStateCLK2 == 1){
    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT2) != currentStateCLK2) {
      // Encoder is rotating CW so increment
      if (nextVal < maximum) {
        nextVal++;
      }
    } else {
      if (nextVal > minimum) {
        nextVal--;
      }
    }
  }

  // Remember last CLK state
  lastStateCLK2 = currentStateCLK2;

  return nextVal;
}



void pressDownListener() {
  if(button.pressed()) {
    
    pressStart = millis();
    Serial.println("button PRESSED! pressStart: " + String(pressStart));
  }
}

void pressDownListener2() {
  if(button2.pressed()) {
    pressStart2 = millis();
    Serial.println("button2 PRESSED! pressStartt2: " + String(pressStart2));
  }
}

void clearButtonTimeout() {
  pressStart = 0;
  pressStart2 = 0;
}

bool wasShortPress() {
  if (button.released() && pressStart) {
    Serial.println("button released");
    int duration = millis() - pressStart;
    clearButtonTimeout();
    return duration < shortPressMs;
  }
  return false;
}

bool wasShortPress2() {
  if (button2.released() && pressStart2) {
    Serial.println("button2 released");
    int duration = millis() - pressStart2;
    clearButtonTimeout();
    return duration < shortPressMs;
  }
  return false;
}

bool wasLongPress() {
  if (pressStart && millis() - pressStart > shortPressMs) {
    Serial.println("button1 long press! millis(), pressStart: " + String(millis()) + ", " + String(pressStart));
    clearButtonTimeout();
    return true;
  } else {
    return false;
  }
}

bool wasLongPress2() {
  if (pressStart2 && millis() - pressStart2 > shortPressMs) {
      Serial.println("button2 long press! millis(), pressStart2: " + String(millis()) + ", " + String(pressStart2));
    clearButtonTimeout();
    return true;
  } else {
    return false;
  }
}


void renderMenu () {
  int nextMenuIndex = getEncoderVal(menuIndex, 0, menuLength - 1);
  int nextMenuIndex2 = getEncoder2Val(menuIndex, 0, menuLength - 1);

  //  handle button input
  if(button.pressed() || button2.pressed()) {
    //enter menu item
    page = menuTitles[menuIndex];

    if (menuIndex == 0) {
      stage = "";
      stage2 = "";
      renderFillerDisplay();
    }

    if (menuIndex == 1) {
      stage = "";
      stage2 = "";
      renderLiquidDispenserDisplay();
    }

    if (menuIndex == 2) {
      stage = "";
      stage2 = "";
      renderGasDispenserDisplay();
    }

    if (menuIndex == 3) {
      renderGasFillDisplay();
    }

    if (menuIndex == 4) {
      renderPurgeDisplay();
    }
  }

  if (nextMenuIndex != menuIndex) {
    menuIndex = nextMenuIndex;
    renderMenuDisplay();
    Serial.println("menu index changed: " + String(menuIndex));
  } else if (nextMenuIndex2 != menuIndex) {
    menuIndex = nextMenuIndex2;
    renderMenuDisplay();
    Serial.println("menu index changed: " + String(menuIndex));
  }
}

void renderMenuDisplay() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Menu:");
  lcd.setCursor(1,1);
  lcd.print(menuTitles[menuIndex]);
}

void renderBottleFiller() {
  //handle encoder input
  int nextLiquidSetPoint = getEncoderVal(liquidSetPoint, 0);
  int nextLiquid2SetPoint = getEncoder2Val(liquidSetPoint2, 0);

  //  handle button input
  pressDownListener();
  pressDownListener2();

  //  Long press - go back
  if (wasLongPress() || wasLongPress2()) {
    //   go back
    page = "menu";
    flowCount = 0;
    flowCount2 = 0;
    stage = "";
    stage2 = "";
    //Close relays
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    digitalWrite(relay3, LOW);
    digitalWrite(relay4, LOW);
    // Serial.println("Long press detected. Going back to menu");
    renderMenuDisplay();
  }

  //  Short press - Start dispensing!
  if (wasShortPress()) {
    Serial.println("Short press detected");
    if (stage == "") {
      flowCount = 0;
      stage = "gas";
      digitalWrite(relay1, HIGH);
      renderFillerDisplay();
    }
  }

  if (wasShortPress2()) {
    Serial.println("Short press #2 detected");
    if (stage2 == "") {
      flowCount2 = 0;
      stage2 = "gas";
      digitalWrite(relay4, HIGH);
      renderFillerDisplay();
    }
  }

  //  Stages
  if (stage == "gas") {
    if (flowCount >= (liquidSetPoint * (gasPercent / 100))) {
      flowCount = 0;
      stage = "liquid";
      digitalWrite(relay1, LOW);
      digitalWrite(relay2, HIGH);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount != flowCount) {
      renderFillerDisplay();
    }
  }

  if (stage == "liquid") {
    if (flowCount >= liquidSetPoint) {
      flowCount = 0;
      stage = "purge";
      digitalWrite(relay2, LOW);
      digitalWrite(relay1, HIGH);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount != flowCount) {
      renderFillerDisplay();
    }
  }

  if (stage == "purge") {
    if (flowCount >= purgeSetPoint) {
      stage = "done";
      flowCount = 0;
      digitalWrite(relay1, LOW);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount != flowCount) {
      renderFillerDisplay();
    }
  }

  if (nextLiquidSetPoint != liquidSetPoint) {
    if (stage == "done" && nextLiquidSetPoint > liquidSetPoint) {
      stage = "liquid";
      digitalWrite(relay2, HIGH);
    }

    // Adjust set point
    liquidSetPoint = nextLiquidSetPoint;
    renderFillerDisplay();
  }


  if (stage == "done") {
    if (!doneTime) {
      doneTime = millis();
    } else if (doneTime && millis() - doneTime > doneWait) {
      doneTime = 0;
      stage = "";
      renderFillerDisplay();
    }
  }

  // 2nd Station Stages:
  if (stage2 == "gas") {
    if (flowCount2 >= (liquidSetPoint2 * (gasPercent / 100))) {
      flowCount2 = 0;
      stage2 = "liquid";
      digitalWrite(relay4, LOW);
      digitalWrite(relay3, HIGH);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount2 != flowCount2) {
      renderFillerDisplay();
    }
  }

  if (stage2 == "liquid") {
    if (flowCount2 >= liquidSetPoint2) {
      flowCount2 = 0;
      stage2 = "purge";
      digitalWrite(relay3, LOW);
      digitalWrite(relay4, HIGH);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount2 != flowCount2) {
      renderFillerDisplay();
    }
  }

  if (stage2 == "purge") {
    if (flowCount2 >= purgeSetPoint) {
      stage2 = "done";
      flowCount2 = 0;
      digitalWrite(relay4, LOW);
      renderFillerDisplay();
    } else if (lastDisplayedFlowCount2 != flowCount2) {
      renderFillerDisplay();
    }
  }

  if (nextLiquid2SetPoint != liquidSetPoint2) {
    if (stage2 == "done" && nextLiquid2SetPoint > liquidSetPoint2) {
      stage2 = "liquid";
      digitalWrite(relay3, HIGH);
    }

    // Adjust set point
    liquidSetPoint2 = nextLiquid2SetPoint;
    renderFillerDisplay();
  }


  if (stage2 == "done") {
    if (!doneTime2) {
      doneTime2 = millis();
    } else if (doneTime2 && millis() - doneTime2 > doneWait) {
      doneTime2 = 0;
      stage2 = "";
      renderFillerDisplay();
    }
  }
}

void renderFillerDisplay() {
  Serial.println("renderFillerDisplay called!");
  lcd.clear();

  // TODO: update to consider all render options
  String title = "Fill Routine";
  String text = "";
  lcd.setCursor(1,0);
  lcd.print(title);
  
  lcd.setCursor(1,1);
  if (stage == "") {
    text = String(liquidSetPoint) + "mL";
  } else if (stage == "gas") {
    text = "Gas " + String(flowCount);
  } else if (stage == "liquid") {
    text = "Liq " + String(flowCount);
  } else if (stage == "purge") {
    text = "Purge " + String(flowCount);
  } else if (stage == "done") {
    text = "Done";
  }
  lcd.print(text);

  lcd.setCursor(8, 1);
  if (stage2 == "") {
    lcd.print(String(liquidSetPoint2) + "mL");
  } else if (stage2 == "gas") {
    lcd.print("Gas " + String(flowCount2));
  } else if (stage2 == "liquid") {
    lcd.print("Liq " + String(flowCount2));
  } else if (stage2 == "purge" + String(flowCount2)) {
    lcd.print("Purge");
  } else if (stage2 == "done") {
    lcd.print("Done");
  }

  lastDisplayedFlowCount = flowCount;
  lastDisplayedFlowCount2 = flowCount2;
}


void renderLiquidDispenser() {
  //  handle button input
  pressDownListener();
  pressDownListener2();

  //  Long press - go back
  if (wasLongPress() || wasLongPress2()) {
    //   go back
    page = "menu";
    flowCount = 0;
    flowCount2 = 0;
    stage = "";
    stage2 = "";
    //Close relays
    digitalWrite(relay2, LOW);
    digitalWrite(relay3, LOW);
    Serial.println("Long press detected. Going back to menu");
    renderMenuDisplay();
  }

  //  Short press - Start dispensing!
  if (wasShortPress()) {
    Serial.println("Short press detected");
    if (stage == "") {
      flowCount = 0;
      stage = "dispensing-liquid";
      digitalWrite(relay2, HIGH);
    } else if (stage == "dispensing-liquid") {
      flowCount = 0;
      stage = "";
      digitalWrite(relay2, LOW);
    }
    renderLiquidDispenserDisplay();
  }

  if (wasShortPress2()) {
    if (stage2 == "") {
      flowCount2 = 0;
      stage2 = "dispensing-liquid";
      digitalWrite(relay3, HIGH);
    } else if (stage2 == "dispensing-liquid") {
      flowCount2 = 0;
      stage2 = "";
      digitalWrite(relay3, LOW);
    }
    renderLiquidDispenserDisplay();
  }

  if (lastDisplayedFlowCount != flowCount || lastDisplayedFlowCount2 != flowCount2) {
    renderLiquidDispenserDisplay()
  }
}

void renderLiquidDispenserDisplay() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Liquid Out");

  lcd.setCursor(1,1);
  if (stage == "") {
    lcd.print("Closed");
  } else if (stage == "dispensing-liquid") {
    lcd.print(String(flowCount));
  }

  lcd.setCursor(8, 1);
  if (stage2 == "") {
    lcd.print("Closed");
  } else if (stage2 == "dispensing-liquid") {
    lcd.print(String(flowCount2));
  }

  lastDisplayedFlowCount = flowCount;
  lastDisplayedFlowCount2 = flowCount2;
}


void renderGasDispenser() {
//  handle button input
  pressDownListener();
  pressDownListener2();

  //  Long press - go back
  if (wasLongPress() || wasLongPress2()) {
    //   go back
    page = "menu";
    flowCount = 0;
    flowCount2 = 0;
    stage = "";
    stage2 = "";
    //Close relays
    digitalWrite(relay1, LOW);
    digitalWrite(relay4, LOW);
    Serial.println("Long press detected. Going back to menu");
    renderMenuDisplay();
  }

  //  Short press - Start dispensing!
  if (wasShortPress()) {
    Serial.println("Short press detected");
    if (stage == "") {
      flowCount = 0;
      stage = "dispensing-gas";
      digitalWrite(relay1, HIGH);
    } else if (stage == "dispensing-gas") {
      flowCount = 0;
      stage = "";
      digitalWrite(relay1, LOW);
    }
    renderGasDispenserDisplay();
  }

  if (wasShortPress2()) {
    if (stage2 == "") {
      flowCount2 = 0;
      stage2 = "dispensing-gas";
      digitalWrite(relay4, HIGH);
    } else if (stage2 == "dispensing-gas") {
      flowCount2 = 0;
      stage2 = "";
      digitalWrite(relay4, LOW);
    }
    renderGasDispenserDisplay();
  }

  if (lastDisplayedFlowCount != flowCount || lastDisplayedFlowCount2 != flowCount2) {
    renderGasDispenserDisplay()
  }
}

void renderGasDispenserDisplay() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Gas Out");

  lcd.setCursor(1,1);
  if (stage == "") {
    lcd.print("Closed");
  } else if (stage == "dispensing-gas") {
    lcd.print(String(flowCount));
  }

  lcd.setCursor(8, 1);
  if (stage2 == "") {
    lcd.print("Closed");
  } else if (stage2 == "dispensing-gas") {
    lcd.print(String(flowCount2));
  }

  lastDisplayedFlowCount = flowCount;
  lastDisplayedFlowCount2 = flowCount2;
}

void renderGasFill() {
  int nextGasPercent = getEncoderVal(gasPercent, 0);

  if (nextGasPercent != gasPercent) {
    //    Set point has been adjusted
    gasPercent = nextGasPercent;

    //NOTE: this should only display temporarily while bottle is actually being filled
    renderGasFillDisplay();
  }

  //  handle button input
  pressDownListener();

  //  Long press - go back
  if (wasLongPress()) {
    //   go back
    page = "menu";    
    Serial.println("Long press detected. Going back to menu");
    renderMenuDisplay();
  }

  //  Short press - Start dispensing!
  if (wasShortPress()) {
    Serial.println("Short press detected. Doing nothing");
  }
}

void renderGasFillDisplay() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Gas Fill (%):");
  lcd.setCursor(1,1);
  lcd.print(gasPercent);
}

void renderPurge() {
  int nextPurgeSetPoint = getEncoderVal(purgeSetPoint, 0);

  if (nextPurgeSetPoint != purgeSetPoint) {
    //    Set point has been adjusted
    purgeSetPoint = nextPurgeSetPoint;

    //NOTE: this should only display temporarily while bottle is actually being filled
    renderPurgeDisplay();
  }

  //  handle button input
  pressDownListener();
  //  Long press - go back
  if (wasLongPress()) {
    //   go back
    page = "menu";    
    //Close relays
    digitalWrite(relay1, LOW);
    Serial.println("Long press detected. Going back to menu");
    renderMenuDisplay();
  }

  //  Short press - Start dispensing!
  if (wasShortPress()) {
    Serial.println("Short press detected. Doing nothing");
  }
}

void renderPurgeDisplay() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Purge Vol (mL):");
  lcd.setCursor(1,1);
  lcd.print(purgeSetPoint);
}



/*
Interrupt Service Routine
 */
void flow () // Interrupt function
{
   flowCount++;
}

/*
Interrupt Service Routine
 */
void flow2 () // Interrupt function
{
   flowCount2++;
}


void setup() {
  //  Relay
  pinMode(relay4, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay1, OUTPUT);

  // Set encoder pins as inputs
  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);
  pinMode(SW, INPUT_PULLUP);

  //  Flow meter
  pinMode(flowsensor, INPUT);
  digitalWrite(flowsensor, HIGH);
  attachInterrupt(digitalPinToInterrupt(flowsensor), flow, RISING); // Setup Interrupt
  flowCount = 0;

   //  Flow meter 2
  pinMode(flowsensor2, INPUT);
  digitalWrite(flowsensor2, HIGH);
  attachInterrupt(digitalPinToInterrupt(flowsensor2), flow2, RISING); // Setup Interrupt
  flowCount2 = 0;

  // initialize the lcd
  lcd.init(); 
  // Print a message to the LCD.
  lcd.backlight();
  
  // Setup Serial Monitor
  Serial.begin(9600);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);

  renderMenuDisplay();
}

void loop() {
  if (page == "menu") {
    renderMenu();
  } else if (page == menuTitles[0]) {
    renderBottleFiller();
  } else if (page == menuTitles[1]) {
    renderLiquidDispenser();
  } else if (page == menuTitles[2]) {
    renderGasDispenser();
  } else if (page == menuTitles[3]) {
    renderGasFill();
  } else if (page == menuTitles[4]) {
    renderPurge();
  }
}
