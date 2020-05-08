
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include<avr/wdt.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//const int  Inputs
const int x0Limit = 19;
const int x1Limit = 18;
const int z0Limit = 2;
const int z1Limit = 3;
const int glassSensor = 13;
const int fixation0Sensor = 23;
const int fixation1Sensor = 25;
const int guide0Sensor = 27;
const int guide1Sensor = 29;
const int startPedal = 12;
const int gunPressureSensor = 36;
const int gunSensor = 34;

//control Panel
const int mode1Switch = 17;
const int  okButon =  16;
const int nextButton = 15;
const int  backButton = 14;
const int inputs[16] = {x0Limit, x1Limit, z0Limit, z1Limit, glassSensor, fixation0Sensor, fixation1Sensor, guide0Sensor, guide1Sensor, startPedal, gunPressureSensor, gunSensor, mode1Switch, okButon, nextButton, backButton};
const int readyInterface[2] = {startPedal, okButon};
const int settingsInterface[3] = {okButon, nextButton, backButton};
//const int  Outputs
const int guide = 52;
const int fixation = 53;
const int gunPressure = 51;
const int gun = 49;
const int xAxisDir = 54;
const int xAxisStep = 55;
const int zAxisDir = 56;
const int zAxisStep = 57;
const int errorLight = 47;
const int siren = 45;
const int xMotorEnable = 43;
const int zMotorEnable = 41;

const int outputs[12] = {guide, fixation, gunPressure, gun, xAxisDir, xAxisStep, zAxisDir, zAxisStep, errorLight, siren,xMotorEnable,zMotorEnable};

//parameters

const int maxStepsX = 10000;
const int maxStepsZ = 10000;
const int activeStepDelay = 2; //microseconds
const int zAxisStepMicrodelay = 200; //microseconds
const int xAxisStepMicrodelay = 200; //microseconds
const int zStepsPerMm=10;
const int xStepsPerMm=10;
const int glassSensorStartOffset = 50; //mm
const int glassSensorStopOffset = 50; //mm

const int pneumaticOutputs[4] = {guide, fixation, gunPressure, gun};
const int zeroPneumaticArray[4] = {0, 0, 0, 0};
const int zeroSensorsArray[10] = {fixation0Sensor, fixation1Sensor, guide0Sensor, guide1Sensor, gunPressureSensor, gunSensor,x0Limit, x1Limit, z1Limit, startPedal,};
const String zeroSensorsNameArray[10] = {"Fixation 0", "Fixation 1", "Guide 0", "Guide 1", "Gun Pressure", "Gun","Limit X0", "Limit X1", "Limit Z1","Start Pedal" };
const int zeroSateArray[10] =   {0, 1, 1, 0, 1, 1, 0, 1, 1, 1};
const int conflictsSensorsArray[2][2] = { {fixation0Sensor, fixation1Sensor},  {guide0Sensor, guide1Sensor},};
const String conflictsSensorsNameArray[2] = {"Fixation", "Guide"};
const int conflictsLimitSensorsArray[2][2] = {  {x0Limit, x1Limit},  {z0Limit, z1Limit},};
const String conflictsLimitSensorsNameArray[2] = {"Limit X", "Limit Z"};
const long shortPress = 10;
const long longPress = 800;
const int outputDelay = 1000;
const int outputDelayGun = 1000;
const int glassThicknessEepromaddress = 0;
const int pumpSpeedEepromaddress = 1;
const int spacerEepromaddress = 2;
const int scrapeDistance = 100; //mm


const String menuItems[5] = {"   Spacer   "," Pump Speed ", "Glass Thick.", "    Exit    ", "    Save    "};

const String subMenu[3][5] = {
  {"     6mm    ", "     8mm    ", "     10mm    ", "     12mm    ", "    Back    "}, //Spacer
  {"     1      ", "     2      ", "     3      ", "     4      ", "    Back    "}, //pumpSpeed
  {"     5mm    ", "     6mm    ", "     8mm    ", "    10mm    ", "    Back    "}, //GlassThickness
  
};

const int subMenuValues[3][5] = {
  {6, 8, 10, 12, 999}, //spacer
  {1, 2, 3, 4, 999}, //pumpSpeed
  {5, 6, 8, 10, 999}, //GlassThickness
};

int glassThickness, pumpSpeed,spacer, stepCount;
float coeficient = 1.255;
int zOffset=20;//mm


//temporary variables
int subMenuElement, menuElement;
int keyState = 0;
long previousTime = 0;
int pressedKey, elementNumber, tempSpacer, tempPumpSpeed, tempGlassThickness, save, startedGun,temporaryInt,firstStart=1;
String temporaryString;


void setup() {

  Serial.begin(9600);
  lcd.begin();
  lcd.backlight();
  declareInputsAndOutputs();
  activate(errorLight);
  bootingMessage(2000);
  sirenOn(100);
  delay(100);
  disable(errorLight);
  delay(1000);

}

void loop() {
  while (1) {
    //if conflicts
    temporaryInt= noConflicts(conflictsSensorsArray,sizeof(conflictsSensorsArray)/sizeof(conflictsSensorsArray[0]), conflictsSensorsNameArray,0);
    if(temporaryInt!= 0){
        temporaryString = "Err ";
      temporaryString += conflictsSensorsNameArray[temporaryInt-1];
      errorIs(temporaryString);
    } 

    //if Zero position
    read(glassSensor)?next():errorIs("Glass Detected!");
      
    if (allOk(sizeof(zeroSensorsArray)/sizeof(zeroSensorsArray[0]),zeroSensorsArray, zeroSateArray, zeroSensorsNameArray, true)) {
        glassThickness = EepromdataRead(glassThicknessEepromaddress);
        pumpSpeed = EepromdataRead(pumpSpeedEepromaddress);
        spacer = EepromdataRead(spacerEepromaddress);
      //Zero position  ok
      //if first start put Z to default position
        if( firstStart== 1 ){
          
          show("Zero Z Axis?");
          waitForKeyPress(readyInterface,sizeof(readyInterface)/sizeof(readyInterface[0]));
          activate(zMotorEnable);
          delay(500);
          move(zAxisDir, zAxisStep, 0, maxStepsZ, zAxisStepMicrodelay, z0Limit, z0Limit, maxStepsZ, 1);
          Serial.print("Z Axis to glassThickness level");
          move(zAxisDir, zAxisStep, 1, (glassThickness+zOffset) * coeficient * zStepsPerMm, zAxisStepMicrodelay, z1Limit, z1Limit, maxStepsZ, 0);
          Serial.print("Disable Z Motor");
          disable(zMotorEnable);
          firstStart=0;
      }
     
      temporaryString = "Ready! V";
      temporaryString += pumpSpeed;
      temporaryString += " b:";
      temporaryString += spacer;
      temporaryString += "mm";
      show(temporaryString);
      pressedKey = waitForKeyPress(readyInterface,sizeof(readyInterface)/sizeof(readyInterface[0]));
      pressedKey == 1 ? settings() : startProtocol();
    } else {
      //Not at Zro position
      show("Start Zero Prog?");
      pressedKey = waitForKeyPress(readyInterface,sizeof(readyInterface)/sizeof(readyInterface[0]));
      zeroProtocol();
      show("Zero Done!");
      delay(3000);
    }
  }
}

void declareInputsAndOutputs() {
  for (int i = 0; i < sizeof(inputs)/sizeof(inputs[0]); i++) {
    Serial.print(inputs[i]);
    Serial.print(" Input ");
    Serial.println(i);
    pinMode(inputs[i], INPUT_PULLUP);
  }
  for (int i = 0; i < sizeof(outputs)/sizeof(outputs[0]); i++)
  {
    Serial.print(outputs[i]);
    Serial.print(" OUTPUT ");
    Serial.println(i);

    pinMode(outputs[i], OUTPUT);
    digitalWrite(outputs[i], LOW);
  }
}



void bootingMessage(int duration) {
  lcd.clear();
  lcd.print("  Glass Numide  ");
  //lcd.setCursor(0,1);
  show("            T-II");
  delay(duration);
  lcd.clear();
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("         By:AZ27");
  delay(duration);
  lcd.clear();
  lcd.print("      T-II      ");
}


void settings() {
  save = 0;
  menuElement = 0;
  tempPumpSpeed = pumpSpeed;
  tempGlassThickness = glassThickness;
  while (save == 0) {

    temporaryString = "<|";
    temporaryString += menuItems[menuElement];
    temporaryString += "|>";
    show(temporaryString);
    pressedKey = waitForKeyPress(settingsInterface,sizeof(settingsInterface)/sizeof(settingsInterface[0]));
    if (pressedKey == 1) { //back
      menuElement == 0 ? menuElement = (sizeof(menuItems)/sizeof(menuItems[0])) - 1 : menuElement--;
    } else if (pressedKey == 2) { //next
      menuElement++;
       if(menuElement == (sizeof(menuItems) / sizeof(menuItems[0]))){
          menuElement = 0;
       }
    } else if (pressedKey == 0 && menuElement < sizeof(menuItems) / sizeof(menuItems[0])-2) { //ok

      pressedKey = 3; //nothing
      subMenuElement = 0;
      while (pressedKey != 0) {
        
        temporaryString = "<|";
        temporaryString += subMenu[menuElement][subMenuElement];
        temporaryString += "|>";
        show(temporaryString);

        pressedKey = waitForKeyPress(settingsInterface,sizeof(settingsInterface)/sizeof(settingsInterface[0]));

        if (pressedKey == 1) { //back
          subMenuElement == 0 ? subMenuElement = (sizeof(subMenu[menuElement])/sizeof(subMenu[menuElement][0])) - 1 : subMenuElement--;
        } else if (pressedKey == 2) { //next
          subMenuElement == (sizeof(subMenu[menuElement])/sizeof(subMenu[menuElement][0])) - 1 ? subMenuElement = 0 : subMenuElement++;
        } else if (pressedKey == 0) { //ok save temp value.
          if (menuElement == 0 && subMenuValues[menuElement][subMenuElement] != 999) {

            tempSpacer = subMenuValues[menuElement][subMenuElement];

          } else if (menuElement == 1 && subMenuValues[menuElement][subMenuElement] != 999) {

            tempPumpSpeed = subMenuValues[menuElement][subMenuElement];

          }else if (menuElement == 2 && subMenuValues[menuElement][subMenuElement] != 999) {

            tempGlassThickness = subMenuValues[menuElement][subMenuElement];

          }
        }
      }

    } else if (pressedKey == 0 && menuElement == 4) {
      EEPROM.update(glassThicknessEepromaddress, tempGlassThickness);
      EEPROM.update(pumpSpeedEepromaddress, tempPumpSpeed);
      EEPROM.update(spacerEepromaddress, tempSpacer);
      show("Saved!");
      delay(2000);
      save = 1;
    } else if (pressedKey == 0 && menuElement == 3) {
      save = 999;
      show("Exit!");
      delay(1000);
    }
  }
}
void startProtocol() {
  show("Working...");
  sirenOn(1500);

  activate(fixation);
  show("Fixation ON!");
  Serial.println("Fixation ON");
  delay(outputDelay);
  !read(fixation1Sensor) ? next() : errorIs("Fixation Not AC.");

  disable(guide);
  show("Guide OFF!");
  Serial.println("guide OFF");
  delay(outputDelay);
  !read(guide0Sensor) ? next() : errorIs("Guide ACTIVE!");

  activate(xMotorEnable);
  
  Serial.println("x Motor ON");
  delay(500);
  Serial.println("Started Mooving X");
  show("Mooving X Fwd!");
  move(xAxisDir, xAxisStep, 1, maxStepsX, xAxisStepMicrodelay, x1Limit, glassSensor, maxStepsX, 0);
  show("Glass ?");
  Serial.println("Stoped Mooving X");
  read(x1Limit) ? next() : errorIs("Limit Error!");
  Serial.println("Glass maybe detected");
  Serial.println("Mooving a little bit more X");
  move(xAxisDir, xAxisStep, 1, glassSensorStartOffset * xStepsPerMm, xAxisStepMicrodelay, x1Limit, x1Limit, maxStepsX, 0);
  !read(glassSensor) ? next() : errorIs("Glass Error!");
  show("Glass Detected!");
  Serial.println("Glass Detected");


  activate(gunPressure);
  Serial.println("Gun Pressure ON");
  delay(outputDelay);
  !read(gunPressureSensor) ? next() : errorIs("GunPr. Not ACti.");

  activate(gun);
  show("Gun Active!");
  Serial.println("Gun ON");
  delay(outputDelayGun);
  !read(gunSensor) ? next() : errorIs("Gun Not Active!");

  Serial.println("Started Mooving X untill no more glass or limit X1");
  //mooving untill no glasse (Mode 2)
  move(xAxisDir, xAxisStep, 1, maxStepsX , xAxisStepMicrodelay, x1Limit, glassSensor, maxStepsX, 2);
  show("No More Glass!");
  Serial.println("Started Mooving X to compleate sensor offset");
  //compleate the offset distance
  move(xAxisDir, xAxisStep, 1, glassSensorStopOffset * xStepsPerMm, xAxisStepMicrodelay, x1Limit, x1Limit, maxStepsX, 0);
  
  //disable Gun
  disable(gun);
  Serial.println("Gun OFF");
  delay(outputDelayGun);
  read(gunSensor) ? next() : errorIs("Gun Still Active!");

  //scrapping
  activate(zMotorEnable);
  Serial.println("Z motor ON");
  delay(500);
  Serial.println("Scrapping Process");
  move(zAxisDir, zAxisStep, 0, scrapeDistance * zStepsPerMm, zAxisStepMicrodelay, z0Limit, z0Limit, maxStepsZ, 0);


  disable(gunPressure);
  Serial.println("Gun Pressure OFF");
  delay(outputDelay);
  read(gunPressureSensor) ? next() : errorIs("GunPr. Active!");
  
  disable(fixation);
  Serial.println("Fixation OFF");
  delay(outputDelay);
  !read(fixation0Sensor) ? next() : errorIs("Fixation active!");
  Serial.println("Mooving z to origin position");
  show("Z axis to pos.");
  move(zAxisDir, zAxisStep, 1, scrapeDistance * zStepsPerMm, zAxisStepMicrodelay, z1Limit, z1Limit, maxStepsZ, 0);
  
  Serial.println("Zero X");
  show("Zero X axis");
  move(xAxisDir, xAxisStep, 0, maxStepsX, xAxisStepMicrodelay, x0Limit, x0Limit, maxStepsX, 1);

  show("Done!  Guide up?");
  disable(xMotorEnable);
  disable(zMotorEnable);
  Serial.println("Disable Motor X and Z");

  waitForKeyPress(readyInterface,sizeof(readyInterface)/sizeof(readyInterface[0]));

  activate(guide);
  Serial.println("Activate Guide");
  delay(outputDelay);
  !read(guide1Sensor) ? next() : errorIs("Guide Not Active");
}

void errorIs(String data) {
  show(data);
  //disable(gun);
  //end();
  delay(2000);
  next();
}


void zeroProtocol() {
  show("Zero Protocol...");
  delay(1500);
  show("Pneumatic zero..");
  disable(gun);
  disable(gunPressure);
  activate(guide);
  disable(fixation);
  
  delay(outputDelay);

  
  allOk(6,zeroSensorsArray, zeroSateArray, zeroSensorsNameArray, true)? next() : errorIs("Pneu. Zero error");
  delay(1000);
  show("Z Axis zero..");
  activate(zMotorEnable);
  delay(500);
  move(zAxisDir, zAxisStep, 0, maxStepsZ, zAxisStepMicrodelay, z0Limit, z0Limit, maxStepsZ, 1);

  move(zAxisDir, zAxisStep, 1, (glassThickness+zOffset) * coeficient * zStepsPerMm, zAxisStepMicrodelay, z1Limit, z1Limit, maxStepsZ, 0);
          
  activate(xMotorEnable);
  show("X Axis zero..");
  delay(500);
  move(xAxisDir, xAxisStep, 0, maxStepsX, xAxisStepMicrodelay, x0Limit, x0Limit, maxStepsX, 1);

  disable(xMotorEnable);
  disable(zMotorEnable);
  firstStart=0;
}

void move(int axisDir, int axisStep, bool direction, int steps, int microdelay, int limit1, int limit2, int maxSteps, int Mode) {
  setDirection(axisDir, direction);
  stepCount = 0;
  if (Mode == 2) { //started Gun
    while (read(limit1) && !read(limit2) && stepCount < maxSteps && stepCount < steps) {
      oneStep(axisStep);
      delayMicroseconds(microdelay);
      stepCount++;
    }
  } else {
    while (read(limit1) && read(limit2) && stepCount < maxSteps && stepCount < steps) {
      oneStep(axisStep);
      delayMicroseconds(microdelay);
      stepCount++;
    }
  }
  if (Mode == 1 && checkState(limit1, 0, 0)) {
    errorIs("Zero error!");
    
  }
}




void next() {}
void activate(int pinNumber) {
  digitalWrite(pinNumber, 1);
}
void disable(int pinNumber) {
  digitalWrite(pinNumber, 0);
}
int waitForKeyPress(int keys[],int size) {
  keyState = 0;
  while (1) {
    for (int i = 0; i < size; i++)
    {
      if (!read(keys[i])) {
        previousTime = millis();
        while (!read(keys[i])) {

        }
        if ((millis() - previousTime) > longPress) {
          keyState = 2;
          return (i);
        } else if ((millis() - previousTime) > shortPress) {
          keyState = 1;
          return (i);
        }
      }
    }
  }
}


void sirenOn(int duration) {
  digitalWrite(siren, HIGH);
  delay(duration);
  digitalWrite(siren, LOW);
}


int EepromdataRead(int address) {
  return (EEPROM.read(address));
}

void show(String data) {
  //Set cursor to the second line
  lcd.setCursor(0, 1);
  //Clear the line and //Display Data
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(data);
  //display data on the serial Monitor
  Serial.println(data);
}
void end() {
  wdt_disable();
  activate (errorLight);
  sirenOn(2000);

  while (1) {};
}


bool read(int pinNumber) {
  Serial.print(pinNumber);
  Serial.print(" : ");
  Serial.println(digitalRead(pinNumber));
  
  return (digitalRead(pinNumber));
}
void setDirection(int axis, int dir) {
  digitalWrite(axis, dir);
}

void oneStep(int pinNumber) {
  digitalWrite(pinNumber, 1);
  delayMicroseconds(activeStepDelay);
  digitalWrite(pinNumber, 0);
}

bool checkState(int pinNumber, bool condition, bool value) {
  if (read(pinNumber) != condition) {
    return (!value);
  }
  return (value);
}

bool allOk(int size,int pins[], int state[], String names[], bool equality) {

  for (int i = 0; i < size; i++) {
    if (equality) {
      if (read(pins[i]) != state[i]) {
        temporaryString = "Err ";
        temporaryString += names[i];
        show(temporaryString);
        return (false);
      }
    } else {
      if (read(pins[i]) == state[i]) {
        temporaryString = "Err ";
        temporaryString += names[i];
        show(temporaryString);
        return (false);
      }
    }

  }
  return (true);
}
int noConflicts(int conflicts[][2],int sizeOfConflict, String names[],bool mode) {
  Serial.print("Conflicts");
  Serial.println(sizeOfConflict);
  
  for (int i = 0; i < sizeOfConflict; i++)
  {
    if (mode == 0 && checkState(conflicts[i][0], read(conflicts[i][1]), 1)) {
      return (i+1);
    }
    if (mode == 1  && checkState(conflicts[i][0], 0, 1) && checkState(conflicts[i][1], 0, 1)) {
      return (i+1);
    }
  }
  return (0);

}
