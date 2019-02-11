//
//  Ligght.h
//  Ligght – Daisy-chained traffic lights controlled via I2C
//
//  Created by Janik Schmidt on 10.02.19.
//  Copyright © 2019 FESTIVAL Development. All rights reserved.
//

#include <Arduino.h>
#include <Wire.h>

#ifndef Ligght_h
#define Ligght_h

#define VERSION "1.0-4"
#define BUILD "19w07a.development.190211-1223"

#define BAUD_RATE 115200
#define MAX_OUT_CHARS 24

typedef enum LightState {
	Off,
	Maintenance,
	On,
	Debug	
};

typedef enum LightTransitionState {
	None,
	Green,
	Yellow,
	Red,
	RedYellow
};

typedef enum ButtonState {
	Pressed,
	Unpressed
};

typedef enum SerialCommand {
	SETTYPE,
	SETSTATE,
	SETTRANSITION
};

const byte ledPorts[] = { 0x05, 0x06, 0x07 };
const byte dipPorts[] = { 0x09, 0x0A, 0x0B, 0x0C };
const byte btnPort = 0x0E;

class Ligght {
	public:
		static void printHelp();
		static void printVersionInfo();
		static void sendSplitI2CMessage(byte* connectedBoards, int connectedBoardCount, String mainPayload, String secondaryPayload, byte flag);
		static void sendI2CMessage(byte address, String payload);
};

#endif