
void rfcontrol_command_send();
void rfcontrol_command_receive();
typedef unsigned long ulong; //RKR U N S I G N E D is so verbose
typedef unsigned int uint;

void PrintChar(byte S)
  {
  Serial.write(S);
  }
void PrintDash(void)
  {
  PrintChar('-');
  }

void PrintComma(void)
  {
  Serial.print(F(", "));
  }

void PrintNum(uint x, char c, int digits) {
     // Rinie add space for small digits
     if(c) {
     	PrintChar(c);
 	}
	for (uint i=1, val=10; i < digits; i++, val *= 10) {
		if (x < val) {
			PrintChar(' ');
		}
	}

    Serial.print(x,DEC);
}

void PrintNumHex(uint x, char c, uint digits) {
	// Rinie add space for small digits
	if(c) {
		PrintChar(c);
	}
	for (uint i=1, val=16; i < digits; i++, val *= 16) {
		if (x < val) {
			PrintChar('0');
		}
	}

	Serial.print(x,HEX);
}

#define NINJA_BLOCK
#ifndef NINJA_BLOCK
#define MonitorLedPin              13  // bij iedere ontvangst of verzending licht deze led kort op.
#else
#define BLUE_STAT_LED_PIN 	7
#define MonitorLedPin              BLUE_STAT_LED_PIN  // bij iedere ontvangst of verzending licht deze led kort op.
#endif

void rfcontrol_loop() {
  if(RFControl::hasData()) {
#ifndef RKR_NIBBLEINDEX
      unsigned int *timings;
      unsigned int timings_size;
      RFControl::getRaw(&timings, &timings_size);
      unsigned int buckets[8];
      unsigned int pulse_length_divider = RFControl::getPulseLengthDivider();
      RFControl::compressTimings(buckets, timings, timings_size);
      Serial.print("RF receive ");
      Serial.print(timings_size);
      Serial.print(":");
      for(unsigned int i=0; i < 8; i++) {
          unsigned long bucket = buckets[i] * pulse_length_divider;
          Serial.print(bucket);
          Serial.write(' ');
      }
      for(unsigned int i=0; i < timings_size; i++) {
          Serial.write('0' + timings[i]);
      }
      Serial.print("\r\n");
#else
	unsigned char *psiNibbles;
	unsigned int psMinMaxCount;
	unsigned int *psMicroMin;
	unsigned int *psMicroMax;
	unsigned char *psiCount;
      unsigned int pulse_length_divider = RFControl::getPulseLengthDivider();
	unsigned int package = RFControl::getPacket(&psiNibbles, &psMinMaxCount, &psMicroMin, &psMicroMax, &psiCount);
	unsigned int psCount;
	unsigned int pulseCount0 = 0;
	unsigned int pulseCount1 = 0;
	unsigned int pulseCountX = 0;
	unsigned int spaceCount0 = 0;
	unsigned int spaceCount1 = 0;
	unsigned int spaceCountX = 0;
	bool fHeader = false;
	bool fFooter = false;
	bool fPrintHex = false;
	if (!(package>0 /* || (psMinMaxCount > 0)*/) ) {
		      RFControl::continueReceiving();
		      return;
	}
	  digitalWrite(MonitorLedPin,HIGH);
	psCount = (psiNibbles[0] * 2) - (((psiNibbles[psiNibbles[0]] & 0x0F) == 0x0F) ? 1 : 0);
      Serial.print("RF receive ");
      Serial.print(psCount);
      Serial.print("#");
      Serial.print(package);
      for(unsigned int i=1; i <= psiNibbles[0]; i++) {
		    byte pulse = ((psiNibbles[i] >> 4) & 0x0F);
          	byte space = ((psiNibbles[i]) & 0x0F);
          	if (i == 1) { // check header
				fHeader = (pulse > 1) || (space > 1);
				continue;
			}
			else if (i == psiNibbles[0]) { // check footer
				fFooter = (pulse > 1) || (space > 1);
				continue;
			}

          	switch (pulse) {
			case 0:
				pulseCount0++;
				break;
			case 1:
				pulseCount1++;
				break;
			default:
				pulseCountX++;
				break;
			}
          	switch (space) {
			case 0:
				spaceCount0++;
				break;
			case 1:
				spaceCount1++;
				break;
			default:
				spaceCountX++;
				break;
			}
      }
	  if ((pulseCountX > 0) || (spaceCountX > 0)) {
		  // don't bother
	  }
	  else {
	      Serial.print(" {P");
	      Serial.print(pulseCount0);
	      Serial.print("/");
	      Serial.print(pulseCount1);
	      Serial.print(" S");
	      Serial.print(spaceCount0);
	      Serial.print("/");
	      Serial.print(spaceCount1);
	      Serial.print("}");
	      fPrintHex = true;
	  }
	  if (fPrintHex) {
		  if (fHeader) {
			unsigned int i=1;
			byte pulse = ((psiNibbles[i] >> 4) & 0x0F);
			byte space = ((psiNibbles[i]) & 0x0F);
			unsigned long bucket0 = ((psMicroMax[pulse] + psMicroMin[pulse]) / 2)  * pulse_length_divider;
			unsigned long bucket1 = ((psMicroMax[space] + psMicroMin[space]) / 2)  * pulse_length_divider;
			Serial.print(" H[");
			PrintNum(bucket0, ' ', 4);
			PrintNum(bucket1, ' ', 4);
			Serial.print("] ");
		  }
		  if (fFooter) {
			unsigned int i=psiNibbles[0];
			byte pulse = ((psiNibbles[i] >> 4) & 0x0F);
			byte space = ((psiNibbles[i]) & 0x0F);
			unsigned long bucket0 = ((psMicroMax[pulse] + psMicroMin[pulse]) / 2)  * pulse_length_divider;
			unsigned long bucket1 = ((psMicroMax[space] + psMicroMin[space]) / 2)  * pulse_length_divider;
			Serial.print(" F[");
			PrintNum(bucket0, ' ', 4);
			PrintNum(bucket1, ' ', 4);
			Serial.print("] ");
		  }
		Serial.print(" [");
		unsigned long bucket0 = ((psMicroMax[0] + psMicroMin[0]) / 2)  * pulse_length_divider;
		unsigned long bucket1 = ((psMicroMax[1] + psMicroMin[1]) / 2)  * pulse_length_divider;
		PrintNum(bucket0, ' ', 4);
		PrintNum(bucket1, ' ', 4);
		Serial.print("] ");

		unsigned int j = 0;
		byte x = 0;
		for(unsigned int i=1; i <= psiNibbles[0]; i++) {
			byte pulse = ((psiNibbles[i] >> 4) & 0x0F);
			byte space = ((psiNibbles[i]) & 0x0F);
			if ((i == 1) && fHeader) { // check header
				continue;
			}
			else if ((i == psiNibbles[0]) && fFooter) { // check footer
				continue;
			}
			x = (x << 2) | (pulse << 1) | space;
			j += 2;
			if (j >= 8) {
				PrintNumHex(x, 0, 2);
				j = 0;
				x = 0;
			}
		}
		if (j > 0) {
				PrintNumHex(x, 0, 2);
		}
	  }
	  else {
		  Serial.print(" [");
		  for(unsigned int i=0; i < psMinMaxCount; i++) {
			  unsigned long bucket = ((psMicroMax[i] + psMicroMin[i]) / 2)  * pulse_length_divider;
			  //Serial.print(psiCount[i]);
			  //Serial.write('/');
			  PrintNum(bucket, ' ', 4);
			  //Serial.write(' ');
		  }
			// detect sync out of band value
		  unsigned int i = ((psiNibbles[1] >> 4) & 0x0F);
		  if (i <= psMinMaxCount) {
			  i = ((psiNibbles[1]) & 0x0F);
		  }
		  if (i > psMinMaxCount) {
			  unsigned long bucket = ((psMicroMax[i] + psMicroMin[i]) / 2)  * pulse_length_divider;
			  PrintNum(bucket, ' ', 4);
			  //Serial.write(' ');
		   }
		  Serial.print("] ");

		  for(unsigned int i=1; i <= psiNibbles[0]; i++) {
				Serial.print(((psiNibbles[i] >> 4) & 0x0F),HEX);
				Serial.print(((psiNibbles[i]) & 0x0F),HEX);
		  }
		}
      Serial.print("\r\n");
	  digitalWrite(MonitorLedPin,LOW);
#endif
      RFControl::continueReceiving();
    }
}

void rfcontrol_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  if (strcmp(arg, "send") == 0) {
    rfcontrol_command_send();
  } else if (strcmp(arg, "receive") == 0) {
    rfcontrol_command_receive();
  } else {
    argument_error();
  }
}

void rfcontrol_command_receive() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int interrupt_pin = atoi(arg);
  RFControl::startReceiving(interrupt_pin);
  Serial.print("ACK\r\n");
}


void rfcontrol_command_send() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int transmitter_pin = atoi(arg);

  arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int repeats = atoi(arg);

  // read pulse lengths
  unsigned long buckets[8];
  for(unsigned int i = 0; i < 8; i++) {
    arg = sCmd.next();
    if(arg == NULL) {
      argument_error();
      return;
    }
    buckets[i] = strtoul(arg, NULL, 10);
  }
  //read pulse sequence
  arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  RFControl::sendByCompressedTimings(transmitter_pin, buckets, arg, repeats);
  Serial.print("ACK\r\n");
}