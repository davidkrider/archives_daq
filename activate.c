/*********************************************************************
*
* ANSI C Example program:
*    WriteDigChan.c
*
* Example Category:
*    DO
*
* Description:
*    This example demonstrates how to write values to a digital
*    output channel.
*
* Instructions for Running:
*    1. Select the digital lines on the DAQ device to be written.
*    2. Select a value to write.
*    Note: The array is sized for 8 lines, if using a different
*          amount of lines, change the number of elements in the
*          array to equal the number of lines chosen.
*
* Steps:
*    1. Create a task.
*    2. Create a Digital Output channel. Use one channel for all
*       lines.
*    3. Call the Start function to start the task.
*    4. Write the digital Boolean array data. This write function
*       writes a single sample of digital data on demand, so no
*       timeout is necessary.
*    5. Call the Clear Task function to clear the Task.
*    6. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal output terminals match the Lines I/O
*    Control. In this case wire the item to receive the signal to the
*    first eight digital lines on your DAQ Device.
*
*********************************************************************/

#include <stdio.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int main(void)
{
	int32 error = 0;
	TaskHandle taskHandle = 0;
	uInt8 data[8] = {0, 1, 1, 0, 1, 0, 1, 0};
	char errBuff[2048] = {'\0'};
	int counter;
	bool32 isTaskDone;

	DAQmxErrChk (DAQmxCreateTask("", &taskHandle));
	DAQmxErrChk (DAQmxCreateDOChan(taskHandle, "Dev1/port0/line0:7", "",
		DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxStartTask(taskHandle));
	DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle, 1, 1, 10.0, 
		DAQmx_Val_GroupByChannel, data, NULL, NULL));

	printf("Holding DO voltages. Press Enter key to quit\n");
	getchar();

	for (counter = 0; counter < 8; counter++) {
		data[counter] = 0;
	}
	DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle, 1, 1, 10.0, 
		DAQmx_Val_GroupByChannel, data, NULL, NULL));
	DAQmxStopTask(taskHandle);
	DAQmxIsTaskDone (taskHandle, &isTaskDone);
	while (!isTaskDone)
		printf("Waiting on task to finish...\n");
	DAQmxClearTask(taskHandle);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		printf("DAQmx Error: %s\n", errBuff);
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		return 1;
	}

	return 0;
}
