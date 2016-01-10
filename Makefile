build:
	g++ -Wall -l serial -ggdb -o espcli fakeserial.cpp faketime.cpp espwifi.cpp espcli.cpp
