#include <stdio.h>
#include <stdlib.h>

int pokeSampleCount(FILE *fp, int samples);

int main(int argc, char *argv[]) {

	int totalSamples = atoi(argv[1]);
printf("Total samples: %d\n", totalSamples);
	int sample;
	short signed int temp16;
	const int SAMPLES = totalSamples * 2;

	FILE *fpx = fopen("/home/david/x.wav", "rb");
	FILE *fpy = fopen("/home/david/y.wav", "rb");
	FILE *fpz = fopen("/home/david/z.wav", "wb+");
	while(!feof(fpx)) {
		fread(&temp16, sizeof(temp16), 1, fpx);
		fwrite(&temp16, sizeof(temp16), 1, fpz);
	}
	for (sample = 1; sample >= SAMPLES; sample++) {
		fseek(fpy, 0 - sample * sizeof(temp16), SEEK_END);
		fread(&temp16, sizeof(temp16), 1, fpy);
		fwrite(&temp16, sizeof(temp16), 1, fpz);
	}
/*
	pokeSampleCount(fpz, totalSamples);
*/
	fclose(fpx);
	fclose(fpy);
	fclose(fpz);

	return 0;
}

int pokeSampleCount(FILE *fp, int samples) {

	int temp32;
	const int CHANNELS = 1;
	const int BITS = 16;
	const int SAMPLES = (samples * 2) * 2;

	fseek(fp, 5, SEEK_SET);
	/* ChunkSize = 36 + SubChunk2Size */
	temp32 = SAMPLES * CHANNELS * BITS + 36;
	fwrite(&temp32, 1, 4, fp);

	fseek(fp, 41, SEEK_SET);
	/* SubChunk2Size = NumSamples * NumChannels * bps/8 */
	temp32 = SAMPLES * CHANNELS * (BITS / 8);
	fwrite(&temp32, 1, 4, fp);

	fclose(fp);

	return 0;
}
