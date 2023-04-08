#include <LiquidCrystal.h>
#include <Encoder.h> 
#include <EEPROM.h>

LiquidCrystal lcd(4, 5, 6, 7, 8, 9);
//Variables for timer
unsigned long current = 0, normalizedTime = 0;
unsigned long previous = 0;   //Used to calculate speed
unsigned long pauseTime = 0;  //Used to stop timer in display when mode=paused 
unsigned long pausedTime = 0; //Total paused time to substract from total run time
const byte refreshTime = 500; //Refresh time for lcd
unsigned long setupTime = 0;
unsigned long auxTime = 0;
//Bicycle parameters
float radius = 0.5; //In m
//Pins
const byte hall = 2;
const byte pauseBt = 3;
//Variables for user performance
float km = 0.0;
unsigned long revs = 0;
float prevKm = 0;
float vel = 0;
int timestamp = 2000; //Miliseconds to calculate velocity
//Enable running program
bool pause = false;
bool runP = true;

//Define encoder for parameter selection
#define encoderA 11
#define encoderB 12
Encoder encoder(encoderA, encoderB);
int selection;
long lastEncoder = 0;
//Alarm buzzer
#define buzzer 13

//Variables for users
String names[5] = {"Jorge", "Xochitl", "Daniela", "Ximena", "Alex"};
byte user = 0;
byte auxUsers = 0; 
bool arrow = 0;
//Variables for settings adjustments
byte option=0;
unsigned int timer = 0;        //En minutos

void setup() {
  Serial.begin(9600);
  EEPROM.begin();
  lcd.begin(16,2);
  pinMode(hall, INPUT_PULLUP);
  pinMode(pauseBt, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  attachInterrupt(digitalPinToInterrupt(hall), newRev, FALLING);
  
  lcd.setCursor(3,0);       //Show title
  lcd.print("Performance");
  lcd.setCursor(4,1);
  lcd.print("Tracker");
  delay(1500);
  userMenu();     // Display User selection menu
  adjustMenu();
  setupTime = millis();
}

void loop() {  
  if(digitalRead(pauseBt) == LOW){
    stopRun();
    delay(100);
    while(!digitalRead(pauseBt));
  }
  if(runP){
    Serial.println(encoder.read());
    if(!pause){
      if(millis() - (current) >= refreshTime){
        current = millis();
        performance();
        normalizedTime = current - (pausedTime + setupTime + auxTime);
        showClock(int(normalizedTime/1000));
        if(timer > 0 && normalizedTime >= timer * 60000){  //Convert timer in minutes to milliseconds
          finishTraining(); // If training time exceeds timer, end training
        }
      }
      delay(15);
    }
    else{
      delay(700);
      pauseMenu();
    }
  } else{
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Hasta luego!");
    delay(500);
  }
}

void pauseMenu(){
  unsigned long start = millis();
  pause = true;
  lastEncoder = encoder.read();
  option = 0;
  byte auxMenu = 0; // To check where the arrow goes in the display
  arrow = 0;
  unsigned long auxTimer = 0;
  String options[3] = {"Temporizador", "Continuar", "Finalizar"};
  while(option == 0){
    selection = (encoder.read()- lastEncoder)%14;  // Module 29 to continue even if value is larger
    if(selection < 0){ //Do not allow it to go below 0
      selection = 0;
    }
    switch(selection){
      case 0 ... 4:   //Set timer
        if(!digitalRead(pauseBt)){
          delay(700);
          timer = setTimer(0, timer);     //Nuevo timer y agregar esto al timer inicial = 0 en este caso 
        } 
        arrow = 0;
        auxMenu = 0;
        break;
      case 5 ... 9: //Continue
        if(!digitalRead(pauseBt)){
          delay(700);
          option = 1;
          pause = false; 
          auxTime += millis() - start;
        }
        if(auxMenu > byte(selection / 10)){ arrow = 0; }else{ arrow = 1;} 
        auxMenu = 1;
        break;
      case 10 ... 14: //Finalizar
        if(!digitalRead(pauseBt)){
          delay(700);
          option = 1;
          runP = false;
          saveExit(); //Guardar performance data
          delay(700); 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Hasta luego!");
        }
        arrow = 1;
        auxMenu = 2;
        EEPROM.end();
        break;
    }
    if(millis() - auxTimer >= refreshTime){
      lcd.clear();
      auxTimer = millis();
      lcd.setCursor(0,arrow);
      lcd.print(">" + options[auxMenu]);
      if(arrow == 0){
        lcd.setCursor(0,1);
        lcd.print(options[auxMenu + 1]);
      }else{
        lcd.setCursor(0,0);
        lcd.print(options[auxMenu - 1]);
      }
    }
    delay(25);
  }
}

void userMenu(){
  while(user==0){
    selection = encoder.read()%24;
      if(selection < 0){ //Do not allow it to go below 0
        selection = 0;
      }
    switch(selection){
      case 0 ... 4:
        arrow = 0; // To check if > arrow goes in row 0 or 1 of lcd
        auxUsers=0; // User 1 hovered
        if(!digitalRead(pauseBt)){user = 1;}  
        break;  
      case 5 ... 9:
      if(auxUsers > byte(selection/10)){ arrow = 0;} else {arrow = 1;}
        auxUsers=1; // User 2 hovered
        if(!digitalRead(pauseBt)){user = 2;}
        break;
      case 10 ... 14:
      if(auxUsers > byte(selection/10)){ arrow = 0;} else {arrow = 1;}
        auxUsers=2; // User 3 hovered
        if(!digitalRead(pauseBt)){user = 3;}  
        break;
      case 15 ... 19:
      if(auxUsers > byte(selection/10)){ arrow = 0;} else{arrow = 1;}
        auxUsers=3; // User 4 hovered
        if(!digitalRead(pauseBt)){user = 4;}  
        break;
      case 20 ... 24:
        arrow = 1;
        auxUsers=4; // User 5 hovered
        if(!digitalRead(pauseBt)){user = 5;}  
        break;
    }
    //LCD DISPLAY
    if(millis() - setupTime >= refreshTime){ //Only refresh the necessary times
    setupTime = millis();
    lcd.clear();
    if(arrow == 0){
      lcd.setCursor(0,0);
      lcd.print(">" + names[auxUsers]);
      lcd.setCursor(1,1);
      lcd.print(names[auxUsers + 1]);
    }else{
      lcd.setCursor(0,1);
      lcd.print(">" + names[auxUsers]);
      lcd.setCursor(1,0);
      lcd.print(names[auxUsers - 1]);
    }
    } //End if
    delay(50);
  } //End while
  
  lcd.clear();         //Display a message with a duration to allow user to stop pressing button before loop
  lcd.setCursor(0,0);
  lcd.print("User: " + names[user -1]);  // Display user
  delay(2500);
  setupTime = millis();
  lcd.clear();
  lastEncoder = encoder.read(); //Update final encoder value to reset
}

void adjustMenu(){
  while(option == 0){
    lcd.clear();
    selection = (encoder.read()- lastEncoder)%14;  // Module 29 to continue even if value is larger
    if(selection < 0){ //Do not allow it to go below 0
      selection = 0;
    }
    Serial.println(selection);
    switch(selection){
      case 0 ... 4:
        lcd.setCursor(0,0);
        lcd.print(">Temporizador");
        lcd.setCursor(0,1);
        lcd.print("Comenzar");
        if(!digitalRead(pauseBt)){
          delay(500);
          lastEncoder = encoder.read();
          timer = setTimer(0, timer);
        }  
        break;
      case 5 ... 9:
        lcd.setCursor(0,0);
        lcd.print("Temporizador");
        lcd.setCursor(0,1);
        lcd.print(">Comenzar");
        if(!digitalRead(pauseBt)){
          delay(700);
          option = 1;
          } 
        break;
      case 10 ... 14:
        lcd.setCursor(0,0);
        lcd.print("Comenzar");
        lcd.setCursor(0,1);
        lcd.print(">Ver status");
        if(!digitalRead(pauseBt)){
          delay(500);
          showStatus();
        } 
        break;
    }
    delay(100);
  }
}

int setTimer(int localTimer, unsigned int temp){
  encoder.write(0);
  bool state = true;
  while(state){
    Serial.println(encoder.read());
    if(!digitalRead(pauseBt) && selection >= temp){
      state = false;
      localTimer = selection;
      delay(500);
    } 
    if(encoder.read() < 0){ 
      encoder.write(0);
      } else{
        selection = int(encoder.read()/2); //Reset encoder and for 2 encoder pulses +/- 1 to mins
      }

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Tiempo:");
    lcd.setCursor(9,0);
    lcd.print(String(selection) + " min");
    lcd.setCursor(0,1);
    lcd.print("Tiempo total: " + String(timer));
    delay(25);
  }
  return selection;
}

void showStatus(){
  float maxKm = 0.0f;            
  EEPROM.get((user-1)*4, maxKm); //User toma un valor de 1-5 pero se hace el map de 0-4
  while(true){
    if(!digitalRead(pauseBt)){
      delay(500);                 //Cuando se pulsa el boton se tiene que haacer un delay chico
      break;                      // para permitir al usuario dejar de pulsar el boton antes de que una nueva
    }                            // funcion inicie (ya que el boton se usa para varias cosas)
    if(millis() - setupTime >= refreshTime){
      setupTime = millis();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nombre: " + names[user-1]);
      lcd.setCursor(0,1);
      lcd.print("Record: " + String(maxKm) + "km");
    }
  }
}

void finishTraining(){
  byte rings = 0; //Total buzzer rings
  byte ringSecs = 1; //Duration of a ring
  unsigned long start = millis();
  unsigned long localTimer = 0;
  unsigned long lastRefresh = 0;
  while(true){
    if(!digitalRead(pauseBt)){
      delay(700);
      digitalWrite(buzzer, LOW);
      break;
    }
    if(millis() - localTimer >= ringSecs * 1000 && rings < 3*2){
      rings++;
      localTimer = millis(); 
      digitalWrite(buzzer, !digitalRead(buzzer)); //BUZZ!!
    }
    if(millis() - lastRefresh >= refreshTime){
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("Objetivo");
      lcd.setCursor(4,1);
      lcd.print("Logrado");
    }
    delay(25);
  }
  finishMenu(); //MENU
  auxTime = millis() -  start;  
}

void finishMenu(){
  lastEncoder = encoder.read();
  option = 0;
  while(option == 0){
    selection = (encoder.read() - lastEncoder) % 19;
    if(selection < 0){ selection = 0;}
    switch(selection){
      case 0 ... 9:
        if(!digitalRead(pauseBt)){
          delay(700);
          lastEncoder = encoder.read();
          timer = setTimer(0, timer);     //Nuevo timer y agregar esto al timer inicial
          option = 1;
        }
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(">Anadir tiempo");
        lcd.setCursor(0,1);
        lcd.print("Finalizar");  
        break;
      case 10 ... 19:
        if(!digitalRead(pauseBt)){
          delay(700);
          runP = false;
          option = 1;
          saveExit();
        }
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Anadir tiempo");
        lcd.setCursor(0,1);
        lcd.print(">Finalizar");
        break;
    }
    delay(25);
  }
}

void saveExit(){
  float maxKm = 0.0f;
  EEPROM.get((user-1)*4, maxKm);
  while(true){
    if(km > maxKm){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nuevo record");
      lcd.setCursor(0,1);
      lcd.print(String(km) + "km");
      EEPROM.put((user-1)*4, km);
      delay(5000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Datos Guardados");
      delay(1000);
      break;
    }else{
      break;
    }
  }
}

void performance(){
  lcd.clear();
  lcd.setCursor(0,0);
  km = revs * (3.1416 * 2 * radius)/1000;
  if(current - previous >= timestamp){
    vel = (km - prevKm)/((current - previous)/1000); //Speed in km/given timestamp
    vel = vel * (3600/(timestamp/1000));
    previous = current;
    prevKm = km;
  }
  lcd.print(String(km,1) + "km");
  lcd.setCursor(7,0);
  lcd.print(String(vel,1) + "km/h");
}

void showClock(int totalSecs){
  String mins = String(byte(totalSecs/60));
  String hours = String(byte(totalSecs/3600));
  String secs = String(byte(totalSecs%60));
  if(hours.length()<2){
    hours = "0" + hours;
  }
  if(mins.length()<2){
    mins = "0" + mins;
  }
  if(secs.length()<2){
    secs = "0" + secs;
  }
  lcd.setCursor(1,1);
  lcd.print("Time: " + hours + ":" + mins + ":" + secs);
}

void newRev(){
  if(!pause){
    revs++;
  }
}

void stopRun(){
  pause = !pause;
  if(pause == false){
    pausedTime += millis() - pauseTime;
  }else{
    pauseTime = millis();
  }
}
