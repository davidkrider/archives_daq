#include <stdio.h>
#include <NIDAQmx.h>
#include <time.h>


int main(void) {

	uInt8 port0[1];
	int32 written;
	TaskHandle taskHandle = 0;
	
	DAQmxCreateTask("", &taskHandle);
	DAQmxCreateDOChan(taskHandle, "Dev1/port0", "", DAQmx_Val_ChanForAllLines);
	port0[0] = 0;
	DAQmxWriteDigitalU8(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, port0, &written, NULL);
	DAQmxClearTask(taskHandle);

}
