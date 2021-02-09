#include <stdio.h>
#include <stdlib.h>

int main(void) {

	const int totalSamples = 226100000;
	const int SENSITIVITY = 500;
	int x_start = 0;
	int x_stop = 0;
	int WINDOW = 11025;
	int counter;

	FILE *fpx = fopen("y.bin", "rb");
	short x_buffer[WINDOW], x_position, x_temp;
	float x_average;

	x_average = 0;
	for (counter = 0; counter < WINDOW; counter++) {
		fread(&x_buffer[counter], 2, 1, fpx);
		x_average += (float)abs(x_buffer[counter]) / WINDOW;
	}
	
	x_position = 0;
	counter = WINDOW;
	while (x_average < SENSITIVITY && counter++ < totalSamples) {
		x_temp = abs(x_buffer[x_position]);
		x_average -= (float)x_temp / WINDOW;
		fread(&x_buffer[x_position], 2, 1, fpx);
		x_average += (float)abs(x_buffer[x_position]) / WINDOW;
		if (x_position == WINDOW) {
			x_position = 0;
		} else {
			x_position++;
		}
	}

	printf("\nStart position: %d\n", (counter - WINDOW));

	fseek(fpx, -WINDOW, SEEK_END);
	x_average = 0;
	for (counter = 0; counter < WINDOW; counter++) {
		fread(&x_buffer[counter], 2, 1, fpx);
		x_average += (float)abs(x_buffer[counter]) / WINDOW;
	}
	
	x_position = WINDOW;
	counter = totalSamples - WINDOW;
	while (x_average < SENSITIVITY && counter-- > 0) {
		x_temp = abs(x_buffer[x_position]);
		x_average -= (float)x_temp / WINDOW;
		fseek(fpx, counter, SEEK_SET);
		fread(&x_buffer[x_position], 2, 1, fpx);
		x_average += (float)abs(x_buffer[x_position]) / WINDOW;
		if (x_position == 0) {
			x_position = WINDOW;
		} else {
			x_position--;
		}
	}

	printf("\nStop position: %d\n", (counter + WINDOW));

}
