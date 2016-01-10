#include "fakeserial.h"


SerialStream ser;

FakeSerial::FakeSerial(uint16_t speed) {
	if (speed != 9600) { exit(EXIT_FAILURE); }

	ser.Open("/dev/ttyUSB0");
	ser.SetBaudRate(SerialStreamBuf::BAUD_9600);
}

void FakeSerial::transmit_byte(uint8_t input) {
	char buf[] = {(char)input};
	ser.write(buf, 1);
}

uint8_t FakeSerial::receive_byte(void) {
	char next_byte;
	ser.get(next_byte);
	return (uint8_t)next_byte;
}

uint8_t FakeSerial::has_data(void) {
	return ser.rdbuf()->in_avail() != 0;
}

uint8_t FakeSerial::is_ready(void) {
	return 1; // TODO: REAL CHECKING
}

void FakeSerial::transmit_text(const char input[]) {
	uint8_t i = 0;
	while (input[i]) {
		transmit_byte(input[i]);
		i++;
	}
}

void FakeSerial::transmit_text(const char input[], bool newline) {
	transmit_text(input);

	if (newline) {
		// Add newline and return chars.
		transmit_byte('\r');
		transmit_byte('\n');
	}
}

uint8_t FakeSerial::receive_text(char output[], uint8_t max_length) {
	uint8_t received, i;

	for(i = 0; i < max_length; i++) {
		// Get byte and echo back.
		received = receive_byte();
		transmit_byte(received);

		if (received == '\r') {
			// End of line. We're done.
			break;
		} else {
			output[i] = received;
			i++;
		}
	}

	// Add NULL to the end.
	output[i] = '\0';
	return i;
}
