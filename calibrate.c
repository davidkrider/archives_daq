#include <stdio.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int main(void) {

	int32 error = 0;
	TaskHandle taskHandle = 0;
	char errBuff[2048] = {'\0'};
	int32 read = 0;
	float64 data[10];
	bool32 isTaskDone;

	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	DAQmxErrChk (DAQmxCreateAIFreqVoltageChan(taskHandle, "Dev1/ai0", "", 
		-5.0, 5.0, DAQmx_Val_Hz, 4.5, 0.5, ""));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle, "", 48000.0, DAQmx_Val_Rising,
		DAQmx_Val_ContSamps, 96000));
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	while (1) {
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandle, 1, 10, DAQmx_Val_GroupByChannel,
			data, sizeof(data), &read, NULL));
		printf("Current reading: %f\r", data[0]);
	}

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
		DAQmxIsTaskDone (taskHandle, &isTaskDone);
		while (!isTaskDone)
			printf("Waiting on task to finish...\n");
		DAQmxClearTask(taskHandle);
	}

	return 0;

}
