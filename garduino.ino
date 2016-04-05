
// Inlcudes
#include <LiquidCrystal.h>     // LCD Display

// Digital PINs
int LED_RED_PIN      = 6;      // Shows that irrigation is running
int LED_GREEN_PIN    = 7;      // Shows that the system is running
int TRIGGER_PIN     = 8;      // Push button to select the system profile
int MOISTURE_PIN     = A0;      // Moisture Sensor
int LCD_RS_PIN       = 12;     //LCD Pin
int LCD_ENABLE_PIN   = 11;     //LCD Pin
int LCD_D4_PIN       = 5;      //LCD Pin
int LCD_D5_PIN       = 4;      //LCD Pin
int LCD_D6_PIN       = 3;      //LCD Pin
int LCD_D7_PIN       = 2;      //LCD Pin
int V1_PIN           = 17;     //Valve1 Pin
int V2_PIN           = 18;     //Valve2 Pin
int V3_PIN           = 19;     //Valve3 Pin
int V4_PIN           = 20;     //Valve4 Pin
int V5_PIN           = 21;     //Valve5 Pin

int TR               = 0;      // Trigger Reading
int LTR              = 0;      // Last Trigger Reading

int actValve         = 0;      // Actual Valve
int actProfile       = 0;      // Actual Profile in Profile Array
int maxProfile       = 3;      // Max Array Element in Profile Array (start with 0) -> (length -1)

const int ACTIVE     = 0;      // System is active 
const int IDLE       = 1;      // System is idle
int state            = IDLE;   // ACTIVE or IDLE

int moistureValue    = 0;      // Value of the sensor

LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);   // LCD Display

int valves[] = {V1_PIN, V2_PIN, V3_PIN, V4_PIN, V5_PIN}; // array of the Valves PINs

typedef struct{
  String name;      // name of the profile to display
  int start;        // start time (minutes after 00:00)     
  int duration;     // duration in minutes
  int frequence;    // days e.g. 1 = daily, 2 = every second day, ...
  int moisture;     // 0-1023 treshhold if the moisture is over the value
  int predictive;   // 0/1
  int valves[5];     // percent of duration for this ventile
}profile;

profile profiles[10];

//Declaration of profiles
profile p1 = { "Dauer", 0, 2, 1,0,0,{20,20,20,20,20}};
profile p2 = { "Aus", 0, 0, 1,1023,0,{100,0,0,0,0}};
profile p3 = { "Feucht", 300, 10, 2,600,0,{20,20,20,20,20}};
profile p4 = { "Trocken", 300, 30, 1,600,0,{20,20,20,20,20}};

unsigned long previousMillis = 0; // last time update
long interval = 2000; // interval at which to do something (milliseconds)

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);  // LCD initialize

  // PINs
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(MOISTURE_PIN, INPUT);

  digitalWrite(TRIGGER_PIN, HIGH);     // Initialization

  // Zuordnung zum Array - TODO: Kann noch vereinfacht werden, wenn der Array direkt initialisiert wird
  profiles[0] = p1;
  profiles[1] = p2;
  profiles[2] = p3;
  profiles[3] = p4;
}

void loop() {
  listenTrigger();                        //listen to the Trigger 
  moistureValue=analogRead(MOISTURE_PIN); //get Moisture Value
  displayLCD();                           //displays the LCD
  displayLED();                           //display LEDs
  checkStatus();                          //check the status and action

  if(millis() - previousMillis > interval) { // every 2 Seconds the full status
    previousMillis = millis();   
    Serial.println("------------------------------------------------------------------------------------");
    Serial.print ("Status:           "); Serial.println(state);
    Serial.print ("Profile-Name:     "); Serial.println(profiles[actProfile].name);
    Serial.print ("Profile-Start:    "); Serial.println(profiles[actProfile].start);
    Serial.print ("Profile_Duration: "); Serial.println(profiles[actProfile].duration);
    Serial.print ("moistureValue:    "); Serial.println(moistureValue);
    Serial.print ("V1:               "); Serial.println(digitalRead(V1_PIN));
    Serial.print ("V2:               "); Serial.println(digitalRead(V2_PIN));
    Serial.print ("V3:               "); Serial.println(digitalRead(V3_PIN));
    Serial.print ("V4:               "); Serial.println(digitalRead(V4_PIN));
    Serial.print ("V5:               "); Serial.println(digitalRead(V5_PIN));
  }
}

void listenTrigger() {  // listen to the trigger button and changes the profile  ------------------------------------------------------------
  LTR = TR; // Records previous state
  TR = digitalRead(TRIGGER_PIN);  // Looks up current trigger button state
  
  if(TR != LTR && TR == LOW){ // if status changed and status LOW
    if (actProfile = maxProfile) {
      actProfile = 0;
    } else {
      actProfile++;
    }
  }
}

void displayLED() {  // displays LEDs by status ---------------------------------------------------------------------------------------------
  if(state==ACTIVE) {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
  } else {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
  }
}

void displayLCD() { // displays Information on LCD ------------------------------------------------------------------------------------------
  lcd.clear();

  //TODO Update after Timer integration, at the moment only millis() are used to test system so far

  if(state==ACTIVE) { 
    //FirstLine
    lcd.print(getTime(profiles[actProfile].start + profiles[actProfile].duration - millis()/1000/60));
    lcd.setCursor(0,15 - ((String)((profiles[actProfile].duration *100) / ((profiles[actProfile].start + profiles[actProfile].duration - millis()/1000/60)*100))).length());
    lcd.print ((String)((profiles[actProfile].duration *100) / ((profiles[actProfile].start + profiles[actProfile].duration - millis()/1000/60)*100)));
    lcd.print("%");
    //SecondLine
    lcd.setCursor(1,0);
    lcd.print("Ventil ");
    lcd.print(actValve + 1);
  } else {
    //First Line
    lcd.print(getTime(millis()/1000/60)); 
    lcd.setCursor(0,14 -(((String)(map(moistureValue, 0, 1023, 100, 0))).length()));
    lcd.print(map(moistureValue, 0, 1023, 100, 0));
    lcd.print("%");
    // Second Line
    lcd.setCursor(1,0);
    lcd.print(profiles[actProfile].name);
    lcd.setCursor(1,8);
    lcd.print(getTime(profiles[actProfile].start));
    lcd.setCursor(1,15 -(((String)(profiles[actProfile].duration)).length()));
    lcd.print(profiles[actProfile].duration);
  }
}

void checkStatus() { // acts based on profile and actual time ---------------------------------------------------------------------------------
  // System is more or less stateless - means each time the status will be checked and conducted

  state = IDLE;

  // are we within start and end?
  if( ((millis()/1000/60) >= profiles[actProfile].start) && ((millis()/1000/60) <= profiles[actProfile].start+ profiles[actProfile].duration)) {
    // is mositure above treshold?
    if(moistureValue > profiles[actProfile].moisture) {
      //iterate through the valves
      int percent = 0;
      for (int intI = 0; intI<= 5; intI++) {

        /*
        Serial.print("Valves Loop: ");
        Serial.print(millis()/1000);
        Serial.print(" IntI: ");
        Serial.print(intI);
        Serial.print(" Percent: ");
        Serial.print(percent); 
        Serial.print(" Min: "); 
        Serial.print(profiles[actProfile].start+(profiles[actProfile].duration*60*percent/100)); 
        Serial.print(" Max");
        Serial.println(profiles[actProfile].start*60+(profiles[actProfile].duration*60*(profiles[actProfile].valves[intI]+percent)/100));
        */
        
        if( ((millis()/1000) >= (profiles[actProfile].start+(profiles[actProfile].duration*60*percent/100))) && ((millis()/1000) <= profiles[actProfile].start*60+(profiles[actProfile].duration*60*(profiles[actProfile].valves[intI]+percent)/100))) {
          digitalWrite(valves[intI], HIGH);
          actValve = intI;
          state = ACTIVE;
        } else {
          digitalWrite(valves[intI], LOW);
        }  
        percent += profiles[actProfile].valves[intI];
      } 
    }
  } 

  // if no Valves were touchen, disable all valves
  if (state == IDLE) {
    for (int intI = 0; intI<= 5; intI++) {
      digitalWrite(valves[intI], LOW);    
    }
  }
}

String getTime(int time) { // converts Minutes to Time String (00:00) ------------------------------------------------------------------------
  String hour = (String)(time/60);
  String minute = (String)(time % 60);

  if (hour.length() == 1) { hour = "0" + hour;}
  if (minute.length() == 1) { minute = "0"+ minute;}

  return hour + ":" + minute;
}

