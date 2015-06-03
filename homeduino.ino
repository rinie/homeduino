#include <avr/pgmspace.h>
#include <SerialCommand.h>
#include <RFControl.h>
#include <DHTlib.h>

void argument_error();

SerialCommand sCmd;

#ifdef KEYPAD_ENABLED
#include <Keypad.h>
#include "keypad_command.h"
#endif
#include "dht_command.h"

void digital_read_command();
void digital_write_command();
void analog_read_command();
void analog_write_command();
void reset_command();
void pin_mode_command();
void ping_command();
void unrecognized(const char *command);
#undef NODO_HARDWARE
#undef NINJA_BLOCK
#define MOTEINO
#ifdef MOTEINO
#include <SPI.h>
#include <RFM69.h>
#define NODEID      1
#define NETWORKID   100
#define FREQUENCY   RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
RFM69 radio;
#endif
#ifndef NINJA_BLOCK
#define MonitorLedPin              13  // bij iedere ontvangst of verzending licht deze led kort op.
#ifdef NODO_HARDWARE
#define IR_ReceiveDataPin           3  // Op deze input komt het IR signaal binnen van de TSOP. Bij HIGH bij geen signaal.
#define IR_TransmitDataPin         11  // Aan deze pin zit een zender IR-Led. (gebufferd via transistor i.v.m. hogere stroom die nodig is voor IR-led)
#define RF_TransmitPowerPin         4  // +5 volt / Vcc spanning naar de zender.
#define RF_TransmitDataPin          5  // data naar de zender
#define RF_ReceiveDataPin           2  // Op deze input komt het 433Mhz-RF signaal binnen. LOW bij geen signaal.
#define RF_ReceivePowerPin         12  // Spanning naar de ontvanger via deze pin.
#endif
#else
#define BLUE_STAT_LED_PIN 	7
#define MonitorLedPin              BLUE_STAT_LED_PIN  // bij iedere ontvangst of verzending licht deze led kort op.
#endif
#include "rfcontrol_command.h"

int lastRssi = -100;
unsigned long lastMillis = 0;

void setup() {
	Serial.begin(115200);
	// Setup callbacks for SerialCommand commands
	sCmd.addCommand("DR", digital_read_command);
	sCmd.addCommand("DW", digital_write_command);
	sCmd.addCommand("AR", analog_read_command);
	sCmd.addCommand("AW", analog_write_command);
	sCmd.addCommand("PM", pin_mode_command);
	sCmd.addCommand("RF", rfcontrol_command);
	sCmd.addCommand("IR", ircontrol_command);
	sCmd.addCommand("PING", ping_command);
	sCmd.addCommand("DHT", dht_command);
  sCmd.addCommand("RESET", reset_command);
  #ifdef KEYPAD_ENABLED
  sCmd.addCommand("K", keypad_command);
  #endif
	sCmd.setDefaultHandler(unrecognized);
#ifndef NINJA_BLOCK
#ifdef MOTEINO
  delay(10);
  pinMode(3,INPUT);
  pinMode(4,INPUT);
  radio.initialize_OOK(FREQUENCY);
//  radio.radioOnOff(true);
  //radio.setHighPower(); //uncomment only for RFM69HW!
  //radio.promiscuous(promiscuousMode);
	Serial.println(F("Moteino 433.92\r\n"));
#endif
#ifdef NODO_HARDWARE
  pinMode(IR_ReceiveDataPin,INPUT);
  pinMode(RF_ReceiveDataPin,INPUT);
  pinMode(RF_TransmitDataPin,OUTPUT);
  pinMode(RF_TransmitPowerPin,OUTPUT);
  pinMode(RF_ReceivePowerPin,OUTPUT);
  pinMode(IR_TransmitDataPin,OUTPUT);
#endif

  pinMode(MonitorLedPin,OUTPUT);
#ifdef NODO_HARDWARE
  digitalWrite(IR_ReceiveDataPin,HIGH);  // schakel pull-up weerstand in om te voorkomen dat er rommel binnenkomt als pin niet aangesloten
  digitalWrite(RF_ReceiveDataPin,HIGH);  // schakel pull-up weerstand in om te voorkomen dat er rommel binnenkomt als pin niet aangesloten
  // nodo hardware
  digitalWrite(RF_ReceivePowerPin,HIGH); // Spanning naar de RF ontvanger aan.
#endif
#else
	pinMode(BLUE_STAT_LED_PIN, OUTPUT);
	digitalWrite(BLUE_STAT_LED_PIN, LOW);	        // Power on Status
#endif
	Serial.print(F("Homeduino Baudrate 115200 ready\r\n"));
	Serial.print(F("try RF receive 0\r\n"));
}

void loop() {
	int rssi;
	// handle serial command
	sCmd.readSerial();
	// handle rf control receiving
	rfcontrol_loop();
  #ifdef KEYPAD_ENABLED
	// handle keypad keypress
	keypad_loop();
  #endif
#if 0
  if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    Serial.print(" [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");
    Serial.print(" [DATALEN:");Serial.print(radio.DATALEN);Serial.print("]");
    Serial.println();
   // Blink(LED,3);
  }
  else {
	  	rssi = radio.readRSSI(false);
		unsigned long now = millis();
	  	if (rssi > lastRssi) {
			if (lastMillis == 0) {
				lastMillis = now;
			}
			//if (now-lastMillis > 10000) {
				Serial.print(now-lastMillis);
				Serial.print(" [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");
				Serial.println();
				lastMillis = now;
			//}
			lastRssi = rssi;
		}
		else if (now-lastMillis > 10000) {
			lastRssi = rssi;
		}
  }
#endif
}

void digital_read_command() {
	char* arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int pin = atoi(arg);
  	int val = digitalRead(pin);
  	Serial.print("ACK ");
  	Serial.write('0' + val);
  	Serial.print("\r\n");
}

void analog_read_command() {
	char* arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int pin = atoi(arg);
  	int val = analogRead(pin);
  	Serial.print("ACK ");
  	Serial.print(val);
  	Serial.print("\r\n");
}

void digital_write_command() {
	char* arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int pin = atoi(arg);
  	arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int val = atoi(arg);
  	digitalWrite(pin, val);
  	Serial.print("ACK\r\n");
}

void analog_write_command() {
	char* arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int pin = atoi(arg);
  	arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int val = atoi(arg);
  	analogWrite(pin, val);
  	Serial.print("ACK\r\n");
}

void pin_mode_command() {
	char* arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	int pin = atoi(arg);
  	arg = sCmd.next();
  	if(arg == NULL) {
  		argument_error();
    	return;
  	}
  	// INPUT 0x0
	// OUTPUT 0x1
  	int mode = atoi(arg);
  	pinMode(pin, mode);
    Serial.print("ACK\r\n");
}


void ping_command() {
  char *arg;
  Serial.print("PING");
  arg = sCmd.next();
  if (arg != NULL) {
    Serial.write(' ');
    Serial.print(arg);
  }
  Serial.print("\r\n");
}


void reset_command() {
  RFControl::stopReceiving();
  Serial.print("ready\r\n");
}

void argument_error() {
	Serial.print(F("ERR argument_error\r\n"));
}
// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
	Serial.print(F("ERR unknown_command\r\n"));
}

