#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

bool reverse_sign(int a, int b);

int main(void) {

	const int totalSamples = 226100000;
	const int WINDOW = 44100;
	const int LEVEL = 1000;
	const int FREQUENCY = floor(2000 * 2 / (44100 / WINDOW));
	int start = 0;
	int stop = 0;
	int counter;

	FILE *fpx = fopen("x.bin", "rb");
	short x, buffer[WINDOW], position, temp, flips;
	float rms;
	bool started, done, old_sign, new_sign;
	int prev_old, prev_new;

	printf("WINDOW: %d, LEVEL: %d, FREQ: %d\n", WINDOW, LEVEL, FREQUENCY);

	flips = 0;
	fseek(fpx, start, SEEK_SET);
	fread(&buffer[0], 2, 1, fpx);
	rms = abs(buffer[0]);
	for (counter = 1; counter < WINDOW; counter++) {
		fread(&buffer[counter], 2, 1, fpx);
		rms = pow(((pow(rms, 2) * counter + pow(buffer[counter], 2)) / 
			(counter + 1)), 0.5);
		if (reverse_sign(buffer[counter - 1], buffer[counter])) {
			flips++;
		}
	}
	//printf("Pre-fill buffer: rms, flips: %f\t%d\n", rms, flips);

	fread(&x, 2, 1, fpx);
	rms = pow(((pow(rms, 2) * WINDOW - pow(buffer[0], 2) + pow(x, 2)) /
		WINDOW), 0.5);
	prev_new = buffer[0];
	buffer[0] = x;
	counter = 1;
	while ((rms < LEVEL || flips < FREQUENCY) && ftell(fpx) != -1) {
		fread(&x, 2, 1, fpx);
		rms = pow(((pow(rms, 2) * WINDOW - pow(buffer[counter], 2) + pow(x, 2)) /
			WINDOW), 0.5);
		prev_old = prev_new;
		prev_new = buffer[counter];
		buffer[counter] = x;
		if (reverse_sign(prev_old, prev_new)) {
			flips--;
		}
		if (reverse_sign(buffer[counter - 1], buffer[counter])) {
			flips++;
		}
		if (counter < WINDOW) {
			counter++;
		} else {
			counter = 0;
		}
		/*
		if (counter % 1000 == 0) {
			printf("%0.2f\t%d\t%d\t%d\n", rms, flips, counter, ftell(fpx));
		}
		*/
	}

	if (ftell(fpx) == totalSamples) {
		printf("Skipping this track!\n");
	} else {
		printf("\rEnd run: rms, flips, position: %f\t%d\t%d\n", 
			rms, flips, ftell(fpx) / 2);
	}

	return 0;

}

bool reverse_sign(int a, int b) {

	bool new_sign, old_sign;

	if (a > 0) {
		old_sign = true;
	} else {
		old_sign = false;
	}

	if (b > 0) {
		new_sign = true;
	} else {
		new_sign = false;
	}
	
	if (new_sign == old_sign) {
		return false;
	} else {
		return true;
	}

}
