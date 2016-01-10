#ifndef espwifi_h
#define espwifi_h

#include <iostream>
#include <string.h>
#include "fakeserial.h"
#include "faketime.h"


struct EspState {
	uint8_t started: 1;
	uint8_t configured_ap: 1;
	uint8_t configured_station: 1;
	uint8_t configured_server: 1;
	uint8_t debug: 1;
};

struct EspPacket {
	uint16_t channel;
	uint16_t length;
	char data[1024];
};

class EspWifi {
	public:
		EspWifi(FakeSerial*, uint8_t debug);

		int8_t execute(const char* cmd, const char* exp[], uint8_t len, uint16_t timeout);
		int8_t execute(const char* cmd, const char* exp, uint16_t timeout);

		uint8_t reset(void);
		uint8_t configure_ap(const char* ssid, const char* pass, const char* ch);
		uint8_t configure_station(const char* ssid, const char* pass);
		uint8_t configure_server(uint16_t port);

		uint8_t get_ip_address(char ip[]);
		uint8_t receive_packet(EspPacket* packet);
		uint8_t send_packet(EspPacket* packet);
		uint8_t send_packet(EspPacket* packet, uint8_t close);

	private:
		FakeSerial *ser;
		EspState state;

		uint8_t convert_number(uint16_t number, char text[]);

};

#endif
