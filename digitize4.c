#include <stdio.h>
#include <unistd.h>
#include <NIDAQmx.h>

#define SCRATCH "/tmp"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK EveryNCallback(TaskHandle aiDigitizeTask,
	int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle aiDigitizeTask, int32 status,
	void *callbackData);
int32 writeWAVHeader(FILE *fp);
int32 pokeSampleCount(FILE *fp);

int totalSamples = 0;
TaskHandle aiDigitizeTask = 0, doSwitchTask = 0, diPowerTask = 0, diRewindTask = 0;
char sideA[512], sideB[512];

int main(int argc, char *argv[]) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};
	bool32 isTaskDone;
	int32 bytesPerSamp;
	int counter = 0;
	int byte;
	uInt8 relayBit[1], rewindBit[1], powerBit[1], rewindStatus, powerStatus;
	int32 written, read;

	sprintf(sideA, "%s/%s-a.wav", SCRATCH, argv[1]);
	sprintf(sideB, "%s/%s-b.wav", SCRATCH, argv[1]);
	FILE *fpx = fopen(sideA, "wb");
	FILE *fpy = fopen(sideB, "wb");
	writeWAVHeader(fpx);
	writeWAVHeader(fpy);
	fclose(fpx);
	fclose(fpy);

	// Define all the parts
	DAQmxErrChk (DAQmxCreateTask("AI_Digitize_Task", &aiDigitizeTask));
	DAQmxErrChk (DAQmxCreateTask("DO_Switch_Task", &doSwitchTask));
	DAQmxErrChk (DAQmxCreateTask("DI_Power_Task", &diPowerTask));
	DAQmxErrChk (DAQmxCreateTask("DI_Rewind_Task", &diRewindTask));

	// Setup all the parameters for each part
	DAQmxErrChk (DAQmxCreateAIVoltageChan(aiDigitizeTask, "Dev1/ai0,Dev1/ai15", "",
		DAQmx_Val_RSE, -5.0, 5.0, DAQmx_Val_Volts, NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(aiDigitizeTask, "", 625000.0,
		DAQmx_Val_Rising, DAQmx_Val_ContSamps, 2500000));
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(aiDigitizeTask,
		DAQmx_Val_Acquired_Into_Buffer, 50000, 0, EveryNCallback, NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(aiDigitizeTask, 0, DoneCallback, NULL));
	
	DAQmxErrChk (DAQmxCreateDOChan(doSwitchTask, "Dev1/port0/line0", "",
		DAQmx_Val_ChanForAllLines));
	
	DAQmxErrChk (DAQmxCreateDIChan(diRewindTask, "Dev1/port0/line1", "",
		DAQmx_Val_ChanForAllLines));

	DAQmxErrChk (DAQmxCreateDIChan(diPowerTask, "Dev1/port0/line2", "",
		DAQmx_Val_ChanForAllLines));
	
	// Turn ON the relay
	relayBit[0] = 1;
	DAQmxErrChk (DAQmxWriteDigitalU8(doSwitchTask, 1, 1, 10.0,
		DAQmx_Val_GroupByChannel, relayBit, &written, NULL));

	// Start looking for the REWIND light...
	// Sticking this between toggling the relay to give it a tiny second to do its work
	DAQmxErrChk (DAQmxStartTask(diRewindTask));

	sleep(1);

	// Turn OFF the relay
	relayBit[0] = 0;
	DAQmxErrChk (DAQmxWriteDigitalU8(doSwitchTask, 1, 1, 10.0,
		DAQmx_Val_GroupByChannel, relayBit, &written, NULL));

	// The unit starts in rewind mode, then clears the light, and records
	DAQmxErrChk (DAQmxReadDigitalU8(diRewindTask, 1, 10.0,
		DAQmx_Val_GroupByChannel, rewindBit, 1, &read, NULL));
	printf("Waiting on initial rewind...\n");
	rewindStatus = rewindBit[0];
	while (rewindBit[0] == rewindStatus) {
		DAQmxErrChk (DAQmxReadDigitalU8(diRewindTask, 1, 10.0,
			DAQmx_Val_GroupByChannel, rewindBit, 1, &read, NULL));
	}

	// Actually start the digitizing process
	DAQmxErrChk (DAQmxStartTask(aiDigitizeTask));

	// Look for the rewind to turn on (technically, again)
	DAQmxErrChk (DAQmxReadDigitalU8(diRewindTask, 1, 10.0,
		DAQmx_Val_GroupByChannel, rewindBit, 1, &read, NULL));
	printf("Recording...\n");
	rewindStatus = rewindBit[0];
	while (rewindBit[0] == rewindStatus) {
		DAQmxErrChk (DAQmxReadDigitalU8(diRewindTask, 1, 10.0,
			DAQmx_Val_GroupByChannel, rewindBit, 1, &read, NULL));
	}

	// Now that the REWIND light came on, and program execution got here, clear this task
	DAQmxStopTask(diRewindTask);
	DAQmxClearTask(diRewindTask);

	// Stop the digitizing, which will kick off the task's Done callback
	DAQmxStopTask(aiDigitizeTask);

	DAQmxIsTaskDone (aiDigitizeTask, &isTaskDone);
	while (!isTaskDone)
		printf("Waiting on buffer finish writing...\n");
	DAQmxClearTask(aiDigitizeTask);

/*
	This whole business is really academic. We can use the rewinding time
	to do the post-processing.
	
	// Look for POWER light to go off.
	DAQmxErrChk (DAQmxStartTask(diPowerTask));
	DAQmxErrChk (DAQmxReadDigitalU8(diPowerTask, 1, 10.0,
		DAQmx_Val_GroupByChannel, powerBit, 1, &read, NULL));
	printf("Waiting on duplicator to stop rewinding...\n");
	powerStatus = powerBit[0];
	while (powerBit[0] == powerStatus) {
		DAQmxErrChk (DAQmxReadDigitalU8(diPowerTask, 1, 10.0,
			DAQmx_Val_GroupByChannel, powerBit, 1, &read, NULL));
	}
	DAQmxStopTask(diPowerTask);
	DAQmxClearTask(diPowerTask);
*/

	printf("\nDone!\n");

Error:

	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		printf("DAQmx Error: %s\n", errBuff);
		DAQmxStopTask(aiDigitizeTask);
		DAQmxIsTaskDone (aiDigitizeTask, &isTaskDone);
		while (!isTaskDone)
			printf("Waiting on task to finish...\n");
		DAQmxClearTask(aiDigitizeTask);
		return 1;
	}

	fpx = fopen(sideA, "rb+");
	pokeSampleCount(fpx);
	fpy = fopen(sideB, "rb+");
	pokeSampleCount(fpy);

	return 0;

}


int32 CVICALLBACK EveryNCallback(TaskHandle aiDigitizeTask,
	int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};
	int32 pointsRead = 0;
	int16 data[100000];
	uInt8 finished[1] = {64};
	int32 bytesPerSamp;

	FILE *fpx = fopen(sideA, "ab+");
	FILE *fpy = fopen(sideB, "ab+");
	int i;
	int16 x, y;
	float a, b;

	/* From http://stackoverflow.com/questions/1149092/how-do-i-attenuate-a-wav-file-by-a-given-decibel-value
	public void AttenuateAudio(float[] data, int decibels) {
		float gain = (float)Math.Pow(10, (double)-decibels / 20.0);
		for (int i = 0; i < data.Length; i++) {
			data[i] *= gain;
		}
	}
	*/

	DAQmxErrChk (DAQmxReadBinaryI16(aiDigitizeTask, 50000, 10.0,
		DAQmx_Val_GroupByScanNumber, data, sizeof(data), &pointsRead, NULL));

	if (pointsRead > 0) {
		for (i = 0; i < pointsRead * 2; i = i + 2) {
			x = data[i] * 16;
			y = data[i + 1] * 16;
			a = x; b = y;
			// 7 dB = .44, 10 dB = .32, 13 dB = .22
			a = a - (b * 0.32);
			b = b - (a * 0.32);
			x = a; y = b;
			fwrite(&x, 2, 1, fpx);
			fwrite(&y, 2, 1, fpy);
		}
		printf("Acquired %d samples. %d total\r", pointsRead, totalSamples += pointsRead);
		fflush(stdout);
	}

	fclose(fpx);
	fclose(fpy);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxStopTask(aiDigitizeTask);
		DAQmxClearTask(aiDigitizeTask);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}


int32 CVICALLBACK DoneCallback(TaskHandle aiDigitizeTask, int32 status,
	void *callbackData) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};

	DAQmxErrChk (status);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxClearTask(aiDigitizeTask);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}


int32 writeWAVHeader(FILE *fp) {

	int temp32;
	short signed int temp16;
	const int CHANNELS = 1;
	const int BITS = 16;
	const int RATE = 38072;
	const int SAMPLES = 0;

	/* write the 'RIFF' header */
	fwrite("RIFF", 1, 4, fp);

	/* ChunkSize = 36 + SubChunk2Size */
	temp32 = SAMPLES * CHANNELS * BITS + 36;
	fwrite(&temp32, 1, 4, fp);

	/* write the 'WAVE' type */
	fwrite("WAVE", 1, 4, fp);

	/* write the 'fmt ' tag */
	fwrite("fmt ", 1, 4, fp);

	/* write the format record length: 16 for PCM */
	temp32 = 16;
	fwrite(&temp32, 1, 4, fp);

	/* write the format (PCM) */
	temp16 = 1;
	fwrite(&temp16, 1, 2, fp);

	/* write the channels */
	temp16 = CHANNELS;
	fwrite(&temp16, 1, 2, fp);

	/* write the sample rate */
	temp32 = RATE;
	fwrite(&temp32, 1, 4, fp);

	/* write the bytes/second */
	temp32 = RATE * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fp);

	/* write the block align */
	temp16 = CHANNELS * (BITS / 8);
	fwrite(&temp16, 1, 2, fp);

	/* write the bits/sample */
	temp16 = BITS;
	fwrite(&temp16, 1, 2, fp);

	/* write the 'data' tag */
	fwrite("data", 1, 4, fp);

	/* SubChunk2Size = NumSamples * NumChannels * bps/8 */
	temp32 = SAMPLES * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fp);

	return 0;

}


int32 pokeSampleCount(FILE *fp) {

	int temp32;
	const int CHANNELS = 1;
	const int BITS = 16;
	const int SAMPLES = totalSamples * 2;

	fseek(fp, 4, SEEK_SET);
	/* ChunkSize = 36 + SubChunk2Size */
	temp32 = SAMPLES * CHANNELS * BITS + 36;
	fwrite(&temp32, 1, 4, fp);

	fseek(fp, 40, SEEK_SET);
	/* SubChunk2Size = NumSamples * NumChannels * bps/8 */
	temp32 = SAMPLES * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fp);

	fclose(fp);

	return 0;
}
