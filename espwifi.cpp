#include "espwifi.h"
#include <signal.h>

#define LOGC(col, inp) if(state.debug) std::cerr << "\033[0" << col << "m" << inp << "\033[0m";
#define LOG(inp) LOGC(0, inp << std::endl);
#define LOGG(inp) LOGC(32, inp << std::endl);
#define LOGR(inp) LOGC(31, inp << std::endl);
#define LOGB(inp) LOGC(36, inp);


EspWifi::EspWifi(FakeSerial *serial_connection, uint8_t debug) {
	ser = serial_connection;

	state = EspState();
	state.debug = debug;

	reset();
}

int8_t EspWifi::execute(const char* cmd, const char* exp[], uint8_t len, uint16_t timeout) {
	if (strlen(cmd) > 0) {
		LOGB(std::endl << "> " << cmd << std::endl);
		ser->transmit_text(cmd, true);
	}

	// Init string lengths and response positions
	uint8_t resp_pos[len];
	uint8_t exp_len[len];
	for(uint8_t i = 0; i < len; i++) {
		resp_pos[i] = 0;
		exp_len[i] = strlen(exp[i]);
	}

	// lets start the stopwatch.
	uint16_t receive_timeout = time_ms() + timeout;
	uint8_t possible_match = 1;
	uint8_t next_byte;

	do {
		while(ser->has_data()) {
			next_byte = ser->receive_byte();

			LOGB(next_byte);
	
			for(uint8_t i = 0; i < len; i++) {
				if (next_byte == exp[i][resp_pos[i]]) {
					// Ok, current byte matches the current position
					// of this response possibility
					resp_pos[i] += 1;
					possible_match = 1;
				} else {
					// No match anymore. start over.
					resp_pos[i] = 0;
					possible_match = 0;
				}

				// Check if the current response possibility is complete,
				// if yes, return the matched entry position. Priority is
				// first match.
				if (exp_len[i] > 0 && resp_pos[i] == exp_len[i]) {
					return i;
				}
			}
		}
	} while (possible_match || (time_ms() < receive_timeout));
	return -1;
}

int8_t EspWifi::execute(const char* cmd, const char* exp, uint16_t timeout) {
	const char* exp_[1];
	exp_[0] = exp;
	return execute(cmd, exp_, 1, timeout);
}

uint8_t EspWifi::reset() {
    if (
		execute("AT+RST", "ready", 10000) > -1  // Reboot device.
		&& execute("ATE0", "OK", 1000) > -1  // Don't echo commands, reduce traffic.
	) {
		state.started = true;
		LOGG("Reset ok.");
		return true;
	} else {
		state.started = false;
		LOGR("Reset failed.");
		return false;
	}
}

uint8_t EspWifi::configure_ap(const char* ssid, const char* pass, const char* ch) {
	if (!state.started) {
		LOGR("Esp not started.");
		return false;
	}

	const char* exps[2];

	exps[0] = "OK";
	exps[1] = "no change";

	if (execute("AT+CWMODE=2", exps, 2, 1000) < 0) {
		LOGR("AP Config failed.");
		state.configured_ap = false;
		return false;
	}

	state.configured_station = false;

	// 17 for hardcoded strings, + 1 for str termination
	char sap[(17 + 1) + strlen(ssid) + strlen(pass) + strlen(ch)];
	strcpy(sap, "AT+CWSAP=\"");  // 10
	strcat(sap, ssid);  // AP Name/ssid
	strcat(sap, "\",\"");  // 3
	strcat(sap, pass);  // AP password
	strcat(sap, "\",");  // 2
	strcat(sap, ch);  // AP channel
	strcat(sap, ",2");  // 2, Encryption hardcoded to wpa2 psk

	exps[0] = "OK";
	exps[1] = "ERROR";

	if (execute(sap, exps, 2, 5000) != 0) {
		LOGR("AP Config failed.");
		state.configured_ap = false;
		return false;
	}

	LOGG("AP Config done.");
	state.configured_ap = true;
	return true;
}

uint8_t EspWifi::configure_station(const char* ssid, const char* pass) {
	if (!state.started) {
		LOGR("Esp not started.");
		return false;
	}

	const char* exps[2];

	exps[0] = "OK";
	exps[1] = "no change";

	if (execute("AT+CWMODE=1", exps, 2, 5000) < 0) {
		LOGR("Station Config failed.");
		state.configured_station = false;
		return false;
	}

	state.configured_ap = false;

	// 14 for hardcoded strings, + 1 for str termination
	char jap[(14 + 1) + strlen(ssid) + strlen(pass)];
	strcpy(jap, "AT+CWJAP=\"");  // 10
	strcat(jap, ssid);  // AP Name/ssid
	strcat(jap, "\",\"");  // 3
	strcat(jap, pass);  // AP password
	strcat(jap, "\"");  // 1

	exps[0] = "OK";
	exps[1] = "ERROR";

	if (execute(jap, exps, 2, 10000) != 0) {
		LOGR("Station Config failed.");
		state.configured_station = false;
		return false;
	}

	LOGG("Station Config done.");
	state.configured_station = true;
	return true;
}

uint8_t EspWifi::configure_server(uint16_t port) {
	if (!state.configured_ap && !state.configured_station) {
		LOGR("No ap and no station configured. Failed to start server.");
		state.configured_server = false;
		return false;
	}

	if (execute("AT+CIPMUX=1", "OK", 1000) != 0) {
		LOGR("Server Config failed.");
		state.configured_server = false;
		return false;
	}

	char portstr[5];
	convert_number(port, portstr);

	// 15 for hardcoded string, + 1 for string termination
	char cipserver[(15 + 1) + strlen(portstr)];
	strcpy(cipserver, "AT+CIPSERVER=1,");  // 15
	strcat(cipserver, portstr);  // Server port

	if (execute(cipserver, "OK", 1000) != 0) {
		LOGR("Server Config failed.");
		state.configured_server = false;
		return false;
	}

	LOGG("Server Config done.");
	state.configured_server = true;
	return true;
}

uint8_t EspWifi::get_ip_address(char ip[]) {
	if (execute("AT+CIFSR", "+CIFSR:STAIP,\"", 1000) != 0) {
		LOGR("No ip address found.");
		return 0;
	}

	for(uint8_t i = 0; i < 16; i++) {
		ip[i] = ser->receive_byte();
		if (ip[i] == '"') {
			ip[i] = '\0';
			LOGG("IP: " << ip);
			return i - 1;
		}
	}

	LOGR("No ip address found.");
	return 0;
}

uint8_t EspWifi::receive_packet(EspPacket* packet) {
	// Try to receive ip data packet.
	if (execute("", "+IPD,", 0) != 0) {
		return 0;
	}

	uint16_t i = 4;
	uint8_t buf[i] = {'\0'};
	buf[0] = ser->receive_byte();
	buf[1] = '\0';

	packet->channel = (uint16_t)atoi((char *)buf);
	LOGG("Packet received.");
	LOGG("- channel: " << packet->channel);

	// skip comma after channel to read the length.
	ser->receive_byte();

	for (i = 0; i < sizeof(buf); i++) {
		buf[i] = ser->receive_byte();
		if (buf[i] == ':') {
			buf[i] = '\0';
			break;
		}
	}

	packet->length = (uint16_t)atoi((char *)buf);
	// Ensure we don't handle more data than we can buffer.
	if (packet->length > sizeof(packet->data) - 1) {
		packet->length = sizeof(packet->data) - 1;
	}
	LOGG("- length: " << packet->length);

	packet->data[0] = '\0';
	for(i = 0; i < packet->length - 1; i++) {
		packet->data[i] = ser->receive_byte();
	}
	packet->data[i + 1] = '\0';

	LOGG("- data: " << packet->data);
	return 1;
}

uint8_t EspWifi::send_packet(EspPacket* packet, uint8_t close) {
	char channel[4], length[4];
	convert_number(packet->channel, channel);
	convert_number(packet->length, length);

	LOGG("Sending packet of length " << length << " to " << channel);

	// 12 for hardcoded strings, + 1 for str termination
	char send[(14 + 1) + strlen(channel) + strlen(length)];
	strcpy(send, "AT+CIPSEND=");  // 11
	strcat(send, channel);  // channel id
	strcat(send, ",");  // 1
	strcat(send, length);  // data length

	if (execute("", ">", 100) != 0) {
		LOGR("Sending, initializing failed.");
		return 0;
	}

	if (execute(packet->data, "SEND OK", 1000) != 0) {
		LOGR("Sending, timeout.");
		return 0;
	}

	if(close) {
		char cipclose[(12 + 1) + strlen(channel)];
		strcpy(cipclose, "AT+CIPCLOSE=");
		strcat(cipclose, channel);

		const char* exps[2] = {"OK", "ERROR"};
		execute(cipclose, exps, 2, 10);
		LOGG("Closed channel " << packet->channel);
	}

	return 0;
}

uint8_t EspWifi::send_packet(EspPacket* packet) {
	return send_packet(packet, 0);
}

uint8_t EspWifi::convert_number(uint16_t number, char text[]) {
	uint8_t i, j, c;

	i = 0;
	do {
		text[i++] = number % 10 + '0';
		number = number / 10;
	} while (number > 0);

	text[i] = '\0';

	for (i = 0, j = strlen(text) - 1; i < j; i++, j--) {
		c = text[i];
		text[i] = text[j];
		text[j] = c;
	}

	return strlen(text);
}
