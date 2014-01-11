/* Bill acceptor applications
    DIY-SciB.org
    TokyoHackerSpace.org
    
    copyright shit goes here */
    
const float ver = 0.90;

/*    
    PINS:
    D0 - 1 debug serial          A0 - Membership payment button
    D2 - Pulse from BA           A1 - Membership LED
    D3 - BA Enable               A2 - Donation button
    D4 - LCD 7                   A3 - Donate LED
    D5 - LCD 6                   A4 - RTC SDA
    D6 - LCD 5                   A5 - RTC SCL
    D7 - LCD 4
    D8 - LCD E
    D9 - LCD RS
    D10 - printer serial TX
    D11 - printer serial RX
    D12 - audit button
    D13 - debugging
*/

// HARDWARE startups
#include <SoftwareSerial.h>
#include "Adafruit_Thermal.h"
Adafruit_Thermal printer(11,10);  // RX, TX

#include <LiquidCrystal.h>
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
#include <EEPROM.h>

#include "DIYSciBorg_RTC8564.h"
#include  <Wire.h>

RTC myClock;

// PIN defines
const int ledPin = 13;
const int BApulse = 2;
const int BAenable = 3;
const int printerTX = 10;
const int printerRX = 11;
const int auditBtn = 12;
const int debuggingLED = 13;
const int membershipBtn = 14;
const int membershipLED = 15;
const int donateBtn = 16;
const int donateLED = 17;

// attract mode variables
byte msgNumber= 0;
long previousAttractMillis = 0;
long attractInterval = 10000;

// Bill insertion and wait
volatile byte cashCount = 0;  // counter for cash inserted
int previousCashCount = 0;
long previousInsertMillis = 0;
long insertInterval = 1000; // wait for 10000 yen bills

// Button press debugging
long previousPressMillis = 0;
long pressInterval = 1000;

// EEPROM data locations
const int donationAudit = 0;
const int membershipAudit = 1;
const int escrowAudit = 2;
const int transactionAudit = 3;

// NV10USB constants
const int BAon = LOW;
const int BAoff = HIGH;


/***************** Reuseable strings *****************/
/* This is where code action starts. However, because the 
String to FLASH option does not work outside the calling function
we need to set up some quick print functions.
The goal here is to save RAM.
You will find Setup further down.
*/
void txtTHS(){
  printer.println(F("TokyoHackerSpace"));}
void txtSorry(){
  printer.println(F("Sorry. No prize."));}
void txtWinner(){
  printer.println(F("You are a WINNER!"));}
void txtWrite1(){
  printer.println(F("Write your name on this"));}
void txtWrite2(){
  printer.println(F("receipt. Insert it in the box."));}
void txtWrite3(){
  printer.println(F("Name: ________________________"));}
void txtWrite4(){
  printer.println(F("-------------TEAR-------------"));}


/***************** Let's get going! *****************/
void setup()
{
  Wire.begin();
  int Status = myClock.isRunning();
    // if unset, then set the clock
    if (Status == false){
      myClock.reset();   // sets all registeres in stable states
                  //year, month, day, weekday, hour, minute, second
      myClock.setTime(0x13, 0x01, 0x10, 0x05, 0x14, 0x20, 0x00);
    }
  
  pinMode(ledPin, OUTPUT);
  pinMode(BAenable, OUTPUT);
  pinMode(auditBtn, INPUT);
  pinMode(debuggingLED, OUTPUT);
  pinMode(membershipBtn, INPUT);
  pinMode(membershipLED, OUTPUT);
  pinMode(donateBtn, INPUT);
  pinMode(donateLED,OUTPUT);
  
  digitalWrite(donateLED, HIGH);
  digitalWrite(membershipLED, HIGH);
  digitalWrite(BAenable, BAoff);
  
  printer.begin();

  lcd.begin(20, 2);
  lcd.print("THS Payment Box");
  lcd.setCursor(6,1);
  lcd.print("ver: ");
  lcd.print(ver);
  
  attachInterrupt(0, caChing, FALLING);
  
  delay(3000);          // Wait for the BA to boot
 // EEPROM.write(escrowAudit, 0);   // just long enough to clear the eeprom. then remove and upload
  cashCount = EEPROM.read(escrowAudit); 
  
  
}

/************** what to do in ISR **************/
void caChing()
{
  cashCount++;    // for every bill pulse, add 1
   unsigned long currentInsertMillis = millis();
  if(currentInsertMillis - previousInsertMillis > insertInterval){
   previousInsertMillis = currentInsertMillis;
   cashIn();
  }
}

/************** Mainline **************/
void loop(){
 digitalWrite(ledPin, HIGH);
 digitalWrite(BAenable, BAon);
 
  if(cashCount == 0){
    digitalWrite(ledPin, LOW);
    idling();
  }
  if(cashCount != previousCashCount){
    digitalWrite(ledPin, LOW);
    cashIn();
  }
  if(cashCount != 0 && digitalRead(donateBtn) == LOW){
    unsigned long currentPressMillis = millis();
    if(currentPressMillis - previousPressMillis > pressInterval){
      previousPressMillis = currentPressMillis;
      digitalWrite(BAenable, BAoff);
      donateProcess();
    }
  }
  if(cashCount >= 5 && digitalRead(membershipBtn) == LOW){
    unsigned long currentPressMillis = millis();
    if(currentPressMillis - previousPressMillis > pressInterval){
      previousPressMillis = currentPressMillis;
      digitalWrite(BAenable, BAoff);
      membershipProcess();
    }
  }
  if(cashCount == 0 && digitalRead(auditBtn) == LOW){
    unsigned long currentPressMillis = millis();
    if(currentPressMillis - previousPressMillis > pressInterval){
      previousPressMillis = currentPressMillis;
      digitalWrite(BAenable, BAoff);
      auditProcess();
    }
  }
}        // keep it short and sweet!


/************** Audit Process **************/
void auditProcess(){
  digitalWrite(BAenable, BAoff);    // disable the acceptor
  lcd.clear();
  lcd.print("Audit mode!");
  delay(1000);
  lcd.clear();
  lcd.print("Donations: ");
  lcd.print(EEPROM.read(donationAudit));
  lcd.setCursor(0,1);
  lcd.print("Memberships: ");
  lcd.print(EEPROM.read(membershipAudit));
  delay(3000);
  lcd.clear();
  lcd.print("EXIT: Audit");
  lcd.setCursor(0,1);
  lcd.print("Clear: Membership");
  
  int holdingPattern = 1;
  while (holdingPattern = 1){
    digitalWrite(membershipLED, LOW);
  if(digitalRead(auditBtn) == LOW){
    unsigned long currentPressMillis = millis();
    if(currentPressMillis - previousPressMillis > pressInterval){
      previousPressMillis = currentPressMillis;
      holdingPattern = 0;
      lcd.clear();
      lcd.print("Audits preserved!");
      delay(1000);
      digitalWrite(membershipLED, HIGH);
      digitalWrite(BAenable, BAon);  // enable the acceptor
      break;
    }
  }
  if(digitalRead(membershipBtn) == LOW){
    unsigned long currentPressMillis = millis();
    if(currentPressMillis - previousPressMillis > pressInterval){
      previousPressMillis = currentPressMillis;
      holdingPattern = 0;
      digitalWrite(membershipLED, HIGH);
      clearAudits();
      digitalWrite(BAenable, BAon);  // enable the acceptor
      break;
    }
  }
 }
}

/************** Clear Audits **************/
void clearAudits(){
 EEPROM.write(donationAudit, 0);
 EEPROM.write(membershipAudit, 0);
 lcd.clear();
 lcd.print("Audits cleared!");
 delay(1000);
}


/************** Donate Process **************/
void donateProcess(){
  digitalWrite(BAenable, BAoff);
  lcd.clear();
  lcd.print("Thank you for your");
  lcd.setCursor(5,1);
  lcd.print("DONATION!");
  cashCount--;
  
  int donationValue = EEPROM.read(donationAudit);
  if (donationValue == 255){
    donationValue = 0;}
  donationValue++;
  EEPROM.write(donationAudit, donationValue);
  
  int transactionNumber = EEPROM.read(transactionAudit);
  if (transactionNumber == 255){
    transactionNumber = 0;}
  transactionNumber++;
  EEPROM.write(transactionAudit, transactionNumber);
  
  delay(1000);
  lcd.clear();
  lcd.print("We've had ");
  lcd.print(EEPROM.read(donationAudit));
  lcd.setCursor(0,1);
  lcd.print("donations this month");
  delay(1000);
//  printDonation();
  cashIn();
}

/************** Memebership Process **************/
void membershipProcess(){
  digitalWrite(BAenable, BAoff);
  lcd.clear();
  lcd.print("Printing receipts");
  cashCount = (cashCount - 5);
  
  int membershipValue = EEPROM.read(membershipAudit);
  if (membershipValue == 255){
    membershipValue = 0;}
  membershipValue++;
  EEPROM.write(membershipAudit, membershipValue);
  
  int transactionNumber = EEPROM.read(transactionAudit);
  if (transactionNumber == 255){
    transactionNumber = 0;}
  transactionNumber++;
  EEPROM.write(transactionAudit, transactionNumber);
  
  delay(1000);
  myClock.getTime();
  printTHScopy();
  printMembership();
  cashIn();
}
/************** Idling - attract **************/
void idling(){
 /* Here we put various attract messages on the screen
 while idling away with nothing else to do.
 */
 unsigned long currentAttractMillis = millis();
 if(currentAttractMillis - previousAttractMillis > attractInterval){
   previousAttractMillis = currentAttractMillis;
   switch (msgNumber) {
     case 0:
       lcd.clear();
       lcd.print("Make a donation!");
       msgNumber++;
       break;
     case 1:
       lcd.clear();
       lcd.print("Pay membership here!");
       msgNumber++;
       break;
     case 2:
       lcd.clear();
       lcd.print("Yo! Help us out!");
       msgNumber++;
       break;
       }
  lcd.setCursor(0,1);
  myClock.getTime();
  lcd.print(myClock.Year, HEX);
  lcd.print("/");
  lcd.print(myClock.Month, HEX);
  lcd.print("/");
  lcd.print(myClock.Day, HEX);
  lcd.print(" ");
  lcd.print(myClock.Hour);
  lcd.print(":");
  lcd.print(myClock.Minute);
 }
 if(msgNumber >= 3){  // set number of messages and resets
   msgNumber = 0;
 }
}


/************** CashIn **************/
void cashIn(){
  previousCashCount = cashCount;
  lcd.clear();
  lcd.print("You have ");
  lcd.print(cashCount * 1000);
  lcd.print(" yen!");
  setLEDs();
  EEPROM.write(escrowAudit, cashCount); 
}


/************** Set LEDs **************/
void setLEDs(){
  if(cashCount == 0){
    digitalWrite(donateLED, HIGH);
    digitalWrite(membershipLED, HIGH);
  }else if(cashCount < 5){
    digitalWrite(donateLED, LOW);
    digitalWrite(membershipLED, HIGH);
  }else{
      digitalWrite(donateLED, LOW);
      digitalWrite(membershipLED, LOW);
  } 
}

/********* Print member's receipt *********/
void printMembership(){
  lcd.clear();
  lcd.print("Keep this copy");
  
  printer.setSize('L');
  txtTHS();
  printer.setSize('M');
  printer.println("Member's Copy");
  printer.print("Receipt #M");
  printer.println(EEPROM.read(transactionAudit));
  printer.setSize('S');
  printer.print("Date: ");
  printer.print(myClock.Year, HEX);
  printer.print("/");
  printer.print(myClock.Month, HEX);
  printer.print("/");
  printer.println(myClock.Day, HEX);
  printer.print("Time: ");
  printer.print(myClock.Hour);
  printer.print(":");
  printer.println(myClock.Minute);
          // fill with date printing code later
  printer.feed(3);
  printer.println("Keep this receipt!");
  printer.println("This is to certify that you paid");
  printer.println("a 5000 yen membership.");
  printer.feed(3);
  txtWrite4();
  printer.feed(3);
  
}

/********* Print THS's receipt ***********/
void printTHScopy(){
  lcd.clear();
  lcd.print("Sign this one");
  lcd.setCursor(0, 1);
  lcd.print("Put in the slot");
  
  printer.setSize('L');
  txtTHS();
  printer.setSize('M');
  printer.println("Admin Copy");
  printer.print("Receipt #M");
  printer.println(EEPROM.read(transactionAudit));
  printer.setSize('S');
  printer.print("Date: ");
  printer.print(myClock.Year, HEX);
  printer.print("/");
  printer.print(myClock.Month, HEX);
  printer.print("/");
  printer.println(myClock.Day, HEX);
  printer.print("Time: ");
  printer.print(myClock.Hour);
  printer.print(":");
  printer.println(myClock.Minute);
  printer.feed(3);
  txtWrite1();
  txtWrite2();
  printer.feed(3);
  txtWrite3;
  printer.feed(3);
  txtWrite4();
  printer.feed(3);
  delay(2500);
}

/********* Print Donation receipt ***********/
void printDonation(){
  lcd.clear();
  lcd.print("Donation Receipt");
  
  printer.setSize('L');
  txtTHS();
  printer.setSize('M');
  printer.println("1000 yen Donation!");
  printer.print("Receipt #D");
  printer.println(EEPROM.read(transactionAudit));
  printer.setSize('S');
  printer.println("Date: ");
          // fill with date printing code later
  printer.feed(3);
  printer.println("Thank you for your donation.");
  printer.println("It will help us pay the bills.");
  printer.feed(3);
  txtWrite4();
  printer.feed(3);
}

