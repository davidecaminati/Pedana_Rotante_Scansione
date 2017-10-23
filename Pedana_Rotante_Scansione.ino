// Bounce.pde
// -*- mode: C++ -*-
//
// Make a single stepper bounce from one limit to another
//
// Copyright (C) 2012 Mike McCauley
// $Id: Random.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

/*
 FUNCTION = 0, DRIVER = 1, FULL2WIRE = 2, FULL3WIRE = 3,
  FULL4WIRE = 4, HALF3WIRE = 6, HALF4WIRE = 8 
*/

// modified by Davide Caminati 
// version 0.9

#include <AccelStepper.h>

// Define a stepper and the pins it will use
AccelStepper stepper(8,4,5,7,6); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

volatile boolean Allarme = false;

volatile bool rotation_left_available = true;
volatile bool rotation_right_available = true;
String actual_rotation = "";

String version = "0.9";

int pin_velocita = A0; // wire 6 color yellow
int pin_ampiezza = A1; // wire 6 color white
int pin_accelerazione = A2; // wire 6 color brown

int end_stop_left = 34;  // green
int end_stop_right = 36; // white
int end_stop_common = 38; //  yellow + brown


// Potentiometers BOX Buttons
int reset_button = 26; // wire 4 color yellow
int center_button = 24; // wire 4 color brown
int sx_button = 28; // wire 4 color white

//  Potentiometer BOX LED
int potenziometer_box_green = A5; // wire 6 color green
int potenziometer_box_red = 22; // wire 4 color green


int maxSpeed = 500;
int acceleration = 300;
int ampiezza = 0;
int old_val_ampiezza = 0;

int led_eye_left = 8;  // wire 6  color brown
int led_eye_right = 9;  // wire 6 color white

int led_red = 10; // wire 6  color red
int led_yellow = 11; // wire 6  color green
int led_green = 12; // wire 6 color yellow

int rele1 = A3;
int rele2 = A4;

int val_velocita;
int val_ampiezza;
int val_accelerazione;

boolean setupComplete=false;
long debounceDelay = 50;    // the debounce time; increase if the output flickers
long lastDebounceTime = 0;  // the last time the output pin was toggled
boolean last_reset_button_val = true;
boolean detect_mode = false;

boolean stringComplete = false;  // whether the string is complete
String inputString = "";         // a string to hold incoming data

int endstop_right_pin = 34;
int endstop_left_pin = 36;

boolean Debug = true;
void setup()
{  
  // Change these to suit your stepper if you want
	stepper.setMaxSpeed(maxSpeed);
	stepper.setAcceleration(acceleration);
	stepper.move(ampiezza);
  
  // ENDSTOP
	pinMode(endstop_right_pin,INPUT_PULLUP);
	pinMode(endstop_left_pin,INPUT_PULLUP);
	pinMode(end_stop_common,OUTPUT);
	delay(100);
	digitalWrite(end_stop_common,LOW);
  
  // RELE
	pinMode(rele1,OUTPUT);
	pinMode(rele2,OUTPUT);
  
  
    // initialize serial:
  Serial.begin(9600);
}

void loop()
{
	
	int reset_button_val =  digitalRead(reset_button);
	
	if (reset_button_val != last_reset_button_val) {
    	lastDebounceTime = millis();
  	}

  if (debounceDelay + lastDebounceTime > millis()) {
    if (reset_button_val != last_reset_button_val) {
      last_reset_button_val = reset_button_val;
      if (last_reset_button_val == LOW) {
        detect_mode = !detect_mode;
        analogWrite(potenziometer_box_green,detect_mode *255);
      }
    }
  }
  last_reset_button_val = reset_button_val;
  
	if (reset_button_val == LOW){
		RiarmaHandler();
		BlinkAllLED();
		
	}
	
	if (digitalRead(endstop_right_pin) == LOW){
		rotation_right_available = true;
	}
	else
	{
		rotation_right_available = false;
	}
	if (digitalRead(endstop_left_pin) == LOW){
		rotation_left_available = true;
	}
	else
	{
		rotation_left_available = false;
	}
	
    if (Debug) {
      val_velocita = 700;
      val_ampiezza = 4000;
      val_accelerazione = 200;
    }
    else
    {
      val_velocita = map(analogRead(pin_velocita),0,1024,800,300) ;
      val_ampiezza = map(analogRead(pin_ampiezza),0,1020,4000,1000) ;
      val_accelerazione = map(analogRead(pin_accelerazione),0,1024,400,100) ;
    }
    
    
	if (rotation_right_available == false)
	{        
		LedON(potenziometer_box_red);
	    LedON(led_eye_right);
	}
	else if (rotation_left_available == false)
	{       
		LedON(potenziometer_box_red);
		LedON(led_eye_left);
	}
	else
	{
        LedOFF(potenziometer_box_red);
        LedOFF(led_eye_left);
        LedOFF(led_eye_right);
	}
	

    stepper.setMaxSpeed(val_velocita);
    
    if (abs(old_val_ampiezza - val_ampiezza) > 100){
      old_val_ampiezza = val_ampiezza;
      ampiezza = val_ampiezza;
    }
    
    stepper.setAcceleration(val_accelerazione);
    
    
    if ((rotation_right_available == false) && (actual_rotation == "right") ){
    	rotation_left_available = true;
        stepper.disableOutputs();
        StopMotore();
        stepper.setCurrentPosition(0);
    }
    if ((rotation_left_available == false) && (actual_rotation == "left") ){
    	rotation_right_available = true;
        stepper.disableOutputs();
        StopMotore();
        stepper.setCurrentPosition(0);
    }
    else
    {
        stepper.enableOutputs();
        Muovi();
    }

    if (detect_mode){
    	Execute();
	}
	else{
		stepper.move(0); 
	}
    
      
}


void Execute(){
    if (stringComplete) {
      if (inputString.startsWith("--L") && (rotation_left_available)) 
      {
      	actual_rotation = "left";
      	rotation_right_available = true;
        StartMotore();
        stepper.move(ampiezza); 
      };
      if (inputString.startsWith("--R") && (rotation_right_available))
      {
      	actual_rotation = "right";
      	rotation_left_available = true;
        StartMotore();
        stepper.move(-ampiezza);
      }
      if (inputString.startsWith("--A"))  // allarme
      {
        stepper.disableOutputs();
        StopMotore();
        stepper.move(0);
      }
      if (inputString.startsWith("--D"))   // disarma
      {
        stepper.enableOutputs();
        stepper.move(0);
      }
      if (inputString.startsWith("--5"))   // Led Verde
      {
		LedOFF(led_green);
      }
      if (inputString.startsWith("++5"))   // Led Verde
      {
		LedON(led_green);
      }
      if (inputString.startsWith("--4"))   // Led Verde
      {
		LedOFF(led_yellow);
      }
      if (inputString.startsWith("++4"))   // Led Verde
      {
		LedON(led_yellow);
      }
      if (inputString.startsWith("--3"))   // Led Verde
      {
		LedOFF(led_red);
      }
      if (inputString.startsWith("++3"))   // Led Verde
      {
		LedON(led_red);
      }
      if (inputString.startsWith("--2"))   // Led rosso alto
      {
		LedOFF(led_eye_right);
      }
      if (inputString.startsWith("++2"))   // Led rosso alto
      {
		LedON(led_eye_right);
      }
      if (inputString.startsWith("--1"))   // Led rosso alto
      {
		LedOFF(led_eye_left);
      }
      if (inputString.startsWith("++1"))   // Led rosso alto
      {
		LedON(led_eye_left);
      }
      if (inputString.startsWith("?"))   // request kind of device
      {
		Serial.println("motor");
      }
      if (inputString.startsWith("V"))   // request software verion
      {
		Serial.println(version);
      }
    // clear the string:
    inputString = "";
    stringComplete = false;
    }
    
}

void LedON(int pin){
	digitalWrite(pin,HIGH);
}

void LedOFF(int pin){
	digitalWrite(pin,LOW);
}

void RiarmaHandler(){
  Allarme = false;
  actual_rotation = "";
  rotation_left_available = true;
  rotation_right_available = true;
}

void Muovi(){
    // If at the end of travel go to the other end
    if (stepper.distanceToGo() == 0){
      stepper.disableOutputs();
      StopMotore();
    }
    stepper.run();
}

void BlinkAllLED(){
	LedON(led_eye_left);
	LedON(led_eye_right);
	LedON(led_red);
	LedON(led_yellow);
	LedON(led_green);
	delay(500);
	LedOFF(led_eye_left);
	LedOFF(led_eye_right);
	LedOFF(led_red);
	LedOFF(led_yellow);
	LedOFF(led_green);
}

void StopMotore(){
    digitalWrite(rele1,HIGH);
    digitalWrite(rele2,HIGH);
}

void StartMotore(){
    digitalWrite(rele1,LOW);
    digitalWrite(rele2,LOW);
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
