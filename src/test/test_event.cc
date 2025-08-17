#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print> 

using namespace ak;

CThread reader_thread(Event* r_ready, Event* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	std::print("ReaderTask started\n");
	int outValue = -1;
	int check = 0;
	while (true) {
		assert(check < 12);

    	// Begin wait read signal
		std::print("read signal: {}\n", *r_signal); 
		if (*r_signal == 0) {
			std::print("ReaderTask about to await ...\n");
			co_await ak::wait(r_ready);
			assert(*r_signal == 1);
			*r_signal = 0;
			std::print("ReaderTask resumed\n");
		} else {
			assert(*r_signal == 1);
			*r_signal = 0;
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
		assert(*w_signal == 0);
		*w_signal = 1;
		int cc = ak::signal(w_ready);
		assert(*w_signal == 1); 
		std::print("`writeSignal` to {} writers\n", cc); 
	    // End Signal writer

		++check;
	}
}

CThread writer_thread(Event* r_ready, Event* w_ready, int *r_signal, int* w_signal, int* value) noexcept {
	int check = 0;

	std::print("WriterTask started\n");
	int i = 10;
	while (true) {
		assert(check < 12);
		*value = i;
		std::print("Written: {}\n", *value); 

		// Begin signal
		assert(*r_signal == 0); 
		*r_signal = 1;
		int cc = ak::signal(r_ready);
		assert(*r_signal == 1);
		std::print("`readSignal` fired {} readers\n", cc);
    	// End signal

		// on zero value break
		if (i == 0) {
			std::print("WriterTask done\n");
			co_return 0;
		}
		--i;

		// Begin wait write signal
		std::print("write signal: {}\n", *w_signal);
		if (*w_signal == 0) {
			std::print("WriterTask about to await ...\n");
			co_await ak::wait(w_ready);
			assert(*w_signal == 1);
			*w_signal = 0;
			std::print("WriterTask resumed\n");
		} else {
			assert(*w_signal == 1);
			*w_signal = 0;
		}
    	// End wait write signal
		++check;
	}
}


CThread co_main(const Char* name) noexcept {
	int   value = -1;
	int   r_signal = 0;
	int   w_signal = 0; 
	Event r_ready;
	Event w_ready;

	ak::init(&r_ready);
	ak::init(&w_ready);

	CThread::Hdl writer = writer_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	CThread::Hdl reader = reader_thread(&r_ready, &w_ready, &r_signal, &w_signal, &value);
	std::print("State of reader: {}; State of writer: {}\n", to_string(ak::get_state(reader)), to_string(ak::get_state(writer)));
	co_await join(reader);
	std::print("State of reader: {}; State of writer: {}\n", to_string(ak::get_state(reader)), to_string(ak::get_state(writer)));
	co_await join(writer);
	std::print("State of reader: {}; State of writer: {}\n", to_string(ak::get_state(reader)), to_string(ak::get_state(writer)));
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

	if (run_main_loop(&config, co_main, "main") != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	return 0;
}