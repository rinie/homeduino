
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
	unsigned int package = RFControl::getRawRkr(&psiNibbles, &psMinMaxCount, &psMicroMin, &psMicroMax, &psiCount);
      Serial.print("RF receive ");
      Serial.print(package);
      Serial.print("*");
      Serial.print(psiNibbles[0]);
      Serial.print("*");
      for(unsigned int i=0; i < psMinMaxCount; i++) {
          unsigned long bucket = ((psMicroMax[i] + psMicroMin[i]) / 2)  * pulse_length_divider;
          //Serial.print(psiCount[i]);
          //Serial.write('/');
          PrintNum(bucket, ' ', 4);
          Serial.write(' ');
      }
		// detect sync out of band value
      unsigned int i = ((psiNibbles[1] >> 4) & 0x0F);
      if (i <= psMinMaxCount) {
		  i = ((psiNibbles[1]) & 0x0F);
	  }
      if (i > psMinMaxCount) {
          unsigned long bucket = ((psMicroMax[i] + psMicroMin[i]) / 2)  * pulse_length_divider;
          PrintNum(bucket, ' ', 4);
          Serial.write(' ');
	   }
      Serial.print(":");

      for(unsigned int i=1; i <= psiNibbles[0]; i++) {
          	Serial.print(((psiNibbles[i] >> 4) & 0x0F),HEX);
          	Serial.print(((psiNibbles[i]) & 0x0F),HEX);
      }
      Serial.print("\r\n");
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