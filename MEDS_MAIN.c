/* 
M.E.D.S
"Medication Examination and Daily Sorting"
By: Connor Simmons, Aditya Matam, Sam Raisbeck
December 4th, 2015
*/

#include "PC_FileIO.c"

const string FILENAME = "medData.txt";
const float BOX_LEN = (1.23*180)/(PI*1.5); // encoder value of 1 compartment length
const float GARBAGE = -6*BOX_LEN-31;  // encoder value for discarding pills
const float EJECT = -8*BOX_LEN; // encoder value to eject entire organizer
const int SPIN = 358;  // encoder value for one full rotation
const int HOLD = 300;  // time delay between
int medData[6][7];
/* This 2D array contains 6 arrays, each size 7. The 6
arrays indicate the color (using the standard NXT color
configuration) and the 7 elements in each indicate the day
of the week (0 Sun, 1 Mon, ... 6 Sat).
There are 6 possible colors the NXT can read but we are only
using 3 of them (3 green, 4 yellow, 5 red). */

// ------------------------------------------------//

int readFile()
/* Reads medication data from a text file into proper
array positions. Returns total number of pills in the file. */
{
	TFileHandle fin;
	word fileSize = 1000;
	TFileIOResult status;
	OpenRead (fin, status, FILENAME, fileSize);
	string dispose = "";
	string colour = "";
	int index = 0, total = 0;
	for (int k = 0; k < 8; k++) // discard the headers
		PCReadString(fin, status, dispose);
	for (int i = 0; i < 3; i++)  // 3 valid colours
	{
		PCReadString (fin, status, colour);
		if (colour == "red")
			index = 4;
		else if (colour == "yellow")
			index = 3;
		else
			index = 2;
		for (int j = 0; j < 7; j++)
			medData [index][j] = PCReadInt (fin, status);
			total += medData[index][j];
	}
	return total;
}

// ------------------------------------------------//

void calibrateMotors()
/* Motor turns on to accept the pill organizer, turn off
when it is in place, and sets motor encoder to 0. */
{
	SensorType [S1] = sensorTouch; /* Initialize in the function because this is
								   the only place it is used, and only once. */
	motor [motorA] = 25;
	while (SensorValue[S1] == 0)
	{}
	motor[motorA]=0;
	nMotorEncoder [motorA]=0;
}

// ------------------------------------------------//

void dispensePill()
/* Moves an arm to grab a pill from the holder and pushes
it down into the colour sensor area. */
{
	nMotorEncoder[motorC] = 0;
	while (nMotorEncoder[motorC] < SPIN)
		motor[motorC] = 15;
	motor[motorC] = 0;
	wait1Msec(HOLD);
}

// ------------------------------------------------//

int getColour()
/* Reads the color sensor for some time (so the readings
can stabalize) and returns the value. */
{
	time1[T1]=0;
	int x = -1;
	while (time1[T1]<4000)
		x = SensorValue[S4];
	return x;
}

// ------------------------------------------------//

void openChute()
/* Opens the floor underneath the colour sensor to
drop the pill. */
{
	nMotorEncoder[motorB]=0;
	motor[motorB] = 10;
	while (nMotorEncoder[motorB]<25)
	{}
	motor[motorB]=0;
	wait1Msec (HOLD);
	motor[motorB] = -10;
	while (nMotorEncoder[motorB]>-1)
	{}
	motor[motorB]=0;
	nMotorEncoder[motorB]=0;
}

// ------------------------------------------------//

void moveGeneral(float encVal)
/* Takes an encoder value as a parameter and moves
the box until that encoder value is reached. */
{
	while (nMotorEncoder[motorA] > encVal)
		motor[motorA] = -20;
	motor[motorA] = 0;
	wait1Msec(HOLD);
	openChute();
	wait1Msec(HOLD);
	while (nMotorEncoder[motorA] < 0)
		motor[motorA] = 20;
	motor[motorA] = 0;
	nMotorEncoder[motorA] = 0;
}

// ------------------------------------------------//

bool moveToDay(int colour)
/* Takes the colour reading as a parameter and
moves the organizer appropriately then drops the
pill into the correct slot. Returns true if a successful
delivery was made, false otherwise. */
{
	bool dispensed = false;
	for (int i = 0; i < 7; i++)
		if (medData[colour-1][i] > 0 && !dispensed)
		{
			moveGeneral(-i*BOX_LEN);
			medData[colour-1][i]--;
			dispensed = true;
		}
	if (!dispensed)
	{
		moveGeneral(GARBAGE);
		wait1Msec(HOLD);
		return false;
	}
	return true;
}

// ------------------------------------------------//

void ejectBox()
/* Returns the pill organizer to the user. */
{
	while (nMotorEncoder[motorA] > EJECT)
		motor[motorA] = -25;
	motor[motorA] = 0;
	nMotorEncoder[motorA] = 0;
}

// ------------------------------------------------//

void conclude(bool failed)
/* Takes boolean as a parameter whether it
failed or not. Goes through the corresponding ending
procedure (displays messages).*/
{
	if (failed)
	{
		displayString(0, "ERROR");
		playSound(soundBeepBeep);
	}
	else
	{
		displayString(0, "SUCCESS");
		playSound(soundFastUpwardTones);
	}
	displayString(2, "Press any button");
	displayString(3, "to exit.");
	while (nNxtButtonPressed == -1)
	{}
	while (nNxtButtonPressed != -1)
	{}
}

//***--------- MAIN PROGRAM --------------------***//

task main()
{
	nVolume = 3;
	SensorType [S4] = sensorColorNxtFULL;
	int completed = 0, x = 0, failCount = 0;
	int total = readFile();
	calibrateMotors();
	wait1Msec(HOLD);
	while (completed < total && failCount < 5)
	{
		dispensePill();
		x = getColour();
		if (moveToDay(x))
		{
			completed++;
			failCount = 0;
		}
		else if (x == 6)
			failCount++;
		else
			failCount = 0;
	}
	ejectBox();
	conclude(failCount >= 5);
}
