#ifndef fakeserial_h
#define fakeserial_h

#include <stdint.h>
#include <stdlib.h>
#include <SerialStream.h>

using namespace LibSerial;


class FakeSerial {
	public:
		FakeSerial(uint16_t speed);

		// Basic input/output.
		void transmit_byte(uint8_t input);
		uint8_t receive_byte(void);

		// Helpers to read flags.
		uint8_t has_data(void);
		uint8_t is_ready(void);

		// Transmit helpers.
		void transmit_text(const char input[]);
		void transmit_text(const char input[], bool newline);

		// Receive helpers.
		uint8_t receive_text(char output[], uint8_t max_length);
};

#endif
