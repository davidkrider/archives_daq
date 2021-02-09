/*
  This version works by recording each side as a wav file. The second side is
  reversed, and no attempt is made to fool with it. The thought is that `sox'
  can manipulate the sound files after the recording, and do it much more 
  efficiently.
*/

#include <stdio.h>
#include <NIDAQmx.h>
#include <time.h>

int totalSamples = 0;

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else


int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, 
	uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);
int32 writeWAVHeader(FILE *fp);
int32 pokeSampleCount(FILE *fp);


int main(void) {

	int32 error = 0;
	TaskHandle taskHandle = 0;
	char errBuff[2048] = {'\0'};
	bool32 isTaskDone;
	int counter;
	int byte;

	char s[30];
	size_t i;
	struct tm tim;
	time_t now;
	now = time(NULL);
	tim = *(localtime(&now));
	i = strftime(s, 30, "%m%d%y%H%M", &tim);
	printf("%s\n", s);
	
	FILE *fpx = fopen("/local/Scratch/x.wav", "wb");
	FILE *fpy = fopen("/local/Scratch/y.wav", "wb");
	writeWAVHeader(fpx);
	writeWAVHeader(fpy);
	fclose(fpx);
	fclose(fpy);

	DAQmxErrChk (DAQmxCreateTask("", &taskHandle));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default,
		-5.0, 5.0, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle, "", 625000.0, DAQmx_Val_Rising,
		DAQmx_Val_ContSamps, 2500000));
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(taskHandle, DAQmx_Val_Acquired_Into_Buffer,
		50000, 0, EveryNCallback, NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	printf("Acquiring samples continuously. Press Enter to stop.\n");
	getchar();
	printf("Received stop.\n");

	DAQmxStopTask(taskHandle);
	printf("Stopped task.\n");
	DAQmxIsTaskDone(taskHandle, &isTaskDone);
	printf("Waiting on task to finish.\n");
	while (!isTaskDone)
		printf("Waiting on task to finish...\n");
	DAQmxClearTask(taskHandle);
	printf("Task is cleared.\n");

Error:

	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		printf("DAQmx Error: %s\n", errBuff);
		DAQmxStopTask(taskHandle);
		DAQmxIsTaskDone (taskHandle, &isTaskDone);
		while (!isTaskDone)
			printf("Waiting on task to finish...\n");
		DAQmxClearTask(taskHandle);
		return 1;
	}

	printf("All samples: %d\n", totalSamples);
	fpx = fopen("/local/Scratch/x.wav", "rb+");
	pokeSampleCount(fpx);
	fpy = fopen("/local/Scratch/y.wav", "rb+");
	pokeSampleCount(fpy);

	return 0;

}


int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType,
	uInt32 nSamples, void *callbackData) {

	int32 error = 0;
	char errBuff[2048] = {'\0'};
	int32 read = 0;
	int16 data[100000];

	FILE *fpx = fopen("/local/Scratch/x.wav", "ab+");
	FILE *fpy = fopen("/local/Scratch/y.wav", "ab+");
	int i;
	int16 x, y;

	DAQmxErrChk (DAQmxReadBinaryI16(taskHandle, 50000, 10.0, DAQmx_Val_GroupByScanNumber,
		data, sizeof(data), &read, NULL));
	if (read > 0) {
		printf("Acquired %d samples. Total %d\r", read, totalSamples += read);
		fflush(stdout);
		for (i = 0; i < read * 2; i = i + 2) {
			x = data[i] * 16;
			fwrite(&x, 2, 1, fpx);
			y = data[i + 1] * 16;
			fwrite(&y, 2, 1, fpy);
		}
	}

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

	// This isn't getting called. I don't know why.
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
