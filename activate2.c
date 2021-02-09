/*********************************************************************
*
* ANSI C Example program:
*    VoltUpdate.c
*
* Example Category:
*    AO
*
* Description:
*    This example demonstrates how to output a single Voltage Update
*    (Sample) to an Analog Output Channel.
*
* Instructions for Running:
*    1. Select the Physical Channel to correspond to where your
*       signal is output on the DAQ device.
*    2. Enter the Minimum and Maximum Voltage Ranges.
*    Note: Use the Acq One Sample example to verify you are
*          generating the correct output on the DAQ device.
*
* Steps:
*    1. Create a task.
*    2. Create an Analog Output Voltage Channel.
*    3. Use the Write function to Output 1 Sample to 1 Channel on the
*       Data Acquisition Card.
*    4. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal output terminal matches the Physical
*    Channel I/O Control. For further connection information, refer
*    to your hardware reference manual.
*
*********************************************************************/

#include <stdio.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else 

int main(void) {

	int error = 0;
	TaskHandle taskHandle = 0;
	char errBuff[2048] = {'\0'};
	float64 data[1] = {10.0};
	bool32 isTaskDone;

	DAQmxErrChk (DAQmxCreateTask("", &taskHandle));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle, "Dev1/ao0", "", -10.0, 10.0, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxStartTask(taskHandle));
	DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL));

	printf("End of program, press Enter key to quit\n");
	getchar();

	data[0] = 0.0;
	DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL));
	DAQmxStopTask(taskHandle);
	DAQmxIsTaskDone (taskHandle, &isTaskDone);
	while (!isTaskDone)
		printf("Waiting on task to finish...\n");
	DAQmxClearTask(taskHandle);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}

	return 0;

}
