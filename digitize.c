/*
 This one works by reading in both sides to two binary files, then making
 a wav file, and appending the first side, and then appending the second 
 side in reverse order.
*/

#include <stdio.h>
#include <NIDAQmx.h>

int totalSamples = 0;

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else


int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, 
	uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);
int32 writeWAVFile();


int main(void) {

	int32 error = 0;
	TaskHandle taskHandle = 0;
	char errBuff[2048] = {'\0'};
	bool32 isTaskDone;

	FILE *fpx = fopen("/home/david/x.bin", "wb");
	FILE *fpy = fopen("/home/david/y.bin", "wb");
	fclose(fpx);
	fclose(fpy);

	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	// I think this was an attempt to create a dB scale. I couldn't make it work.
	//DAQmxErrChk (DAQmxCreateLinScale ("customScale", 2.0, 0.0, DAQmx_Val_Volts,
	//	"customUnits"));
	//DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default,
	//	-5.0, 5.0, DAQmx_Val_FromCustomScale, "customScale"));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default,
		-5.0, 5.0, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle, "", 625000.0, DAQmx_Val_Rising,
		DAQmx_Val_ContSamps, 2500000));
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(taskHandle, DAQmx_Val_Acquired_Into_Buffer,
		50000, 0, EveryNCallback, NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));

	DAQmxErrChk (DAQmxStartTask(taskHandle));

	printf("Acquiring samples continuously. Press Enter to interrupt\n");
	getchar();

	DAQmxStopTask(taskHandle);
	DAQmxIsTaskDone (taskHandle, &isTaskDone);
	while (!isTaskDone)
		printf("Waiting on task to finish...\n");
	DAQmxClearTask(taskHandle);

	writeWAVFile();

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


int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType,
	uInt32 nSamples, void *callbackData) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};
	int32 read = 0;
	int16 data[100000];

	FILE *fpx = fopen("/home/david/x.bin", "abc+");
	FILE *fpy = fopen("/home/david/y.bin", "abc+");
	int i;
	int16 x, y;

	static int16 max = 0;
	static int16 min = 0;
	
	DAQmxErrChk (DAQmxReadBinaryI16(taskHandle, 50000, 10.0, DAQmx_Val_GroupByScanNumber,
		data, sizeof(data), &read, NULL));
	if (read > 0) {
		printf("Acquired %d samples. Total %d\r", read, totalSamples += read);
		fflush(stdout);
		for (i = 0; i < read * 2; i = i + 2) {
			x = data[i] * 16;
			fwrite(&x, 2, 1, fpx);
			if (x > max) max = x;
			if (x < min) min = x;
			y = data[i + 1] * 16;
			fwrite(&y, 2, 1, fpy);
			if (y > max) max = y;
			if (y < min) min = y;
		}
		//printf("Snapshot: %d|%d\n", x, y);
	}

	//printf("Max|Min: %d|%d\n", max, min);
	fclose(fpx);
	fclose(fpy);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}


int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};

	printf("Total samples: %d\n", totalSamples);
	DAQmxErrChk (status);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}


int32 writeWAVFile() {

	int temp32;
	short signed int temp16;

	const int CHANNELS = 1;
	const int BITS = 16;
	const int RATE = 37926;
	const int SAMPLES = totalSamples * 2;

	// Can't use a networked file system for this; that's too slow. It'd
	// be nice to write this as auto-generated temp files that would be portable
	// to both Windows and Linux, but this will do for now.
	FILE *fpx = fopen("/home/david/x.bin", "rb");
	FILE *fpy = fopen("/home/david/y.bin", "rb");
	FILE *fpz = fopen("/home/david/tape.wav", "wb");
	long loc;
	
	/* write the 'RIFF' header */
	fwrite("RIFF", 1, 4, fpz);

	/* ChunkSize = 36 + SubChunk2Size */
	temp32 = SAMPLES * CHANNELS * BITS + 36;
	fwrite(&temp32, 1, 4, fpz);

	/* write the 'WAVE' type */
	fwrite("WAVE", 1, 4, fpz);

	/* write the 'fmt ' tag */
	fwrite("fmt ", 1, 4, fpz);

	/* write the format record length: 16 for PCM */
	temp32 = 16;
	fwrite(&temp32, 1, 4, fpz);

	/* write the format (PCM) */
	temp16 = 1;
	fwrite(&temp16, 1, 2, fpz);

	/* write the channels */
	temp16 = CHANNELS;
	fwrite(&temp16, 1, 2, fpz);

	/* write the sample rate */
	temp32 = RATE;
	fwrite(&temp32, 1, 4, fpz);

	/* write the bytes/second */
	temp32 = RATE * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fpz);

	/* write the block align */
	temp16 = CHANNELS * (BITS / 8);
	fwrite(&temp16, 1, 2, fpz);

	/* write the bits/sample */
	temp16 = BITS;
	fwrite(&temp16, 1, 2, fpz);

	/* write the 'data' tag */
	fwrite("data", 1, 4, fpz);

	/* SubChunk2Size = NumSamples * NumChannels * bps/8 */
	temp32 = SAMPLES * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fpz);

	printf("Writing left channel...\n");
	for (loc = 0; loc < SAMPLES; loc = loc + sizeof(temp16)) {
		fread(&temp16, sizeof(temp16), 1, fpx);
		fwrite(&temp16, sizeof(temp16), 1, fpz);
	} 
	
	printf("Writing right channel...\n");
	for (loc = SAMPLES; loc >= 0; loc = loc - sizeof(temp16)) {
		fseek(fpy, loc, SEEK_SET);
		fread(&temp16, sizeof(temp16), 1, fpy);
		fwrite(&temp16, sizeof(temp16), 1, fpz);
	}

	fclose(fpx);
	fclose(fpy);
	fclose(fpz);

	return 0;

}
