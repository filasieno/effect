#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

DefineTask ReaderTask(Event* canRead, Event* canWrite, int *readSignal, int* writeSignal, int* value) noexcept {
	std::print("ReaderTask started\n");
	int outValue = -1;
	int check = 0;
	while (true) {
		assert(check < 12);

    	// Begin wait read signal
		std::print("read signal: {}\n", *readSignal); 
		if (*readSignal == 0) {
			std::print("ReaderTask about to await ...\n");
			co_await WaitEvent(canRead);
			assert(*readSignal == 1);
			*readSignal = 0;
			std::print("ReaderTask resumed\n");
		} else {
			assert(*readSignal == 1);
			*readSignal = 0;
		}
    	// End wait read signal

    	// Read value
		outValue = *value;
	    std::print("Read: {}\n", outValue);
    	if (outValue == 0) {
			std::print("ReaderTask done\n");
			co_return 0;
		}

    	// Begin Signal writer
		assert(*writeSignal == 0);
		*writeSignal = 1;
		int cc = SignalEventOne(canWrite);
		assert(*writeSignal == 1); 
		std::print("`writeSignal` to {} writers\n", cc); 
	    // End Signal writer

		++check;
	}
}

DefineTask WriterTask(Event* canRead, Event* canWrite, int *readSignal, int* writeSignal, int* value) noexcept {
	int check = 0;

	std::print("WriterTask started\n");
	int i = 10;
	while (true) {
		assert(check < 12);
		*value = i;
		std::print("Written: {}\n", *value); 

		// Begin signal
		assert(*readSignal == 0); 
		*readSignal = 1;
		int cc = SignalEventOne(canRead);
		assert(*readSignal == 1);
		std::print("`readSignal` fired {} readers\n", cc);
    	// End signal

		// on zero value break
		if (i == 0) {
			std::print("WriterTask done\n");
			co_return 0;
		}
		--i;

		// Begin wait write signal
		std::print("write signal: {}\n", *writeSignal);
		if (*writeSignal == 0) {
			std::print("WriterTask about to await ...\n");
			co_await WaitEvent(canWrite);
			assert(*writeSignal == 1);
			*writeSignal = 0;
			std::print("WriterTask resumed\n");
		} else {
			assert(*writeSignal == 1);
			*writeSignal = 0;
		}
    	// End wait write signal
		++check;
	}
}


DefineTask MainTask(const Char* name) noexcept {
	int   value = -1;
	int   readSignal = 0;
	int   writeSignal = 0; 
	Event readyToRead;
	Event readyToWrite;

	InitEvent(&readyToRead);
	InitEvent(&readyToWrite);

	TaskHdl writer = WriterTask(&readyToRead, &readyToWrite, &readSignal, &writeSignal, &value);
	TaskHdl reader = ReaderTask(&readyToRead, &readyToWrite, &readSignal, &writeSignal, &value);
	std::print("State of reader: {}; State of writer: {}\n", to_string(GetTaskState(reader)), to_string(GetTaskState(writer)));
	co_await JoinTask(reader);
	std::print("State of reader: {}; State of writer: {}\n", to_string(GetTaskState(reader)), to_string(GetTaskState(writer)));
	co_await JoinTask(writer);
	std::print("State of reader: {}; State of writer: {}\n", to_string(GetTaskState(reader)), to_string(GetTaskState(writer)));
	std::fflush(stdout);
	
	co_return 0;
}


Char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
  	};

	if (RunMain(&config, MainTask, "main") != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	return 0;
}