#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include "fakeserial.h"
#include "faketime.h"
#include "espwifi.h"
#include "config.h"


int main(int argc, char* argv[]) {
	time_ms(1); // init fake time.

	std::cout << "ESPCLI." << std::endl;
	FakeSerial serial = FakeSerial((uint16_t)9600U);

	EspWifi wifi = EspWifi(&serial, true);

	//if (!wifi.configure_ap("esp_test", "secret", "9")) {
	if (!wifi.configure_station(SSID, SSID_PASSWORD)) {
		return 1;
	}

	if(!wifi.configure_server(80)) {
		return 1;
	}

	char ip[16];
	wifi.get_ip_address(ip);
	std::cout << "My IP: " << ip << std::endl;

	EspPacket packet = EspPacket();

	while(true) {
		if (wifi.receive_packet(&packet)) {
			std::cout << "New packet arrived." << std::endl;
			strcpy(packet.data, "You sent bytes..\n");
			packet.length = strlen(packet.data);
			wifi.send_packet(&packet, true);
		}
	};

	return 0;
}
