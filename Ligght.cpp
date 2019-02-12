//
//  Ligght.cpp
//  Ligght – Daisy-chained traffic lights controlled via I2C
//
//  Created by Janik Schmidt on 10.02.19.
//  Copyright © 2019 FESTIVAL Development. All rights reserved.
//

#include "Ligght.h"

/**
 * Prints information about available commands to the serial console
 **/
void Ligght::printHelp() {
	Serial.println("\"help\"\n - Displays this help dialog");
	Serial.println("\"set_state\"\n - Sets the state of all connected traffic lights (0:Off, 1:Maintenance, 2:On, 3:Debug)");
	Serial.println("\"set_transition_state\"\n - Sets the transition state of all connected traffic lights (0:None, 1:Green, 2:Yellow, 3:Red, 4:RedYellow); requires debug mode");
	Serial.println("\"version\"\n - Prints version information");
}

/**
 * Prints the current version info
 **/
void Ligght::printVersionInfo() {
	// Create a char[] buffer so we can store a formatted string
	char buffer[256];
	// Put the information inside the buffer
	sprintf(buffer, "Ligght CLI\nVersion %s\nBuild %s", VERSION, BUILD);
	// Print the buffer in the serial console
	Serial.println(buffer);
}

/**
 * Sends an I2C message, split into "main street" and "side street" traffic lights
 * 
 * @param {byte*} connectedBoards An array containg the addresses of connected boards
 * @param {int} The amount of connected boards, since C is a stupid language and doesn't know how to count the items in a byte* array
 * @param {String} mainPayload The payload to send to "main street" traffic lights
 * @param {String} mainPayload The payload to send to "side street" traffic lights
 * @param {byte} flag A thing representing the "isSimpleTrafficLight" and "isUsedAsSlave" flags
 **/
void Ligght::sendSplitI2CMessage(byte* connectedBoards, int connectedBoardCount, String mainPayload, String secondaryPayload, byte flag) {
	// If we're a simple traffic light, send the secondary payload to the first address stored
	if (flag & 1) {
		Ligght::sendI2CMessage(connectedBoards[0], secondaryPayload);
		return;
	}
	
	// Send alternating payloads to the connected boards, depending if we're being used as a slave
	for (int i = 0; i < connectedBoardCount; i++) {
		String _payload = (i % 2 == !(flag & 2 == 0)) ? mainPayload : secondaryPayload;
		Ligght::sendI2CMessage(connectedBoards[i], _payload);
	}
}

/**
 * Sends an I2C message to a specified address
 * 
 * @param {byte} address The address of a connected board to send a message to
 * @param {String} payload The content of the message
 **/
void Ligght::sendI2CMessage(byte address, String payload) {
	// Open the connection to the specified address
	Wire.beginTransmission(address);
	// Write the payload to the I2C buffer
	Wire.write(payload.c_str());
	// Close the connection and let I2C do its magic
	Wire.endTransmission();
}