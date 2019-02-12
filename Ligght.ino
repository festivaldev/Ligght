//
//  Ligght.ino
//  Ligght – Daisy-chained traffic lights controlled via I2C
//
//  Created by Janik Schmidt on 10.02.19.
//  Copyright © 2019 FESTIVAL Development. All rights reserved.
//

#include "Ligght.h"
#include "LigghtPrograms.h"

// Enum values representing this board's current state
LightState state = Off;
LightTransitionState transitionState = None;
ButtonState buttonState = Unpressed;

// Flags representing this board's capablilites
bool isMaster = false;
bool isSimpleTrafficLight = false;
bool isUsedAsSlave = false;

// Store this board's address and the connected boards (if master)
byte boardAddress;
byte* connectedBoards;
int connectedBoardCount;

// Store the current processor time
unsigned long cMillis = 0;

void setup() {
	// Begin the serial connection with BAUD_RATE baud (defined in Ligght.h)
	Serial.begin(BAUD_RATE);
	
	// Setup inputs and outputs
	for (byte b: ledPorts) {
		pinMode(b, OUTPUT);
	}
	for (byte b: dipPorts) {
		pinMode(b, INPUT_PULLUP);
	}
	pinMode(btnPort, INPUT_PULLUP);
	
	// Read the value of our dip switch and shift the bits accordingly
	byte addressBits[] = {
		digitalRead(dipPorts[0]) << 3,
		digitalRead(dipPorts[1]) << 2,
		digitalRead(dipPorts[2]) << 1,
		digitalRead(dipPorts[3]) << 0,
	};
	
	// Calculate this board's address and set the isMaster flag if the address is 0
	boardAddress = 15 - (addressBits[0] + addressBits[1] + addressBits[2] + addressBits[3]);
	isMaster = (boardAddress == 0);
	
	// Set the clock speed of the I2C bus to 400 kHz
	Wire.setClock(400E3);
	if (isMaster) {
		// Setup the I2C Protocol
		// If we're the master, we don't supply an address to Wire.begin()
		Wire.begin();
		// Scan for connected devices (address range 0x01-0x0F => 15 theoretical devices)
		scan(0x01, 0x0F);
		// Set the type of the connected boards (only required if we're a simple traffic light)
		setSlaveType(isSimpleTrafficLight);
		// Turn every connected board off (actually only the LEDs, but who cares anyways?)
		setSlaveState(Off, Off);
		
		// Print Ligght's version info and that we're the master board
		Ligght::printVersionInfo();
		Serial.println("Board Type: MASTER");
		Serial.println("Type \"help\" for available commands");
	} else {
		// Setup the I2C Protocol
		Wire.begin(boardAddress);
		// Specify the event handlers for I2C
		Wire.onRequest(onRequest);
		Wire.onReceive(onReceive);
		
		// Print Ligght's version info and that we're a slave board
		Ligght::printVersionInfo();
		Serial.println("Board Type: SLAVE");
	}
}

void loop() {
	// If we're the master board, listen for incoming serial messages
	if (isMaster && Serial.available()) {
		onSerial();
	}
	
	// If we're the master board, a simple traffic light and we're enabled,
	// check the button states of both our board and the slave
	if (isMaster && isSimpleTrafficLight && state > Off) {
		// Request the button state of the slave board
		byte incoming;
		Wire.requestFrom((int)connectedBoards[0], 1);
		
		// Read whatever the slave responds
		while (Wire.available()) {
			incoming = Wire.read();
		}
		
		// Get our button state
		int _btnState = !digitalRead(btnPort);
		
		// If either of the buttons is pressed, change state to "On"
		if ((incoming || _btnState) && buttonState != Pressed) {
			// Any button is pressed, so we need to store this event to prevent
			// this check from constantly running and triggering multiple times
			buttonState = Pressed;
			
			// Reset the stored processor time if we're not in the "On" state
			if (state != On || transitionState == None) {
				cMillis = millis();
			}
			
			// If we're in the last step of the "On" state, reset the processor time
			if (millis() - cMillis >= 51E3 && millis() - cMillis < 71E3) {
				/// TODO: calculate remaining time;
				cMillis = millis();
			}
			
			// Update our and the slave's states
			setSlaveState(Off, On);
			state = On;
		} else if (!(incoming || _btnState)) {
			// No button is pressed or a pressed button has been released
			buttonState = Unpressed;
		}
	}	
	
	// Render the current state (turn on/off LEDs 'n stuff)
	renderState();
}



#pragma mark - Runtime

/**
 * Scans for connected addresses in a specified range
 * 
 * @param {byte} lowerBound The lower bound on the range (inclusive)
 * @param {byte} upperBound The upper bound on the range (inclusive)
 **/
void scan(byte lowerBound, byte upperBound) {
	// Create an array that contains if an address is reachable or not
	bool reachable[upperBound - lowerBound];
	
	// Check for reachable addresses
	for (int i = lowerBound; i<= upperBound; i++) {
		// Begin connection check
		Wire.beginTransmission(i);
		// If the connection result is 0 (connected), we can store this index in the array above
		if (Wire.endTransmission() == 0) {
			reachable[i - lowerBound] = true;
			// We need to track this because C doesn't know how to count byte* arrays
			connectedBoardCount++;
		} else {
			// bool array may initialize with "false" values, but let's make sure they're actually "false"
			reachable[i - lowerBound] = false;
		}
	}
	
	// Reinitalize the connectedBoards array with the count of actually connected boards
	connectedBoards = new byte[connectedBoardCount];
	int index = 0;
	// Iterate through the "reachable" array using the specified bounds
	for (int i = lowerBound; i <= upperBound; i++) {
		// If an address is marked reachable, we can store it in the "connectedBoards" array
		if (reachable[i - lowerBound] == true) {
			connectedBoards[index++] = i;
		}
	}
	
	// Set our boards capability flags
	isSimpleTrafficLight = (connectedBoardCount <= 1);
	isUsedAsSlave = (connectedBoardCount >= 2 && connectedBoardCount <= 3);
}

/**
 * Renders the current state, and (if we're the Master board) sets the state of every board connected
 **/
void renderState() {
	// If we're the master board, we're in charge of what the connected boards render
	if (isMaster) {
		// Check in which state we're currently in
		switch (state) {
			case Maintenance:
				// Use the program defined in LigghtPrograms.h to iterate through its steps
				for (LightProgramStep step: maintenance) {
					// If the processor time is in the range of beginTime..beginTime+duration, we can change transition states
					if (millis() - cMillis >= step.beginTime && millis() - cMillis < (step.beginTime + step.duration)) {
						setSlaveTransitionState(step.mainState, step.secondaryState);

						// If we're a simple traffic light or being used as a slave, we can also react to transition state changes
						if (isSimpleTrafficLight) {
							transitionState = step.secondaryState;
						} else {
							transitionState = (isUsedAsSlave) ? step.mainState : None;
						}
					}
					
					// If we've reached the final step of a program, reset the processor time
					if (step.isFinalStep && millis() - cMillis >= (step.beginTime + step.duration)) {
						cMillis = millis();
						break;
					}
				}
				break;
			case On:
				if (isSimpleTrafficLight) {
					// We're a simple traffic light, so use the simpleLight program
					for (LightProgramStep step: simpleLight) {
						// Same as above
						if (millis() - cMillis >= step.beginTime && millis() - cMillis < (step.beginTime + step.duration)) {
							setSlaveTransitionState(step.mainState, step.secondaryState);
							transitionState = step.secondaryState;
						}
						
						// Same as above, but upon reaching the final step, we fall back into "Maintenance"
						if (step.isFinalStep && millis() - cMillis >= (step.beginTime + step.duration)) {
							setSlaveState(Off, Maintenance);
							state = Maintenance;
						
							cMillis = millis();
							break;
						}
					}
				} else {
					// We're a 4-way junction, so use the fourWayJunction program
					for (LightProgramStep step: fourWayJunction) {
						// Same as above
						if (millis() - cMillis >= step.beginTime && millis() - cMillis < (step.beginTime + step.duration)) {
							setSlaveTransitionState(step.mainState, step.secondaryState);
							
							if (isUsedAsSlave) {
								transitionState = step.mainState;
							}
						}
						
						if (step.isFinalStep && millis() - cMillis >= (step.beginTime + step.duration)) {
							cMillis = millis();
							break;
						}
					}
				}
			default: break;
		}
	}
	
	// If we're currently in the "Off" state, we don't want any LED to light up
	if (state == Off) {
		transitionState = None;
	}
	
	// Turn on/off LEDs according to our current transition state
	switch (transitionState) {
		case None:
			digitalWrite(ledPorts[0], LOW);
			digitalWrite(ledPorts[1], LOW);
			digitalWrite(ledPorts[2], LOW);
			break;
		case Green:
			digitalWrite(ledPorts[0], LOW);
			digitalWrite(ledPorts[1], LOW);
			digitalWrite(ledPorts[2], HIGH);
			break;
		case Yellow:
			digitalWrite(ledPorts[0], LOW);
			digitalWrite(ledPorts[1], HIGH);
			digitalWrite(ledPorts[2], LOW);
			break;
		case Red:
			digitalWrite(ledPorts[0], HIGH);
			digitalWrite(ledPorts[1], LOW);
			digitalWrite(ledPorts[2], LOW);
			break;
		case RedYellow:
			digitalWrite(ledPorts[0], HIGH);
			digitalWrite(ledPorts[1], HIGH);
			digitalWrite(ledPorts[2], LOW);
			break;
		default: break;
	}
}



#pragma mark - Event Handling

/**
 * Handles I2C byte requests
 * 
 * @param {int} byteCount The amount of bytes we're supposed to send. (Unused)
 **/
void onRequest(int byteCount) {
	// The master board requested our current button state, so we respond to that
	Wire.write(!digitalRead(btnPort));
}

/**
 * Handles incoming I2C messages and parses commands contained in the payload
 * 
 * @param {int} byteCount The amount of bytes we're supposed to receive. (Unused)
 **/
void onReceive(int byteCount) {
	// Check if we really recevied a message
	if (Wire.available()) {
		String input = "";
		
		// Read all valid characters inside this message
		while (true) {
			char inputChar = Wire.read();
			// If an invalid character has been found, we need to break out of the while loop immediately
			if (!(isAlpha(inputChar) || isDigit(inputChar) || inputChar == ':')) {
				break;
			}
			
			// Add the parsed char to our string
			input += inputChar;
		}

		// Flush the I2C buffer in case we broke out earlier
		while (Wire.available()) {
			Wire.read();
		}
		
		// Here we parse the I2C message to a command
		// Command strings are always in the same format: "cmd:arg" (note the delimiter)
		char args[2][MAX_OUT_CHARS + 1];
		char* delimiter = ":";
		
		// Split the command using our delimiter
		int i = 0;
		char* ptr = strtok(input.c_str(), delimiter);
		while (ptr != NULL) {
			strcpy(&args[i++][0], ptr);
			ptr = strtok(NULL, delimiter);
		}
		
		// Handle the parsed command
		handleI2CCommand(args);
	}
}

/**
 * Handles parsed I2C commands
 * 
 * @param {char[][]} args The arguments contained in the payload of an I2C message
 **/
void handleI2CCommand(char args[2][MAX_OUT_CHARS + 1]) {
	// Check which type of command we got
	switch ((SerialCommand)atoi(args[0])) {
		case SETTYPE:
			// The master wants to set our type
			isSimpleTrafficLight = atoi(args[1]);
			break;
		case SETSTATE:
			// The master wants to set our operation state
			state = atoi(args[1]);
			break;
		case SETTRANSITION:
			// The master wants to set our transition state
			transitionState = atoi(args[1]);
			break;
		default: break;
	}
}

/**
 * Handles incoming serial messages and parses commands contained in the payload
 **/
void onSerial() {
	// Check if we received a serial message
	if (Serial.available()) {
		// Read the input and transform it to lowercase
		String input = Serial.readStringUntil('\n');
		input.toLowerCase();
		// Print the input to the serial console
		Serial.print("] ");
		Serial.println(input);
		
		// Transform the String object to char[] because strtok cannot handle String
		char _input[input.length() + 1];
		strcpy(_input, input.c_str());
		
		// Here we parse the serial message to a command
		// Command strings are always in the same format: "cmd arg"
		char args[2][MAX_OUT_CHARS + 1];
		char delimiter[] = " ";

		// Split the command using our delimiter
		int i = 0;
		char* ptr = strtok(_input, delimiter);
		while (ptr != NULL && i < 2) {
			strcpy(&args[i++][0], ptr);
			ptr = strtok(NULL, delimiter);
		}
		
		// Handle the parsed command
		handleSerialCommand(args);
	}
}

/**
 * Handles parsed serial commands
 * 
 * @param {char[][]} args The arguments contained in the payload of a serial message
 **/
void handleSerialCommand(char args[2][MAX_OUT_CHARS + 1]) {
	if (strcmp(args[0], "help") == 0) {
		// Print available commands
		Ligght::printHelp();
	} else if (strcmp(args[0], "version") == 0) {
		// Print version information
		Ligght::printVersionInfo();
	} else if (strcmp(args[0], "set_state") == 0) {
		// Set the operating state
		
		// Cast the passed state into LightState, just in case
		LightState _state = (LightState)atoi(args[1]);
		
		// Check if we have a valid value
		if (!(_state >= Off && _state <= Debug)) {
			Serial.println("Invalid value for LightState");
		} else {
			// Reset the processor time
			cMillis = millis();
			
			Serial.print("Setting LightState to ");
			Serial.println(_state);
			
			// Actually set our and the slave's state
			state = _state;
			setSlaveState(_state, _state);
		}
	} else if (strcmp(args[0], "set_transition_state") == 0) {
		// Set the transition state (requires Debug mode)
		
		// Check if we're in Debug mode
		if (state != Debug) {
			Serial.println("set_transition_state: Not in Debug mode!");
			return;
		}
		
		// Cast the passed state into LightTransitionState, just in case
		LightTransitionState _state = (LightTransitionState)atoi(args[1]);
		
		// Check if we have a valid value
		if (!(_state >= None && _state <= RedYellow)) {
			Serial.println("Invalid value for LightTransitionState");
		} else {
			// Reset the processor time
			cMillis = millis();
			
			Serial.print("Setting LightTransitionState to ");
			Serial.println(_state);
			
			// Actually set our and the slave's state
			transitionState = _state;
			setSlaveTransitionState(_state, _state);
		}
	} else {
		// We received an invalid command
		Serial.print("Unknown command \"");
		Serial.print(args[0]);
		Serial.println("\"");
	}
}



#pragma mark - Communication
/**
 * Sets the type of every connected board
 * 
 * @param {int} type The type to set (0: 4-way junction, 1: simple traffic light)
 **/
void setSlaveType(int type) {
	// Create a char[] buffer where we can store a formatted string in
	char buffer[MAX_OUT_CHARS + 1];
	// Put the command and the argument inside that buffer
	sprintf(buffer, "%d:%d", SerialCommand::SETTYPE, type);
	
	// Send the message to every connected board
	for (int i = 0; i < connectedBoardCount; i++) {
		Ligght::sendI2CMessage(connectedBoards[i], buffer);
	}
}

/**
 * Sets the operating state of connected traffic lights
 * 
 * @param {LightState} mainState The operating state of "main street" traffic lights
 * @param {LightState} secondaryState The operating state of "side street" traffic lights
 **/
void setSlaveState(LightState mainState, LightState secondaryState) {
	// Create two independent buffers to store formatted strings
	char mainBuffer[MAX_OUT_CHARS + 1];
	char secondaryBuffer[MAX_OUT_CHARS + 1];
	// Put the command and the arguments inside those buffers
	sprintf(mainBuffer, "%d:%d", SerialCommand::SETSTATE, mainState);
	sprintf(secondaryBuffer, "%d:%d", SerialCommand::SETSTATE, secondaryState);
	
	// Send a split I2C message using the two buffers to the connected boards
	Ligght::sendSplitI2CMessage(connectedBoards, connectedBoardCount, mainBuffer, secondaryBuffer, (isSimpleTrafficLight << 0) | (isUsedAsSlave << 1));
}

/**
 * Sets the transition state (aka phase) of connected traffic lights
 * 
 * @param {LightTransitionState} mainState The transition state of "main street" traffic lights
 * @param {LightTransitionState} secondaryState The transition state of "side street" traffic lights
 **/
void setSlaveTransitionState(LightTransitionState mainState, LightTransitionState secondaryState) {
	// Create two independent buffers to store formatted strings
	char mainBuffer[MAX_OUT_CHARS + 1];
	char secondaryBuffer[MAX_OUT_CHARS + 1];
	// Put the command and the arguments inside those buffers
	sprintf(mainBuffer, "%d:%d", SerialCommand::SETTRANSITION, mainState);
	sprintf(secondaryBuffer, "%d:%d", SerialCommand::SETTRANSITION, secondaryState);
	
	// Send a split I2C message using the two buffers to the connected boards
	Ligght::sendSplitI2CMessage(connectedBoards, connectedBoardCount, mainBuffer, secondaryBuffer, (isSimpleTrafficLight << 0) | (isUsedAsSlave << 1));
}