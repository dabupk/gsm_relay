#include "Sim800l.h"                    // sim800l library
#include <SoftwareSerial.h> //is necesary for the library!! 
#include <EEPROM.h>     // store variables data in EEMPROM
Sim800l Sim800l;  //to declare the library

String textSms;   // used to store incoming text sms 
int relay=13; // use what you need 
bool error;   // variable that holds error states of sim800 funcions
String number="";         // variable that stores number of incoming sms 
bool relay_state = LOW;           // variable for storing status of relay
unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long interval;           // interval at which to turn off (milliseconds)
unsigned long currentMillis;      // will store the current time of millis
volatile unsigned heart_beat=0;

void setup(){
    pinMode(relay,OUTPUT); 
    digitalWrite(relay,relay_state);
     OCR0A = 0xFA;           // Timer0 is already used for millis() - we'll just interrupt somewhere
     TIMSK0 |= _BV(OCIE0A);  // in the middle and call the "Compare A" function below
    if (EEPROMReadlong(0)==0){
      EEPROMWritelong(0,900000);
      }
      interval =  EEPROMReadlong(0);
      
    Serial.begin(9600); // only for debug the results . 
    Sim800l.begin(); // initializate the library. 
    Sim800l.reset();
    error=Sim800l.delAllSms(); //clean memory of sms;  
}

void loop(){
  textSms = Sim800l.listSms(); //list all the unread messages
  Serial.println(textSms);      // only for debuging 
    if (textSms.indexOf("CMGL:")!=-1){ //first we need to know if the messege is correct. NOT an ERROR
                if (user_verified()){     // check user verfication 
                set_pass();           // functions for setting new password
                textSms.toUpperCase();      // set all sms text to uppercase for easy use
                time_settings();        // set timer
                switching();            // set switching
                }
        }
         functions();                   // perform various functions
}

// function that will be called during each timer interupt
ISR(TIMER0_COMPA_vect){
  heart_beat++;
  //
  if (heart_beat < 30000){
    // reset code
    }
}

// check relay on timing
void callback(){
  currentMillis = millis();
   if ((currentMillis - previousMillis) >= interval) {     // if time is up and relay is on then turn it off
          if(relay_state == HIGH){
            relay_state = LOW;
            digitalWrite(relay,relay_state);
            error=Sim800l.sendSms(number,"LED TURN OFF");
          }
          previousMillis = currentMillis;
    }
}

// function setting new password
void set_pass(){
  if (textSms.indexOf("NEW-PASS='")!=-1){
       String password = textSms.substring((textSms.indexOf("NEW-PASS='")+10),(textSms.indexOf("'",textSms.indexOf("NEW-PASS='")+10)));
       write_String(10,password);
        error=Sim800l.sendSms(number,"New password set");
   }
}

//
bool user_verified(){
  textSms.remove(0,textSms.lastIndexOf("+92"));  // remove all the previous meesages, leaving only last message
  number = textSms.substring(textSms.indexOf("+92"),textSms.indexOf('"'));
  if(textSms.indexOf("PASS")!=-1){
    String password = textSms.substring((textSms.indexOf("PASS='")+6),(textSms.indexOf("'",textSms.indexOf("PASS='")+6)));
    if (password == read_String(10) || password == "12345678"){
      return true;
      }
      else {
        error=Sim800l.sendSms(number,"Wrong password");
        return false;
        } 
  }
  else{
    return false;
  }
}

void functions (){
    //Serial.println(heart_beat);        only for debugging
    heart_beat=0;
    callback();
    Sim800l.delreadSms();    //delete all read sms..so when receive a new sms always will be in first position   
//     if (textSms.indexOf("ER") != -1 || signal_quality() < 10){            // check signal quality
//    Sim800l.reset();
//    error=Sim800l.delAllSms(); //clean memory of sms;  
//     reset code
//   }
//   
}

void time_settings(){
  if (textSms.indexOf("TIME")!=-1){
       String time_interval = textSms.substring((textSms.indexOf("TIME='")+6),(textSms.indexOf("'",textSms.indexOf("TIME='")+6)));
       time_interval.trim();     
       if (time_interval.indexOf(".")==-1){              //check that time_interval only contains digits
             long my_interval = time_interval.toInt();
             if(my_interval <= 57600 && my_interval >= 1){       // 57600 is 40 days in minutes
                    interval = my_interval*60000;
                    EEPROMWritelong(0, interval);
                    previousMillis = currentMillis;
                    error=Sim800l.sendSms(number,"New time set");
              }
               else {
                     error=Sim800l.sendSms(number,"time value should be between 1 and 57600");
               }
       }
       else {
         error=Sim800l.sendSms(number,"Incorrect time format");
        }
   }
}


// functions for swtiching relay on and off
void switching(){
  if(textSms.indexOf("TURN")!=-1){
                if (textSms.indexOf("TURN=1")!=-1){       // if turn value is 1 then turn on relay 
                    relay_state = HIGH;
                    digitalWrite(relay,relay_state);
                    previousMillis = currentMillis;
                    error=Sim800l.sendSms(number,"LED TURN ON");
                }
                else if (textSms.indexOf("TURN=0")!=-1){       // if turn value is 0 then turn off relay 
                    relay_state = LOW;
                    digitalWrite(relay,relay_state);
                    error=Sim800l.sendSms(number,"LED TURN OFF");
                }
                 else {
                    error=Sim800l.sendSms(number,"Incorrect switching format");
                 }
          }          
}

  
// functions for checking signal strength
int signal_quality (){
        //check signal quality 
      String signal_quality = Sim800l.signalQuality();
      signal_quality = signal_quality.substring((signal_quality.indexOf(':')+1),signal_quality.indexOf(','));
      signal_quality.trim();
      int sig_quality = signal_quality.toInt();
      return sig_quality;
  }

//writes long variable consisting of 4 bytes to EEPROM
void EEPROMWritelong(int address, unsigned long value){

  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//reads long variable consisting of 4 bytes from EEPROM
unsigned long EEPROMReadlong(unsigned long address){
    unsigned long four = EEPROM.read(address);
    unsigned long three = EEPROM.read(address + 1);
    unsigned long two = EEPROM.read(address + 2);
    unsigned long one = EEPROM.read(address + 3);

    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}


// function for reading strings from EEMPROM
String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<50)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

// function for writing strings to EEMPROM
void write_String(char add,String data)
{
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');   //Add termination null character for String Data
}
